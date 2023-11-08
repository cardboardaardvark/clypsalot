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

#include <clypsalot/catalog.hxx>
#include <clypsalot/error.hxx>
#include <clypsalot/module.hxx>
#include <clypsalot/util.hxx>

namespace Clypsalot
{
    PortTypeCatalogEntryAddedEvent::PortTypeCatalogEntryAddedEvent(const PortTypeDescriptor& newEntry) :
        Event(),
        entry(newEntry)
    { }

    PortTypeCatalog::PortTypeCatalog()
    {
        events->add<PortTypeCatalogEntryAddedEvent>();
    }

    void PortTypeCatalog::add(const PortTypeDescriptor& descriptor)
    {
        std::scoped_lock lock(*this);

        if (catalogEntries.contains(descriptor.name))
        {
            throw KeyError(makeString("Duplicate port type name: ", descriptor.name), descriptor.name);
        }

        catalogEntries[descriptor.name] = &descriptor;
        events->send(PortTypeCatalogEntryAddedEvent(descriptor));
    }

    std::vector<std::string> PortTypeCatalog::names() const noexcept
    {
        std::scoped_lock lock(*this);
        std::vector<std::string> result;

        result.reserve(catalogEntries.size());

        for (const auto& [name, entry] : catalogEntries)
        {
            result.push_back(name);
        }

        return result;
    }

    const PortType& PortTypeCatalog::instance(const std::string& name) const
    {
        std::scoped_lock lock(*this);

        if (! catalogEntries.contains(name))
        {
            throw KeyError(makeString("No known port type name: ", name), name);
        }

        return catalogEntries.at(name)->instance;
    }

    const PortTypeDescriptor& PortTypeCatalog::descriptor(const std::string& name) const
    {
        std::scoped_lock lock(*this);

        if (! catalogEntries.contains(name)) throw KeyError(makeString("No known type type name: ", name), name);

        return *catalogEntries.at(name);
    }

    ObjectCatalogEntryAddedEvent::ObjectCatalogEntryAddedEvent(const ObjectDescriptor& newEntry) :
        Event(),
        entry(newEntry)
    { }

    ObjectCatalog::ObjectCatalog()
    {
        events->add<ObjectCatalogEntryAddedEvent>();
    }

    void ObjectCatalog::add(const ObjectDescriptor& descriptor)
    {
        std::scoped_lock lock(*this);

        if (catalogEntries.contains(descriptor.kind))
        {
            throw KeyError(makeString("Duplicate object kind: ", descriptor.kind), descriptor.kind);
        }

        catalogEntries[descriptor.kind] = &descriptor;
        events->send(ObjectCatalogEntryAddedEvent(descriptor));
    }

    std::vector<std::string> ObjectCatalog::kinds() const noexcept
    {
        std::scoped_lock lock(*this);
        std::vector<std::string> result;

        result.reserve(catalogEntries.size());

        for(const auto& [kind, descriptor] : catalogEntries)
        {
            result.push_back(kind);
        }

        return result;
    }

    SharedObject ObjectCatalog::make(const std::string& kind) const
    {
        std::scoped_lock lock(*this);

        if (! catalogEntries.contains(kind))
        {
            throw KeyError(makeString("Unknown object kind: ", kind), kind);
        }

        return catalogEntries.at(kind)->make();
    }

    PortTypeCatalog& portTypeCatalog()
    {
        static PortTypeCatalog catalog;
        return catalog;
    }

    ObjectCatalog& objectCatalog()
    {
        static ObjectCatalog catalog;
        return catalog;
    }
}
