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

/**
 * @file
 *
 * All forward declarations should be put into this header and the header included when a forward
 * declare is needed instead of spreading the forward declarations around the individual header
 * files.
 */

#pragma once

#include <any>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Clypsalot
{
    class Event;
    class EventSender;
    class InputPort;
    class Lockable;
    class LogEngine;
    enum class LogSeverity : uint_fast8_t;
    struct MessageProcessor;
    struct ModuleDescriptor;
    class Object;
    struct ObjectCatalogEntryAddedEvent;
    struct ObjectDescriptor;
    enum class ObjectState : uint_fast8_t;
    struct ObjectStateChangedEvent;
    class OutputPort;
    class PortLink;
    struct PortType;
    struct PortTypeDescriptor;
    class Property;
    struct PropertyConfig;
    class SharedLockable;
    class Subscription;

    // TODO Figure out if this should go into something like types.h or if it is appropriate here
    using SharedObject = std::shared_ptr<Object>;

    using InputPortConstructor = std::function<InputPort* (const std::string& portName, Object& parent)>;
    using OutputPortConstructor = std::function<OutputPort* (const std::string& name, Object& parent)>;
    using ObjectConfig = std::vector<std::tuple<std::string, std::any>>;
    using ObjectConstructor = std::function<SharedObject ()>;
    using PropertyList = std::initializer_list<PropertyConfig>;

    std::string asString(const LogSeverity severity) noexcept;
    PortLink* linkPorts(OutputPort& output, InputPort& input);
    std::vector<PortLink*> linkPorts(const std::vector<std::pair<OutputPort&, InputPort&>>& portList);
    void scheduleObject(Object&);
    void unlinkPorts(OutputPort& output, InputPort& input);
    void unlinkPorts(const std::vector<std::pair<OutputPort&, InputPort&>>& portList);
}
