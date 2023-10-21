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

#include <any>
#include <cstdint>
#include <memory>
#include <string>

#include <clypsalot/event.hxx>
#include <clypsalot/forward.hxx>
#include <clypsalot/thread.hxx>

/// @file
namespace Clypsalot
{
    using ObjectConfig = std::initializer_list<std::tuple<std::string, std::any>>;
    using SharedObject = std::shared_ptr<Object>;

    enum class ObjectState : uint_fast8_t
    {
        configuring,
        faulted,
        initializing,
        paused,
        stopped,
    };

    struct ObjectEvent : Event
    {
        SharedObject object;
        ObjectEvent(const SharedObject& sender);
    };

    struct ObjectFaultedEvent : ObjectEvent
    {
        const std::string message;
        ObjectFaultedEvent(const SharedObject& sender, const std::string& reason);
    };

    struct ObjectShutdownEvent : ObjectEvent
    {
        ObjectShutdownEvent(const SharedObject& sender);
    };

    struct ObjectStateChangedEvent : public ObjectEvent
    {
        ObjectState oldState;
        ObjectState newState;

        ObjectStateChangedEvent(const SharedObject& sender, const ObjectState previous, const ObjectState current);
    };

    struct ObjectStoppedEvent : public ObjectEvent
    {
        ObjectStoppedEvent(const SharedObject& sender);
    };

    class Object : public Lockable, public std::enable_shared_from_this<Object>
    {
        ObjectState currentState = ObjectState::initializing;
        const std::shared_ptr<EventSender> events = std::make_shared<EventSender>();

        void state(const ObjectState newState);

        protected:
        void fault(const std::string& message);
        virtual void handleInit(const ObjectConfig& config);
        virtual void handleConfigure(const ObjectConfig& config);
        void shutdown();

        public:
        Object();
        Object(const Object&) = delete;
        virtual ~Object();
        void operator=(const Object&) = delete;
        ObjectState state() const noexcept;
        void init(const ObjectConfig& config = {});
        void configure(const ObjectConfig& config = {});
        void stop();

        template <std::derived_from<Event> T>
        [[nodiscard]] std::shared_ptr<Subscription> subscribe(const Subscriber<T>::Handler& handler)
        {
            // No lock on the object mutex is needed because the EventSender is thread safe
            // and the events shared_ptr never changes after construction.
            return events->subscribe<T>(handler);
        }
    };

    bool validateStateChange(const ObjectState oldState, const ObjectState newState) noexcept;
    std::string asString(const ObjectState state) noexcept;
    std::ostream& operator<<(std::ostream& os, const ObjectState state) noexcept;
    std::string formatStateChange(const ObjectState oldState, const ObjectState newState) noexcept;
    std::string formatStateChange(const ObjectStateChangedEvent& event) noexcept;
    std::ostream& operator<<(std::ostream& os, const ObjectStateChangedEvent event) noexcept;
}
