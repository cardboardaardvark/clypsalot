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

#include <clypsalot/port.hxx>

#include "test/lib/test.hxx"
#include "test/module/object.hxx"
#include "test/module/port.hxx"

using namespace Clypsalot;

TEST_MAIN_FUNCTION

BOOST_AUTO_TEST_CASE(link_function)
{
    auto object1 = makeTestObject<TestObject>();
    auto object2 = makeTestObject<TestObject>();
    std::unique_lock object1Lock(*object1);
    std::unique_lock object2Lock(*object2);

    auto& output = object1->publicAddOutput<MTestOutputPort>("output");
    auto& input = object2->publicAddInput<MTestInputPort>("input");
    BOOST_CHECK(output.links().size() == 0);
    BOOST_CHECK(input.links().size() == 0);

    object1->configure();
    object2->configure();
    auto& link = linkPorts(output, input);
    BOOST_CHECK(output.links().size() == 1);
    BOOST_CHECK(*output.links().at(0) == link);
    BOOST_CHECK(input.links().size() == 1);
    BOOST_CHECK(*input.links().at(0) == link);

    unlinkPorts(output, input);
    object1->stop();
    object2->stop();
}

BOOST_AUTO_TEST_CASE(unlink_function)
{
    auto object1 = makeTestObject<TestObject>();
    auto object2 = makeTestObject<TestObject>();
    std::unique_lock object1Lock(*object1);
    std::unique_lock object2Lock(*object2);

    auto& output = object1->publicAddOutput<MTestOutputPort>("output");
    auto& input = object2->publicAddInput<MTestInputPort>("input");

    object1->configure();
    object2->configure();
    linkPorts(output, input);
    BOOST_CHECK(output.links().size() == 1);
    BOOST_CHECK(input.links().size() == 1);

    unlinkPorts(output, input);
    BOOST_CHECK(output.links().size() == 0);
    BOOST_CHECK(input.links().size() == 0);

    object1->stop();
    object2->stop();
}

BOOST_AUTO_TEST_CASE(readiness)
{
    auto object = makeTestObject<TestObject>();
    std::unique_lock lock(*object);
    auto& output = object->publicAddOutput<MTestOutputPort>("output");
    auto& input = object->publicAddInput<MTestInputPort>("input");

    object->configure();
    BOOST_CHECK(output.ready() == false);
    BOOST_CHECK(input.ready() == false);

    output.setReady(true);
    input.setReady(true);
    BOOST_CHECK(output.ready() == true);
    BOOST_CHECK(input.ready() == true);

    object->stop();
}
