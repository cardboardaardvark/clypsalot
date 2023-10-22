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

#include <clypsalot/catalog.hxx>
#include <clypsalot/error.hxx>

#include "test/module/port.hxx"

namespace Clypsalot
{
    const std::string TestPortType::typeName = "test";
    const TestPortType TestPortType::portTypeSingleton = TestPortType();

    const std::string& TestPortType::name() const noexcept
    {
        return typeName;
    }

    PortLink* TestPortType::makeLink(OutputPort& from, InputPort& to) const
    {
        const auto& ourTypeInfo = typeid(TestPortType);
        const auto& fromType = from.type();
        const auto& toType = to.type();

        if (typeid(fromType) != ourTypeInfo || typeid(toType) != ourTypeInfo)
        {
            throw TypeError("Incompatible port types when creating a link");
        }

        return new TestPortLink(
            dynamic_cast<TestOutputPort&>(from),
            dynamic_cast<TestInputPort&>(to)
        );
    }

    TestOutputPort::TestOutputPort(const std::string& name, Object& parent) :
        OutputPort(name, TestPortType::portTypeSingleton, parent)
    { }

    TestInputPort::TestInputPort(const std::string& name, Object& parent) :
        InputPort(name, TestPortType::portTypeSingleton, parent)
    { }

    TestPortLink::TestPortLink(TestOutputPort& from, TestInputPort& to) :
        PortLink(from, to)
    { }
}
