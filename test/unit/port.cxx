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
#include <clypsalot/port.hxx>
#include <clypsalot/util.hxx>

#include "test/lib/test.hxx"
#include "test/module/object.hxx"
#include "test/module/port.hxx"

using namespace Clypsalot;

TEST_MAIN_FUNCTION

TEST_CASE(PortLink_equality_operators)
{
    auto object1 = TestObject::make();
    auto object2 = TestObject::make();
    std::unique_lock object1Lock(*object1);
    std::unique_lock object2Lock(*object2);
    auto& output = object1->publicAddOutput<MTestOutputPort>("output");
    auto& input1 = object2->publicAddInput<MTestInputPort>("input 1");
    auto& input2 = object2->publicAddInput<MTestInputPort>("input 2");
    auto& type = MTestPortType::singleton;

    auto link1 = type.makeLink(output, input1);
    auto link2 = type.makeLink(output, input1);
    auto link3 = type.makeLink(output, input2);

    LOGGER(debug, "link1: ", *link1);
    LOGGER(debug, "link2: ", *link2);
    LOGGER(debug, "link3: ", *link3);

    BOOST_CHECK(*link1 == *link1);
    BOOST_CHECK(*link1 == *link2);
    BOOST_CHECK(! (*link1 == *link3));

    BOOST_CHECK(! (*link1 != *link1));
    BOOST_CHECK(! (*link1 != *link2));
    BOOST_CHECK(*link1 != *link3);

    for (auto link : {link1, link2, link3})
    {
        delete link;
    }
}

TEST_CASE(Port_hasLink)
{
    auto object1 = TestObject::make();
    auto object2 = TestObject::make();
    std::unique_lock object1Lock(*object1);
    std::unique_lock object2Lock(*object2);
    auto& output = object1->publicAddOutput<MTestOutputPort>("output");
    auto& input1 = object2->publicAddInput<MTestInputPort>("input 1");
    auto& input2 = object2->publicAddInput<MTestInputPort>("input 2");
    auto& type = MTestPortType::singleton;

    for (auto object : {object1, object2})
    {
        object->configure();
    }

    auto link1 = linkPorts(output, input1);
    auto link2 = type.makeLink(output, input1);
    auto link3 = type.makeLink(output, input2);

    BOOST_CHECK(output.hasLink(link1));
    BOOST_CHECK(!output.hasLink(link2));
    BOOST_CHECK(! output.hasLink(link3));
    BOOST_CHECK(input1.hasLink(link1));
    BOOST_CHECK(! input1.hasLink(link2));
    BOOST_CHECK(! input1.hasLink(link3));

    for(auto link : {link2, link3})
    {
        delete link;
    }
}

TEST_CASE(linkPorts_function_single)
{
    auto object1 = TestObject::make();
    auto object2 = TestObject::make();
    std::unique_lock object1Lock(*object1);
    std::unique_lock object2Lock(*object2);

    auto& output = object1->publicAddOutput<MTestOutputPort>("output");
    auto& input = object2->publicAddInput<MTestInputPort>("input");
    BOOST_CHECK(output.links().size() == 0);
    BOOST_CHECK(input.links().size() == 0);

    object1->configure(); object1->start();
    object2->configure(); object2->start();

    BOOST_CHECK(object1->state() == ObjectState::waiting);
    BOOST_CHECK(object2->state() == ObjectState::waiting);
    auto link = linkPorts(output, input);
    BOOST_CHECK(object1->state() == ObjectState::waiting);
    BOOST_CHECK(object2->state() == ObjectState::waiting);
    BOOST_CHECK(output.links().size() == 1);
    BOOST_CHECK(output.links().at(0) == link);
    BOOST_CHECK(input.links().size() == 1);
    BOOST_CHECK(input.links().at(0) == link);
}

TEST_CASE(linkPorts_function_list)
{
    auto source = TestObject::make();
    auto sink = TestObject::make();
    std::vector<SharedObject> objects;
    std::vector<std::unique_lock<Object>> locks;
    std::vector<Port*> ports;

    objects.push_back(source);
    objects.push_back(sink);

    for (const auto& object : objects)
    {
        locks.emplace_back(*object);
    }

    auto& output1 = source->publicAddOutput<MTestOutputPort>("output1");
    auto& output2 = source->publicAddOutput<MTestOutputPort>("output2");
    auto& input1 = sink->publicAddInput<MTestInputPort>("input1");
    auto& input2 = sink->publicAddInput<MTestInputPort>("input2");

    ports.push_back(&output1);
    ports.push_back(&output2);
    ports.push_back(&input1);
    ports.push_back(&input2);

    for (const auto& object : objects)
    {
        object->configure();
        object->start();

        BOOST_CHECK(object->state() == ObjectState::waiting);
    }

    for (const auto port : ports)
    {
        BOOST_CHECK(port->links().size() == 0);
    }

    auto links = linkPorts(
    {
        { output1, input1 },
        { output2, input2 },
    });

    for (const auto& object : objects)
    {
        BOOST_CHECK(object->state() == ObjectState::waiting);
    }

    BOOST_CHECK(links.size() == 2);
    BOOST_CHECK(source->links().size() == 2);
    BOOST_CHECK(sink->links().size() == 2);

    auto link = links.at(0);
    BOOST_CHECK(link->from == output1);
    BOOST_CHECK(link->to == input1);
    BOOST_CHECK(output1.links().at(0) == link);
    BOOST_CHECK(input1.links().at(0) == link);

    link = links.at(1);
    BOOST_CHECK(link->from == output2);
    BOOST_CHECK(link->to == input2);
    BOOST_CHECK(output2.links().at(0) == link);
    BOOST_CHECK(input2.links().at(0) == link);
}

TEST_CASE(unlinkPorts_function_single)
{
    auto object1 = TestObject::make();
    auto object2 = TestObject::make();
    std::unique_lock object1Lock(*object1);
    std::unique_lock object2Lock(*object2);

    auto& output = object1->publicAddOutput<MTestOutputPort>("output");
    auto& input = object2->publicAddInput<MTestInputPort>("input");

    object1->configure(); object1->start();
    object2->configure(); object2->start();

    linkPorts(output, input);
    BOOST_CHECK(output.links().size() == 1);
    BOOST_CHECK(input.links().size() == 1);

    BOOST_CHECK(object1->state() == ObjectState::waiting);
    BOOST_CHECK(object2->state() == ObjectState::waiting);
    unlinkPorts(output, input);
    BOOST_CHECK(object1->state() == ObjectState::waiting);
    BOOST_CHECK(object2->state() == ObjectState::waiting);
    BOOST_CHECK(output.links().size() == 0);
    BOOST_CHECK(input.links().size() == 0);
}

TEST_CASE(unlinkPorts_function_list)
{
    auto source = TestObject::make();
    auto sink = TestObject::make();
    std::vector<SharedObject> objects;
    std::vector<std::unique_lock<Object>> locks;
    std::vector<Port*> ports;

    objects.push_back(source);
    objects.push_back(sink);

    for (const auto& object : objects)
    {
        locks.emplace_back(*object);
    }

    auto& output1 = source->publicAddOutput<MTestOutputPort>("output1");
    auto& output2 = source->publicAddOutput<MTestOutputPort>("output2");
    auto& input1 = sink->publicAddInput<MTestInputPort>("input1");
    auto& input2 = sink->publicAddInput<MTestInputPort>("input2");

    ports.push_back(&output1);
    ports.push_back(&output2);
    ports.push_back(&input1);
    ports.push_back(&input2);

    for (const auto& object : objects)
    {
        object->configure();
        object->start();

        BOOST_CHECK(object->state() == ObjectState::waiting);
    }

    for (const auto port : ports)
    {
        BOOST_CHECK(port->links().size() == 0);
    }

    linkPorts(output1, input1);
    linkPorts(output2, input2);

    for (const auto port : ports)
    {
        BOOST_CHECK(port->links().size() == 1);
    }

    unlinkPorts(
    {
        { output1, input1 },
        { output2, input2 },
    });

    for (const auto port : ports)
    {
        BOOST_CHECK(port->links().size() == 0);
    }

    for (const auto& object : objects)
    {
        BOOST_CHECK(object->state() == ObjectState::waiting);
    }
}

TEST_CASE(readiness)
{
    auto object = TestObject::make();
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
}
