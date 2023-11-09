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

#include <functional>

#include <clypsalot/catalog.hxx>
#include <clypsalot/error.hxx>
#include <clypsalot/logger.hxx>
#include <clypsalot/network.hxx>
#include <clypsalot/util.hxx>

using namespace std::placeholders;

namespace Clypsalot
{
    ManagedObject::ManagedObject(const SharedObject& object) :
        object(object)
    { }

    Network::Network()
    {
        messages->registerHandler<ObjectShutdownEvent>(std::bind(&Network::handleObjectEvent, this, _1));
    }

    Network::~Network() noexcept
    {
        messages = nullptr;

        std::scoped_lock lock(mutex);
        _stop();
    }

    bool Network::shouldStop() const noexcept
    {
        assert(mutex.haveLock());

        if (waitForShutdown.size() == 0) return false;

        for (const auto& [object, waiting] : waitForShutdown)
        {
            if (waiting) return false;
        }

        return true;
    }

    void Network::recordWaitForShutdown(const SharedObject& object, std::map<SharedObject, bool>& seenObjects) noexcept
    {
        assert(mutex.haveLock());

        std::unique_lock lock(*object);
        auto objectState = object->state();
        std::vector<SharedObject> checkObjects;

        LOGGER(trace, "Recording if ", *object, " is stopped: ", object->state());

        waitForShutdown[object] = ! objectIsShutdown(objectState);

        for (const auto port : object->outputs())
        {
            for (const auto link : port->links())
            {
                auto checkObject = link->to.parent.shared_from_this();

                if (! seenObjects.contains(checkObject))
                {
                    checkObjects.push_back(checkObject);
                    seenObjects[checkObject] = true;
                }
            }
        }

        lock.unlock();

        for (const auto& nextObject : checkObjects)
        {
            recordWaitForShutdown(nextObject, seenObjects);
        }
    }

    void Network::handleObjectEvent(const ObjectShutdownEvent& event)
    {
        std::scoped_lock lock(mutex);
        std::map<SharedObject, bool> seenObjects;

        if (! running)
        {
            LOGGER(trace, "Skipping handling ObjectShutdownEvent because the network is not running");
            return;
        }

        LOGGER(trace, *event.object, " shutdown");

        recordWaitForShutdown(event.object, seenObjects);

        if (shouldStop())
        {
            LOGGER(debug, "Network needs to stop");
            stop();
        }
    }

    bool Network::_hasObject(const SharedObject& object)
    {
        assert(mutex.haveLock());

        for (const auto& managed : managedObjects)
        {
            if (managed.object == object) return true;
        }

        return false;
    }

    void Network::_addObject(const SharedObject& object)
    {
        assert(mutex.haveLock());

        if (_hasObject(object)) throw RuntimeError(makeString("Object is already registered with network: ", *object));

        ManagedObject managed(object);

        managed.subscribe<ObjectShutdownEvent>(messages);

        managedObjects.push_back(managed);
    }

    SharedObject Network::makeObject(const std::string& kind)
    {
        std::scoped_lock lock(mutex);

        auto object = objectCatalog().make(kind);
        _addObject(object);
        return object;
    }

    void Network::_start()
    {
        assert(mutex.haveLock());

        if (running) return;

        for (const auto& managed : managedObjects)
        {
            std::scoped_lock objectLock(*managed.object);
            startObject(managed.object);
        }

        running = true;
        condVar.notify_all();
    }

    void Network::start()
    {
        std::scoped_lock lock(mutex);
        _start();
    }

    void Network::run()
    {
        std::unique_lock lock(mutex);

        _start();

        condVar.wait(lock, [this]
        {
            if (! running) return true;
            return false;
        });
    }

    void Network::_stop()
    {
        assert(mutex.haveLock());

        if (! running) return;

        for (const auto& managed : managedObjects)
        {
            LOGGER(trace, "Stopping object: ", *managed.object);
            std::scoped_lock objectLock(*managed.object);
            stopObject(managed.object);
        }

        running = false;
        condVar.notify_all();
    }

    void Network::stop()
    {
        std::scoped_lock lock(mutex);
        _stop();
    }
}
