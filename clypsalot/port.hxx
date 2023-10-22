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

#pragma once

#include <vector>
#include <string>

#include <clypsalot/forward.hxx>
#include <clypsalot/port.hxx>
#include <clypsalot/thread.hxx>

namespace Clypsalot
{
    struct PortType
    {
        PortType() = default;
        virtual ~PortType() = default;
        virtual const std::string& name() const noexcept = 0;
        virtual PortLink* makeLink(OutputPort& from, InputPort& to) const = 0;
    };

    class PortLink : Lockable
    {
        OutputPort& fromPort;
        InputPort& toPort;

        public:
        PortLink(OutputPort& from, InputPort& to);
        PortLink(const PortLink&) = delete;
        virtual ~PortLink() = default;
        void operator=(const PortLink&) = delete;
        bool operator==(const PortLink& other);
        bool operator!=(const PortLink& other);
        OutputPort& from() const noexcept;
        InputPort& to() const noexcept;
    };

    class Port
    {
        Object& parentObject;
        const std::string portName;
        const PortType& portType;
        std::vector<PortLink*> portLinks;

        protected:
        Port(const std::string& name, const PortType& type, Object& parent);
        PortLink* findLink(const OutputPort& from, const InputPort& to) const noexcept;

        public:
        Port(const Port&) = delete;
        virtual ~Port();
        void operator=(const Port&) = delete;
        bool operator==(const Port& other);
        bool operator!=(const Port& other);
        const std::string& name() const;
        const PortType& type() const;
        Object& parent() const;
        const std::vector<PortLink*>& links() const noexcept;
        void addLink(PortLink* link) noexcept;
        void removeLink(const PortLink* link);
    };

    class OutputPort : public Port
    {
        public:
        OutputPort(const std::string& name, const PortType& type, Object& parent);
        PortLink* findLink(const InputPort& to) const noexcept;
    };

    class InputPort : public Port
    {
        public:
        InputPort(const std::string& name, const PortType& type, Object& parent);
        PortLink* findLink(const OutputPort& from) const noexcept;
    };

    PortLink& linkPorts(OutputPort& output, InputPort& input);
    void unlinkPorts(OutputPort& output, InputPort& input);
    std::string asString(const Port& port) noexcept;
    std::ostream& operator<<(std::ostream& os, const Port& port) noexcept;
}
