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

#include <atomic>

#include <clypsalot/catalog.hxx>
#include <clypsalot/network.hxx>
#include <clypsalot/object.hxx>

// These must preceed the include of the unit_test.hpp
#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_ALTERNATIVE_INIT_API

#ifdef TEST_NAME
#define BOOST_TEST_MODULE TEST_NAME
#endif

#include <boost/test/unit_test.hpp>

#define TEST_MAIN_FUNCTION int main(int argc, char* argv[]) { return Clypsalot::runTests(argc, argv); }
#define TEST_OBJECT_KIND "Test::Object"
#define TEST_CASE(name) BOOST_FIXTURE_TEST_CASE(name, Clypsalot::TestCaseFixture)

namespace Clypsalot
{
    std::atomic_size_t severeLogEvents = ATOMIC_VAR_INIT(0);

    struct TestCaseFixture
    {
        TestCaseFixture();
        ~TestCaseFixture();
    };

    int runTests(int argc, char* argv[]);
    bool initBoostUnitTest();

    template <std::derived_from<Object> T>
    std::shared_ptr<T> makeTestObject(const std::string& kind)
    {
        return std::dynamic_pointer_cast<T>(objectCatalog().make(kind));
    }

    template <std::derived_from<Object> T>
    std::shared_ptr<T> makeTestObject(Network& network, const std::string& kind)
    {
        return std::dynamic_pointer_cast<T>(network.makeObject(kind));
    }
}
