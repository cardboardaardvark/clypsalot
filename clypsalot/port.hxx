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
    class PortType
    {
        protected:
        const std::string& m_name;

        PortType(const std::string& in_name);

        public:
        virtual ~PortType() = default;
        const std::string& name() const noexcept;
        virtual PortLink* makeLink(OutputPort& from, InputPort& to) const = 0;
    };

    class PortLink : protected Lockable
    {
        bool m_endOfDataFlag = false;
        // FIXME The output and input ports should be std::weak_ptr
        OutputPort& m_from;
        InputPort& m_to;

        public:
        static std::string toString(const PortLink& link) noexcept;
        PortLink(OutputPort& in_from, InputPort& in_to);
        PortLink(const PortLink&) = delete;
        virtual ~PortLink() = default;
        void operator=(const PortLink&) = delete;
        bool operator==(const PortLink& rhs);
        bool operator!=(const PortLink& rhs);
        OutputPort& from() const noexcept;
        InputPort& to() const noexcept;
        void setEndOfData() noexcept;
        bool endOfData() const noexcept;
    };

    class Port
    {
        protected:
        // FIXME The parent should be a std::weak_ptr
        Object& m_parent;
        const std::string m_name;
        const PortType& m_type;
        std::vector<PortLink*> portLinks;

        Port(const std::string& in_name, const PortType& in_type, Object& in_parent);
        PortLink* findLink(const OutputPort& in_from, const InputPort& in_to) const noexcept;

        public:
        Port(const Port&) = delete;
        virtual ~Port();
        void operator=(const Port&) = delete;
        bool operator==(const Port& rhs);
        bool operator!=(const Port& rhs);
        const std::string& name() const noexcept;
        const PortType& type() const noexcept;
        Object& parent() const noexcept;
        const std::vector<PortLink*>& links() const noexcept;
        bool hasLink(const PortLink* link) const noexcept;
        void addLink(PortLink* link);
        void removeLink(const PortLink* link);
    };

    class OutputPort : public Port
    {
        public:
        static std::string toString(const OutputPort& port) noexcept;
        OutputPort(const std::string& name, const PortType& type, Object& parent);
        PortLink* findLink(const InputPort& to) const noexcept;
        void setEndOfData() const noexcept;
        virtual bool ready() const noexcept = 0;
    };

    class InputPort : public Port
    {
        public:
        static std::string toString(const InputPort& port) noexcept;
        InputPort(const std::string& name, const PortType& type, Object& parent);
        PortLink* findLink(const OutputPort& from) const noexcept;
        bool endOfData() const noexcept;
        virtual bool ready() const noexcept = 0;
    };

    PortLink* linkPorts(OutputPort& output, InputPort& input);
    std::vector<PortLink*> linkPorts(const std::vector<std::pair<OutputPort&, InputPort&>>& portList);
    void unlinkPorts(OutputPort& output, InputPort& input);
    void unlinkPorts(const std::vector<std::pair<OutputPort&, InputPort&>>& portList);
    std::ostream& operator<<(std::ostream& os, const OutputPort& port) noexcept;
    std::ostream& operator<<(std::ostream& os, const InputPort& port) noexcept;
    std::ostream& operator<<(std::ostream&os, const PortLink& link) noexcept;
}
