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

#include <condition_variable>
#include <map>
#include <vector>

#include <clypsalot/event.hxx>
#include <clypsalot/forward.hxx>
#include <clypsalot/object.hxx>
#include <clypsalot/thread.hxx>

namespace Clypsalot
{
    struct ManagedObject
    {
        SharedObject object;
        std::vector<std::shared_ptr<Subscription>> subscriptions;

        ManagedObject(const SharedObject& object);

        template <std::derived_from<Event> T>
        std::shared_ptr<Subscription> subscribe(const std::shared_ptr<MessageProcessor>& messages)
        {
            auto subscription = object->subscribe<T>(messages);
            subscriptions.push_back(subscription);
            return subscription;
        }
    };

    class Network : RecursiveLockable
    {
        std::shared_ptr<MessageProcessor> messages = std::make_shared<MessageProcessor>();
        std::condition_variable_any condVar;
        std::vector<ManagedObject> managedObjects;
        std::map <SharedObject, bool> waitForShutdown;
        bool running = false;

        void handleObjectEvent(const ObjectShutdownEvent& event);
        void recordWaitForShutdown(const SharedObject& object, std::map<SharedObject, bool>& seenObjects) noexcept;
        bool shouldStop() const noexcept;
        bool _hasObject(const SharedObject& object);
        void _addObject(const SharedObject& object);
        void _start();
        void _stop();

        public:
        Network();
        Network(const Network&) = delete;
        ~Network() noexcept;
        void operator=(const Network&) = delete;
        bool hasObject(const SharedObject& object);
        SharedObject makeObject(const std::string& kind);
        void addObject(const SharedObject& object);
        void start();
        void run();
        void stop();
    };
}
