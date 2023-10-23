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
#include "test/module/object.hxx"
#include "test/module/port.hxx"

using namespace Clypsalot;

TEST_MAIN_FUNCTION

BOOST_AUTO_TEST_CASE(readiness)
{
    auto object = makeTestObject<TestObject>();
    std::unique_lock lock(*object);
    auto& input1 = object->publicAddInput<MTestInputPort>("input 1");
    auto& input2 = object->publicAddInput<MTestInputPort>("input 2");
    auto& output1 = object->publicAddOutput<MTestOutputPort>("output 1");
    auto& output2 = object->publicAddOutput<MTestOutputPort>("output 2");

    BOOST_CHECK(input1.ready() == false);
    BOOST_CHECK(input2.ready() == false);
    BOOST_CHECK(output1.ready() == false);
    BOOST_CHECK(output2.ready() == false);
    BOOST_CHECK(object->ready() == false);

    object->configure();
    input1.setReady(true);
    BOOST_CHECK(object->ready() == false);
    output1.setReady(true);
    BOOST_CHECK(object->ready() == false);
    input2.setReady(true);
    BOOST_CHECK(object->ready() == false);
    output2.setReady(true);
    BOOST_CHECK(object->ready() == false);
    object->start();
    BOOST_CHECK(object->ready() == true);
    output2.setReady(false);
    BOOST_CHECK(object->ready() == false);
    output2.setReady(true);
    BOOST_CHECK(object->ready() == true);
    input2.setReady(false);
    BOOST_CHECK(object->ready() == false);

    input2.setReady(true);
    BOOST_CHECK(object->ready() == true);
    LOGGER(verbose, "Stopping test object");
    stopObject(object);
    LOGGER(verbose, "Test object is stopped");
    BOOST_CHECK(object->ready() == false);
    LOGGER(verbose, "Test scope ending");
}