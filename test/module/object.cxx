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

#include "test/module/object.hxx"
#include "test/module/port.hxx"

namespace Clypsalot
{
    const std::string TestObject::objectKind = "Test::Object";

    SharedObject TestObject::make()
    {
        return std::make_shared<TestObject>();
    }

    TestObject::TestObject() :
        Object()
    {
        std::unique_lock lock(*this);

        addOutput<TestOutputPort>("test");
        addInput<TestInputPort>("test");
    }

    const std::string& TestObject::kind() noexcept
    {
        return objectKind;
    }
}
