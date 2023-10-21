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

#include <cassert>

#include <clypsalot/error.hxx>
#include <clypsalot/event.hxx>
#include <clypsalot/logging.hxx>
#include <clypsalot/macros.hxx>
#include <clypsalot/util.hxx>

namespace Clypsalot
{
    std::map<std::type_index, std::string> EventSender::eventNames = {};
    Mutex EventSender::eventNamesMutex = {};

    [[noreturn]] static void noRegisteredEventError(const std::type_info& type)
    {
        throw RuntimeError(makeString("Event type is not registered with sender: ", typeName(type)));
    }

    /// @cond NO_DOCUMENT
    Subscription::Subscription(const std::shared_ptr<EventSender>& sender) :
        weakSender(sender)
    { }
    /// @endcond

    SubscriberBase::SubscriberBase(const std::shared_ptr<Clypsalot::Subscription>& subscription) :
        weakSubscription(subscription)
    { }

    /**
     * @brief Identify if the event sender associated with the subscription is still alive.
     * @return True if the event sender exists or false otherwise.
     */
    bool Subscription::valid() const noexcept
    {
        return weakSender.lock() != nullptr;
    }

    EventSender::~EventSender()
    {
        for (const auto& [type, eventSubscribers] : subscribers)
        {
            for (const auto& subscriber : eventSubscribers)
            {
                delete subscriber;
            }
        }
    }

    std::string EventSender::eventName(const std::type_index& type)
    {
        std::unique_lock lock(eventNamesMutex);
        return eventNames.at(type);
    }

    void EventSender::_add(const std::type_info& type)
    {
        assert(mutex.haveLock());

        const auto eventName = typeName(type);
        LOGGER(trace, "Adding event: ", eventName);

        const auto& [ iterator, result ] = subscribers.emplace(type, 0);

        if (! result) {
            throw RuntimeError(makeString("Event is already registered with sender: ", eventName));
        }

        std::unique_lock lock(eventNamesMutex);
        eventNames[type] = eventName;
    }

    void EventSender::add(const EventTypeList& events)
    {
        std::unique_lock lock(mutex);

        for(const auto event : events)
        {
            _add(*event);
        }
    }

    void EventSender::_subscribe(const std::type_info& type, SubscriberBase* subscriber)
    {
        std::unique_lock lock(mutex);

        if (! subscribers.contains(type))
        {
            noRegisteredEventError(type);
        }

        auto& eventSubscribers = subscribers.at(type);
        auto subscription = std::make_shared<Subscription>(shared_from_this());
        eventSubscribers.push_back(subscriber);
    }

    void EventSender::_cleanupSubscribers() noexcept
    {
        assert(mutex.haveLock());

        for (auto& [type, eventSubscribers] : subscribers)
        {
            for (auto subscriber = eventSubscribers.begin(); subscriber != eventSubscribers.end();)
            {
                if (! (*subscriber)->weakSubscription.lock())
                {
                    LOGGER(trace, "Removing dead subscriber from ", eventName(type), " event");
                    subscriber = eventSubscribers.erase(subscriber);
                    delete *subscriber;
                }
                else
                {
                    subscriber++;
                }
            }
        }
    }

    /**
     * @brief Immediately remove any stale subscriptions.
     */
    void EventSender::cleanupSubscribers() noexcept
    {
        std::unique_lock lock(mutex);
        _cleanupSubscribers();
    }

    /**
     * @brief Send an event to all subscribers.
     * @param event The event to send.
     *
     * This method sends an event serially to each subscriber in the order that the subscriptions
     * happened. The event handlers for the subscribers will be invoked in the same thread that
     * has called this method.
     */
    void EventSender::send(const Event& event)
    {
        const auto& type = typeid(event);

        std::unique_lock lock(mutex);

        if (! subscribers.contains(type))
        {
            noRegisteredEventError(type);
        }

        _cleanupSubscribers();

        auto& eventSubscribers = subscribers.at(type);

        for (const auto subscriber : eventSubscribers)
        {
            auto subscription = subscriber->weakSubscription.lock();

            if (subscription == nullptr)
            {
                LOGGER(trace, "Skipping dead subscriber for ", eventName(type), " event");
                continue;
            }

            subscriber->send(event);
        }
    }
}
