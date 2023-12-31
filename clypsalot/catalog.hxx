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

#include <map>
#include <vector>

#include <clypsalot/event.hxx>
#include <clypsalot/object.hxx>
#include <clypsalot/port.hxx>
#include <clypsalot/thread.hxx>

namespace Clypsalot
{
    struct PortTypeCatalogEntryAddedEvent : public Event
    {
        const PortTypeDescriptor& entry;

        PortTypeCatalogEntryAddedEvent(const PortTypeDescriptor& in_entry);
    };

    class PortTypeCatalog : public Lockable, public Eventful
    {
        std::map<std::string, const PortTypeDescriptor*> m_catalogEntries;

        public:
        PortTypeCatalog();
        PortTypeCatalog(const PortTypeCatalog&) = delete;
        ~PortTypeCatalog() = default;
        void operator=(const PortTypeCatalog&) = delete;
        void add(const PortTypeDescriptor& descriptor);
        std::vector<std::string> names() const noexcept;
        const PortType& instance(const std::string& name) const;
        const PortTypeDescriptor& descriptor(const std::string& name) const;
    };

    struct ObjectCatalogEntryAddedEvent : Event
    {
        const ObjectDescriptor& entry;

        ObjectCatalogEntryAddedEvent(const ObjectDescriptor& in_entry);
    };

    class ObjectCatalog : public Lockable, public Eventful
    {
        std::map<std::string, const ObjectDescriptor*> catalogEntries;

        public:
        ObjectCatalog();
        ObjectCatalog(const ObjectCatalog&) = delete;
        ~ObjectCatalog() = default;
        void operator=(const ObjectCatalog&) = delete;
        void add(const ObjectDescriptor& descriptor);
        std::vector<std::string> kinds() const noexcept;
        SharedObject make(const std::string& kind) const;
    };

    PortTypeCatalog& portTypeCatalog();
    ObjectCatalog& objectCatalog();
}
