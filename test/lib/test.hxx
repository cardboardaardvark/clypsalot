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

#include <memory>

// These must preceed the include of the unit_test.hpp
#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_ALTERNATIVE_INIT_API

#ifdef TEST_NAME
#define BOOST_TEST_MODULE TEST_NAME
#endif

#include <boost/test/unit_test.hpp>

#define TEST_MAIN_FUNCTION int main(int argc, char* argv[]) { return runTests(argc, argv); }
#define TEST_OBJECT_KIND "Test::Object"

namespace Clypsalot
{
    int runTests(int argc, char* argv[]);
    void initTesting(int argc, char* argv[]);
    bool initBoostUnitTest();

    template <class T>
    std::shared_ptr<T> makeTestObject()
    {
        return std::dynamic_pointer_cast<T>(T::make());
    }
}
