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

#include <map>
#include <string>

#include <clypsalot/property.hxx>
#include <clypsalot/thread.hxx>

#include "test/lib/test.hxx"

using namespace Clypsalot;

struct PropertyHost : public Lockable
{
    std::map<std::string, Property> properties;

    Property& addProperty(const PropertyConfig& in_config)
    {
        const auto& [iterator, result] = properties.try_emplace(in_config.name, *this, in_config);
        return iterator->second;
    }
};

TEST_MAIN_FUNCTION

TEST_CASE(Property_flags)
{
    PropertyHost properties;
    std::lock_guard lock(properties);
    auto& noFlagsProperty = properties.addProperty({"noFlags", PropertyType::size, Property::NoFlags, nullptr });
    auto& mutableFlagProperty = properties.addProperty({"mutableFlag", PropertyType::size, Property::PublicMutable, nullptr });

    BOOST_CHECK(noFlagsProperty.flags() == 0);
    BOOST_CHECK(mutableFlagProperty.flags() & Property::PublicMutable);
    BOOST_CHECK(mutableFlagProperty.hasFlag(Property::PublicMutable));
    BOOST_CHECK(! (mutableFlagProperty.flags() & Property::Configurable));
    BOOST_CHECK(! mutableFlagProperty.hasFlag(Property::Configurable));
}
