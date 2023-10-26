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

#include <clypsalot/logging.hxx>
#include <clypsalot/macros.hxx>
#include <clypsalot/util.hxx>

#include "test/lib/test.hxx"

using namespace Clypsalot;

TEST_MAIN_FUNCTION

// Don't use the text fixture because it will always make this test fail
BOOST_AUTO_TEST_CASE(logging)
{
    BOOST_CHECK(severeLogEvents == 0);
    LOGGER(warn, "Test warning severity log event");
    BOOST_CHECK(severeLogEvents == 1);
    LOGGER(error, "Test error severity log event");
    BOOST_CHECK(severeLogEvents == 2);
}

TEST_CASE(logging_reset)
{
    BOOST_CHECK(severeLogEvents == 0);
}
