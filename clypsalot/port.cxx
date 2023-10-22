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
    PortLink::PortLink(OutputPort& from, InputPort& to) :
        fromPort(from),
        toPort(to)
    { }

    bool PortLink::operator==(const PortLink& other)
    {
        return this == &other;
    }

    bool PortLink::operator!=(const PortLink& other)
    {
        return this != &other;
    }

    Port::Port(const std::string& name, const PortType& type, Object& parent) :
        parentObject(parent),
        portName(name),
        portType(type)
    { }

    Port::~Port()
    {
        if (portLinks.size() > 0)
        {
            FATAL_ERROR(makeString("Port had links during destruction: ", *this));
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

    const std::string& Port::name() const
    {
        return portName;
    }

    Object& Port::parent() const
    {
        // No lock required because the parent object will never change after construction.
        return parentObject;
    }

    OutputPort& PortLink::from() const noexcept
    {
        return fromPort;
    }

    InputPort& PortLink::to() const noexcept
    {
        return toPort;
    }

    const std::vector<PortLink*>& Port::links() const noexcept
    {
        assert(parentObject.haveLock());

        return portLinks;
    }

    void Port::addLink(PortLink* link) noexcept
    {
        assert(parentObject.haveLock());

        portLinks.push_back(link);
    }

    void Port::removeLink(const PortLink* link)
    {
        assert(parentObject.haveLock());

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
        assert(parentObject.haveLock());

        for (const auto link : portLinks)
        {
            if (link->from() == from && link->to() == to)
            {
                return link;
            }
        }

        return nullptr;
    }

    const PortType& Port::type() const
    {
        return portType;
    }

    OutputPort::OutputPort(const std::string& name, const PortType& type, Object& parent) :
        Port(name, type, parent)
    { }

    PortLink* OutputPort::findLink(const InputPort& to) const noexcept
    {
        assert(parent().haveLock());

        return Port::findLink(*this, to);
    }

    InputPort::InputPort(const std::string& name, const PortType& type, Object& parent) :
        Port(name, type, parent)
    { }

    PortLink* InputPort::findLink(const OutputPort& from) const noexcept
    {
        assert(parent().haveLock());

        return Port::findLink(from, *this);
    }

    PortLink& linkPorts(OutputPort& output, InputPort& input)
    {
        assert(output.parent().haveLock());
        assert(input.parent().haveLock());

        LOGGER(debug, "Linking ", output, " to ", input);

        auto link = output.type().makeLink(output, input);
        output.addLink(link);
        input.addLink(link);

        return *link;
    }

    void unlinkPorts(OutputPort& output, InputPort& input)
    {
        assert(output.parent().haveLock());
        assert(input.parent().haveLock());

        LOGGER(debug, "Unlinking ", output, " from ", input);

        auto outputLink = output.findLink(input);
        auto inputLink = input.findLink(output);

        assert(outputLink == inputLink);

        if (outputLink == nullptr)
        {
            throw RuntimeError("Ports are not linked");
        }

        output.removeLink(outputLink);
        input.removeLink(outputLink);
        delete(outputLink);
    }

    std::string asString(const Port& port) noexcept
    {
      return std::string(asString(port.parent()))
              + "(" + port.type().name() + ":"
              + port.name() + ")";
    }

    std::ostream& operator<<(std::ostream& os, const Port& port) noexcept
    {
        os << asString(port);
        return os;
    }
}
