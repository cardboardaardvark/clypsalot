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

using namespace Clypsalot;

TEST_MAIN_FUNCTION

static PropertyList propertyList = {
    { "property 1", PropertyType::boolean, true, nullptr },
    { "property 2", PropertyType::string, false, "initial value" },
};

BOOST_AUTO_TEST_CASE(objectProperties)
{
    auto object = makeTestObject<TestObject>();
    std::unique_lock lock(*object);
    const auto& properties = object->properties();
    size_t numChecked = 0;

    BOOST_CHECK(properties.size() == 0);
    object->publicAddProperties(propertyList);
    BOOST_CHECK(! object->hasProperty("property does not exist name"));
    BOOST_CHECK(properties.size() == propertyList.size());

    for (const auto& config : propertyList)
    {
        LOGGER(verbose, "Validating property ", config.name);
        auto& property = object->property(config.name);

        BOOST_CHECK(object->hasProperty(config.name));
        BOOST_CHECK(property.name == config.name);
        BOOST_CHECK(property.type == config.type);
        BOOST_CHECK(property.publicMutable == config.publicMutable);
        BOOST_CHECK(property.defined() != (config.initial.type() == typeid(nullptr)));

        numChecked++;
    }

    BOOST_CHECK(numChecked == propertyList.size());
    BOOST_CHECK(propertyList.size() == 2);

    object->stop();
}
