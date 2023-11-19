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

#include <clypsalot/error.hxx>
#include <clypsalot/forward.hxx>
#include <clypsalot/message.hxx>
#include <clypsalot/util.hxx>

/// @file
namespace Clypsalot
{
    /**
     * @brief Base class for all events that will be sent.
     *
     * All events must be copyable as they could be passed between threads and placed into queues
     * for delivery. This class must be inherited by a subclass.
     */
    class Event : public Message
    {
        protected:
        Event() = default;

        public:
        ~Event() = default;
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
        const std::weak_ptr<EventSender> m_weakSender;

        public:
        /// @cond NO_DOCUMENT
        Subscription(const std::shared_ptr<EventSender>& sender);
        /// @endcond
        bool valid() const noexcept;
    };

    struct SubscriberBase
    {
        const std::weak_ptr<Subscription> m_weakSubscription;

        SubscriberBase(const std::shared_ptr<Subscription>& subscription);
        virtual ~SubscriberBase() = default;
        virtual void send(const Event& event) = 0;
    };

    template <std::derived_from<Event> T>
    struct HandlerSubscriber : SubscriberBase
    {
        using EventType = T;
        using Handler = typename std::function<void (const EventType&)>;

        const Handler m_handler;

        HandlerSubscriber(const std::shared_ptr<Subscription>& subscription, const Handler& eventHandler) :
            SubscriberBase(subscription),
            m_handler(eventHandler)
        { }

        virtual void send(const Event& event)
        {
            m_handler(dynamic_cast<const EventType&>(event));
        }
    };

    template <std::derived_from<Event> T>
    struct MessageSubscriber : SubscriberBase
    {
        using EventType = T;

        const std::weak_ptr<MessageProcessor> m_weakProcessor;

        MessageSubscriber(const std::shared_ptr<Subscription>& subscription, const std::shared_ptr<MessageProcessor> processor) :
            SubscriberBase(subscription),
            m_weakProcessor(processor)
        { }

        virtual void send(const Event& event)
        {
            auto processor = m_weakProcessor.lock();

            if (! processor) return;

            processor->receive(new T(dynamic_cast<const T&>(event)));
        }
    };

    /**
     * @brief Manage event subscriptions and send events to the subscribers.
     */
    class EventSender : Lockable, public std::enable_shared_from_this<EventSender>
    {
        static std::map<std::type_index, std::string> m_eventNames;
        static Mutex m_eventNamesMutex;
        std::map<std::type_index, std::vector<SubscriberBase*>> m_subscribers;

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
            std::scoped_lock lock(m_mutex);
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
        [[nodiscard]] std::shared_ptr<Subscription> subscribe(const typename HandlerSubscriber<T>::Handler& handler)
        {
            auto subscription = std::make_shared<Subscription>(shared_from_this());
            auto subscriber = new HandlerSubscriber<T>(subscription, handler);

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

        template <std::derived_from<Event> T>
        [[nodiscard]] std::shared_ptr<Subscription> subscribe(const std::shared_ptr<MessageProcessor>& processor)
        {
            const auto& type = typeid(T);

            if (! processor->registered(typeid(T))) throw RuntimeError(makeString("Subscriber does not process ", typeName(type), " messages"));

            auto subscription = std::make_shared<Subscription>(shared_from_this());
            auto subscriber = new MessageSubscriber<T>(subscription, processor);

            try {
                _subscribe(type, subscriber);
            } catch (...)
            {
                delete subscriber;
                throw;
            }

            return subscription;
        }
    };

    class Eventful
    {
        protected:
        const std::shared_ptr<EventSender> m_events = std::make_shared<EventSender>();

        public:
        template <std::derived_from<Event> T>
        [[nodiscard]] std::shared_ptr<Subscription> subscribe(const typename HandlerSubscriber<T>::Handler& handler)
        {
            // No lock on the object mutex is needed because the EventSender is thread safe
            // and the events shared_ptr never changes after construction.
            return m_events->subscribe<T>(handler);
        }

        template <std::derived_from<Event> T>
        [[nodiscard]] std::shared_ptr<Subscription> subscribe(const std::shared_ptr<MessageProcessor>& messages)
        {
            // No lock on the object mutex is needed because the EventSender is thread safe
            // and the events shared_ptr never changes after construction.
            return m_events->subscribe<T>(messages);
        }
    };
}
