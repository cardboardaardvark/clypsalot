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

#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <typeindex>
#include <vector>

#include <clypsalot/forward.hxx>
#include <clypsalot/thread.hxx>

/// @file
namespace Clypsalot
{
    /**
     * @brief Base class for all events that will be sent.
     *
     * All events must be copyable as they could be passed between threads and placed into queues
     * for delivery. This class must be inherited by a subclass.
     */
    class Event
    {
        protected:
        Event() = default;

        public:
        virtual ~Event() = default;
    };

    using EventTypeList = std::initializer_list<const std::type_info*>;

    /**
     * @brief A subscription for events.
     *
     * When subscribing to an event an instance of this class is returned inside a shared_ptr. The
     * subscriber will continue to receive events on its event handler for as long as this
     * subscription object is alive. To cancel the event subscription either let the shared_ptr
     * go out of scope or assign null_ptr to it.
     */
    class Subscription
    {
        const std::weak_ptr<EventSender> weakSender;

        public:
        /// @cond NO_DOCUMENT
        Subscription(const std::shared_ptr<EventSender>& sender);
        /// @endcond
        bool valid() const noexcept;
    };

    struct SubscriberBase
    {
        const std::weak_ptr<Subscription> weakSubscription;

        SubscriberBase(const std::shared_ptr<Subscription>& subscription);
        virtual ~SubscriberBase() = default;
        virtual void send(const Event& event) = 0;
    };

    template <typename T>
    struct Subscriber : SubscriberBase
    {
        using EventType = T;
        using Handler = std::function<void (const EventType&)>;

        const Handler handler;

        Subscriber(const std::shared_ptr<Subscription>& subscription, const Handler& eventHandler) :
            SubscriberBase(subscription),
            handler(eventHandler)
        { }

        virtual void send(const Event& event)
        {
            handler(dynamic_cast<const EventType&>(event));
        }
    };

    /**
     * @brief Manage event subscriptions and send events to the subscribers.
     */
    class EventSender : Lockable, public std::enable_shared_from_this<EventSender>
    {
        static std::map<std::type_index, std::string> eventNames;
        static Mutex eventNamesMutex;
        std::map<std::type_index, std::vector<SubscriberBase*>> subscribers;

        static std::string eventName(const std::type_index& type);
        void _add(const std::type_info& type);
        void _subscribe(const std::type_info& type, SubscriberBase* subscriber);
        void _cleanupSubscribers() noexcept;

        public:
        ~EventSender();
        void cleanupSubscribers() noexcept;
        void send(const Event& event);
        /// Because register is a keyword
        void add(const EventTypeList& events);
        /**
         * @brief Register an event so it can be sent later.
         *
         * Events must be registered with the event sender before they can be sent. Attempting
         * to send an event that is not registered will result in an exception being thrown by
         * the send() method.
         *
         * @throws RuntimeError Event is already registered with sender
         */
        template <std::derived_from<Event> T>
        void add()
        {
            std::unique_lock lock(mutex);
            const auto& type = typeid(T);
            _add(type);
        }

        /**
         * @brief subscribe Create a subscription to an event.
         * @param handler A std::function that will be invoked to receive the event.
         * @return An instance of a Subscription
         * @throws RuntimeError Event type is not registered with sender
         */
        template <std::derived_from<Event> T>
        [[nodiscard]] std::shared_ptr<Subscription> subscribe(const Subscriber<T>::Handler& handler)
        {
            auto subscription = std::make_shared<Subscription>(shared_from_this());
            auto subscriber = new Subscriber<T>(subscription, handler);

            try {
                _subscribe(typeid(T), subscriber);
            }
            catch (...)
            {
                delete subscriber;
                throw;
            }

            return subscription;
        }
    };
}