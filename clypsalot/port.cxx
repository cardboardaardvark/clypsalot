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
#include <clypsalot/logging.hxx>
#include <clypsalot/macros.hxx>
#include <clypsalot/object.hxx>
#include <clypsalot/port.hxx>
#include <clypsalot/util.hxx>

namespace Clypsalot
{
    PortType::PortType(const std::string& name) :
        name(name)
    { }

    PortLink::PortLink(OutputPort& from, InputPort& to) :
        from(from),
        to(to)
    { }

    bool PortLink::operator==(const PortLink& other)
    {
        return this == &other;
    }

    bool PortLink::operator!=(const PortLink& other)
    {
        return this != &other;
    }

    void PortLink::setEndOfData() noexcept
    {
        std::unique_lock lock(mutex);
        endOfDataFlag = true;
    }

    bool PortLink::endOfData() const noexcept
    {
        std::unique_lock lock(mutex);
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

    bool Port::operator==(const Port& other)
    {
        return this == &other;
    }

    bool Port::operator!=(const Port& other)
    {
        return this != &other;
    }

    const std::vector<PortLink*>& Port::links() const noexcept
    {
        assert(parent.haveLock());

        return portLinks;
    }

    void Port::addLink(PortLink* link)
    {
        assert(parent.haveLock());

        if (parent.state() != ObjectState::paused)
        {
            throw StateError(makeString("Object was not paused when adding a link: ", parent));
        }

        portLinks.push_back(link);
    }

    void Port::removeLink(const PortLink* link)
    {
        assert(parent.haveLock());

        for (auto i = portLinks.begin(); i != portLinks.end();)
        {
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

        auto link = output.type.makeLink(output, input);
        std::vector<SharedObject> startObjects;
        Finally finally([&startObjects]
        {
            for (auto object : startObjects)
            {
                LOGGER(trace, "Starting object that was paused during linking: ", *object);
                startObject(object);
            }
        });

        for (auto object : { output.parent.shared_from_this(), input.parent.shared_from_this() })
        {
            if (pauseObject(object))
            {
                startObjects.push_back(object);
            }
        }

        output.addLink(link);
        input.addLink(link);

        return link;
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
                LOGGER(debug, "Starting object that was paused for unlinking: ", *object);
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

    std::string asString(const OutputPort& port) noexcept
    {
      return std::string(asString(port.parent))
        + "(output=" + port.type.name + ":"
        + port.name + ")";
    }

    std::ostream& operator<<(std::ostream& os, const OutputPort& port) noexcept
    {
        os << asString(port);
        return os;
    }

    std::string asString(const InputPort& port) noexcept
    {
      return std::string(asString(port.parent))
        + "(input=" + port.type.name + ":"
        + port.name + ")";
    }

    std::ostream& operator<<(std::ostream& os, const InputPort& port) noexcept
    {
        os << asString(port);
        return os;
    }
}
