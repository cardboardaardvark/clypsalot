/* Copyright 2023 Tyler Riddle
 *
 * This file is part of Clypsalot. Clypsalot is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version. Clypsalot is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with Clypsalot. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include <cassert>

#include <clypsalot/error.hxx>
#include <clypsalot/logger.hxx>
#include <clypsalot/macros.hxx>
#include <clypsalot/object.hxx>
#include <clypsalot/port.hxx>
#include <clypsalot/util.hxx>

namespace Clypsalot
{
    [[noreturn]] static void objectStateLinkError(const SharedObject& object)
    {
        throw ObjectStateError(object, object->state(), "Object must be paused");
    }

    PortType::PortType(const std::string& name) :
        name(name)
    { }

    std::string PortLink::toString(const PortLink& link) noexcept
    {
        std::string retval;

        retval += Clypsalot::toString(link.from);
        retval += " -> ";
        retval += Clypsalot::toString(link.to);

        return retval;
    }

    PortLink::PortLink(OutputPort& from, InputPort& to) :
        from(from),
        to(to)
    { }

    bool PortLink::operator==(const PortLink& rhs)
    {
        if (from == rhs.from && to == rhs.to) return true;
        return false;
    }

    bool PortLink::operator!=(const PortLink& rhs)
    {
        return ! operator==(rhs);
    }

    void PortLink::setEndOfData() noexcept
    {
        std::scoped_lock lock(mutex);
        endOfDataFlag = true;
    }

    bool PortLink::endOfData() const noexcept
    {
        std::scoped_lock lock(mutex);
        return endOfDataFlag;
    }

    Port::Port(const std::string& name, const PortType& type, Object& parent) :
        parent(parent),
        name(name),
        type(type)
    { }

    Port::~Port()
    {
        if (portLinks.size() > 0)
        {
            FATAL_ERROR(makeString("Port had links during destruction on ", parent));
        }
    }

    bool Port::operator==(const Port& rhs)
    {
        return this == &rhs;
    }

    bool Port::operator!=(const Port& rhs)
    {
        return this != &rhs;
    }

    const std::vector<PortLink*>& Port::links() const noexcept
    {
        assert(parent.haveLock());

        return portLinks;
    }

    // Return true if the given link is stored in the current link list. This is not for
    // checking if the ports are linked only if the specific link is present.
    bool Port::hasLink(const PortLink* link_in) const noexcept
    {
        assert(parent.haveLock());

        for (const auto link : portLinks)
        {
            // This uses a pointer comparison instead of operator== from PortLink because it is
            // checking for presence of this specific link instance instead of a link existing at
            // all between the ports.
            if (link == link_in) return true;
        }

        return false;
    }

    void Port::addLink(PortLink* link)
    {
        assert(parent.haveLock());

        if (parent.state() != ObjectState::paused) objectStateLinkError(parent.shared_from_this());
        if (findLink(link->from, link->to)) throw DuplicateLinkError(link->from, link->to);

        portLinks.push_back(link);
    }

    void Port::removeLink(const PortLink* link)
    {
        assert(parent.haveLock());

        for (auto i = portLinks.begin(); i != portLinks.end();)
        {
            // This pointer comparison is used instead of operator== on PortLink
            // because it should only remove a specific link that exists in the port instead
            // of removing any link that matches the ports in the link given as the argument.
            if (*i == link)
            {
                i = portLinks.erase(i);
                return;
            }
            else
            {
                i++;
            }
        }

        throw RuntimeError("Link not found");
    }

    PortLink* Port::findLink(const OutputPort& from, const InputPort& to) const noexcept
    {
        assert(parent.haveLock());

        for (const auto link : portLinks)
        {
            if (link->from == from && link->to == to)
            {
                return link;
            }
        }

        return nullptr;
    }

    std::string OutputPort::toString(const OutputPort& port) noexcept
    {
        std::string retval;

        retval += Clypsalot::toString(port.parent);
        retval += "(output=" + port.type.name + ":";
        retval += port.name + ")";

        return retval;
    }

    OutputPort::OutputPort(const std::string& name, const PortType& type, Object& parent) :
        Port(name, type, parent)
    { }

    void OutputPort::setEndOfData() const noexcept
    {
        assert(parent.haveLock());

        for (const auto link : links())
        {
            link->setEndOfData();
        }
    }

    PortLink* OutputPort::findLink(const InputPort& to) const noexcept
    {
        assert(parent.haveLock());

        return Port::findLink(*this, to);
    }

    std::string InputPort::toString(const InputPort& port) noexcept
    {
        std::string retval;

        retval += Clypsalot::toString(port.parent);
        retval += "(input=" + port.type.name + ":";
        retval += port.name + ")";

        return retval;
    }

    InputPort::InputPort(const std::string& name, const PortType& type, Object& parent) :
        Port(name, type, parent)
    { }

    PortLink* InputPort::findLink(const OutputPort& from) const noexcept
    {
        assert(parent.haveLock());

        return Port::findLink(from, *this);
    }

    bool InputPort::endOfData() const noexcept
    {
        assert(parent.haveLock());

        for (const auto link : links())
        {
            if (link->endOfData()) return true;
        }

        return false;
    }

    PortLink* linkPorts(OutputPort& output, InputPort& input)
    {
        assert(output.parent.haveLock());
        assert(input.parent.haveLock());

        LOGGER(debug, "Linking ", output, " to ", input);

        if (output.findLink(input) || input.findLink(output)) throw DuplicateLinkError(output, input);

        PortLink* link = nullptr;
        std::vector<SharedObject> startObjects;
        Finally finally([&startObjects]
        {
            for (auto object : startObjects)
            {
                LOGGER(trace, "Starting object that was paused during linking: ", *object);
                startObject(object);
            }
        });

        auto outputObject = output.parent.shared_from_this();
        auto inputObject = input.parent.shared_from_this();

        for (auto object : {outputObject, inputObject})
        {
            if (pauseObject(object))
            {
                startObjects.push_back(object);
            }

            if (object->state() != ObjectState::paused) objectStateLinkError(object);
        }

        try {
            link = output.type.makeLink(output, input);
            output.addLink(link);
            input.addLink(link);
            return link;
        }
        catch (...)
        {
            if (link != nullptr)
            {
                 if (output.hasLink(link)) output.removeLink(link);
                 if (input.hasLink(link)) input.removeLink(link);
                 delete link;
            }

            throw;
        }
    }

    std::vector<PortLink*> linkPorts(const std::vector<std::pair<OutputPort&, InputPort&>>& portList)
    {
        std::vector<PortLink*> links;
        bool needUnlink = true;
        std::vector<SharedObject> startObjects;
        std::map<SharedObject, bool> seenObject;
        Finally finally([&needUnlink, &links, &startObjects] {
            if (needUnlink)
            {
                for (const auto& link : links)
                {
                    unlinkPorts(link->from, link->to);
                }
            }

            for (const auto& object : startObjects)
            {
                startObject(object);
            }
        });

        links.reserve(portList.size());

        for (const auto& ports : portList)
        {
            auto fromParent = ports.first.parent.shared_from_this();
            auto toParent = ports.second.parent.shared_from_this();

            assert(fromParent->haveLock());
            assert(toParent->haveLock());

            for (const auto& object : { fromParent, toParent })
            {
                if (! seenObject.contains(object))
                {
                    seenObject[object] = true;

                    if (pauseObject(object))
                    {
                        startObjects.push_back(object);
                    }
                }
            }

            auto link = linkPorts(ports.first, ports.second);
            links.push_back(link);
        }

        needUnlink = false;

        return links;
    }

    /**
     * @brief Unlink the Object ports.
     * @param output The output port to unlink
     * @param input The input port to unlink.
     *
     * Unlink the ports and pause and start the object if needed.
     */
    void unlinkPorts(OutputPort& output, InputPort& input)
    {
        assert(output.parent.haveLock());
        assert(input.parent.haveLock());

        LOGGER(debug, "Unlinking ", output, " from ", input);

        auto outputLink = output.findLink(input);
        auto inputLink = input.findLink(output);

        if (outputLink != inputLink)
        {
            throw RuntimeError(makeString("Can't unlink objects: inconsistent links between ", output, " and ", input));
        }

        if (outputLink == nullptr)
        {
            throw RuntimeError(makeString("Ports ", output, " and ", input, " are not linked"));
        }

        assert(outputLink->from == output);
        assert(outputLink->to == input);

        std::vector<SharedObject> startObjects;
        Finally finally([&startObjects]
        {
            for (auto& object : startObjects)
            {
                LOGGER(trace, "Starting object that was paused for unlinking: ", *object);
                startObject(object);
            }
        });

        for (auto object : { output.parent.shared_from_this(), input.parent.shared_from_this() })
        {
            if (! objectIsShutdown(object->state()) && pauseObject(object))
            {
                startObjects.push_back(object);
            }
        }

        output.removeLink(outputLink);
        input.removeLink(outputLink);
        delete(outputLink);
    }

    /**
     * @brief Atomically unlink a list of ports
     */
    void unlinkPorts(const std::vector<std::pair<OutputPort&, InputPort&>>& portList)
    {
        std::vector<std::pair<OutputPort&, InputPort&>> didUnlink;
        bool needRelink = true;
        std::vector<SharedObject> startObjects;
        std::map<SharedObject, bool> seenObject;
        Finally finally([&needRelink, &didUnlink, &startObjects] {
            if (needRelink)
            {
                for (const auto& ports : didUnlink)
                {
                    linkPorts(ports.first, ports.second);
                }
            }

            for (const auto& object : startObjects)
            {
                object->start();
            }
        });

        didUnlink.reserve(portList.size());

        for (const auto& ports : portList)
        {
            auto fromParent = ports.first.parent.shared_from_this();
            auto toParent = ports.second.parent.shared_from_this();

            assert(fromParent->haveLock());
            assert(toParent->haveLock());

            for (const auto& object : { fromParent, toParent })
            {
                if (! seenObject.contains(object))
                {
                    seenObject[object] = true;

                    if (! objectIsShutdown(object->state()) && pauseObject(object))
                    {
                        startObjects.push_back(object);
                    }
                }
            }

            unlinkPorts(ports.first, ports.second);
            didUnlink.push_back(ports);
        }

        needRelink = false;
    }

    std::ostream& operator<<(std::ostream& os, const InputPort& port) noexcept
    {
        os << toString(port);
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const OutputPort& port) noexcept
    {
        os << toString(port);
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const PortLink& link) noexcept
    {
        os << toString(link);
        return os;
    }
}
