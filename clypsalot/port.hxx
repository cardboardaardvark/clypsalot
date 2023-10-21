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
        virtual const std::string& name() noexcept = 0;
        virtual PortLink* makeLink(const OutputPort& from, const InputPort& to) const = 0;
    };

    class PortLink : Lockable
    {
        const OutputPort& fromPort;
        const InputPort& toPort;

        public:
        PortLink(const OutputPort& from, const InputPort& to);
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

        public:
        virtual ~Port() = default;
        const std::string& name() const;
        const PortType& type() const;
        Object& parent();
        const std::vector<PortLink*> links() const noexcept;
        void addLink(PortLink* link) noexcept;
    };

    class OutputPort : public Port
    {
        public:
        OutputPort(const std::string& name, const PortType& type, Object& parent);
    };

    class InputPort : public Port
    {
        public:
        InputPort(const std::string& name, const PortType& type, Object& parent);
    };
}
