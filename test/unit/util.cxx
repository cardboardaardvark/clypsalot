/* Copyright 2023 Tyler Riddle
 *
 * This file is part of Clypsalot. Clypsalot is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version. Clypsalot is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with Clypsalot. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include <clypsalot/util.hxx>

#include "test/lib/test.hxx"

using namespace Clypsalot;

TEST_MAIN_FUNCTION

#define TEST_TYPE_NAME "TestType"
struct TestType { };

#define TEST_TYPE_SSTREAM_VALUE "Test value"

std::ostream& operator<<(std::ostream& os, const TestType) noexcept
{
    os << TEST_TYPE_SSTREAM_VALUE;
    return os;
}

BOOST_AUTO_TEST_CASE(makeString_function)
{
    BOOST_CHECK(typeid(makeString("")) == typeid(std::string));
    BOOST_CHECK(makeString("A string") == "A string");
    BOOST_CHECK(makeString("A longer ", "string") == "A longer string");
    BOOST_CHECK(makeString(1) == "1");
    BOOST_CHECK(makeString(0.0) == "0");
}

BOOST_AUTO_TEST_CASE(typeName_function)
{
    const TestType testTypeInstance;
    BOOST_CHECK(typeName(typeid(TestType)) == TEST_TYPE_NAME);
    BOOST_CHECK(typeName(typeid(testTypeInstance)) == TEST_TYPE_NAME);
}
