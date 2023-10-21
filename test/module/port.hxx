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

#include <clypsalot/port.hxx>

namespace Clypsalot
{
    struct TestPortType : public PortType
    {
        static const std::string typeName;
        static const TestPortType portTypeSingleton;

        virtual const std::string& name() noexcept override;
        virtual PortLink* makeLink(const OutputPort& from, const InputPort& to) const override;
    };

    class TestOutputPort : public OutputPort
    {
        public:
        TestOutputPort(const std::string& name, Object& parent);
    };

    class TestInputPort : public InputPort
    {
        public:
        TestInputPort(const std::string& name, Object& parent);
    };

    class TestPortLink : public PortLink
    {
        public:
        TestPortLink(const TestOutputPort& from, const TestInputPort& to);
    };
}
