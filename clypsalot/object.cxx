#include <atomic>
#include <cassert>

#include <clypsalot/error.hxx>
#include <clypsalot/logging.hxx>
#include <clypsalot/macros.hxx>
#include <clypsalot/object.hxx>
#include <clypsalot/port.hxx>
#include <clypsalot/util.hxx>

/// @file
namespace Clypsalot
{
    static std::atomic<size_t> nextObjectId = ATOMIC_VAR_INIT(1);

    static const EventTypeList objectEvents
    {
        &typeid(ObjectFaultedEvent),
        &typeid(ObjectShutdownEvent),
        &typeid(ObjectStateChangedEvent),
        &typeid(ObjectStoppedEvent),
    };

    [[noreturn]] static void objectStateError(const ObjectState currentState)
    {
        throw StateError(makeString("Operation is invalid given current object state: ", currentState));
    }

    ObjectEvent::ObjectEvent(Object& sender) :
        Event(),
        object(sender)
    { }

    ObjectFaultedEvent::ObjectFaultedEvent(Object& sender, const std::string& reason) :
        ObjectEvent(sender),
        message(reason)
    { }

    ObjectShutdownEvent::ObjectShutdownEvent(Object& sender) :
        ObjectEvent(sender)
    { }

    ObjectStateChangedEvent::ObjectStateChangedEvent(Object& sender, const ObjectState previous, const ObjectState current) :
        ObjectEvent(sender),
        oldState(previous),
        newState(current)
    { }

    ObjectStoppedEvent::ObjectStoppedEvent(Object& sender) :
        ObjectEvent(sender)
    { }

    Object::Object(const std::string& kind) :
        id(nextObjectId++),
        kind(kind)
    {
        OBJECT_LOGGER(debug, "Object is being constructed");

        std::unique_lock lock(mutex);

        events->add(objectEvents);
    }

    /**
     * FIXME It is not currently possible to send an ObjectEvent from inside the destructor because
     * the shared_ptr is already gone so when shared_from_this() is called during event construction
     * so shared_from_this() throws a std::bad_weak_ptr exception. Because events could be queued
     * or sent to other threads the object attribute on the event can't safely be a reference.
     *
     * This is a nasty race condition as other things may rely on the ObjectShutdownEvent to track
     * an object. The temporary workaround is to require the object to be shutdown prior to destruction.
     *
     * Another nasty problem is related to links to other objects. Links use object references so
     * they must be removed from the other objects prior to destruction. Locking the mutex of the
     * object being destroyed or any other object from inside the destructor is highly non-optimal
     * because of the entirely non-obvious deadlock possibilities since the locking operation is
     * not immediately clear to the software developer.
     *
     * The temporary solution to that problem is for Port to throw an exception if any links
     * exist when it is destructed. This causes the program to terminate because the destructor
     * is intentionally set to noexcept since throwing exceptions from a destructor is very poor
     * form and leaves the object in a very difficult to define state and will certainly leak memory.
     */
    Object::~Object() noexcept
    {
        OBJECT_LOGGER(debug, "Destroying object");

        if (! objectIsShutdown(currentState))
        {
           FATAL_ERROR(makeString("Attempt to destroy ", *this, " when it was not shutdown"));
        }

        for (const auto output : outputPorts)
        {
            delete output;
        }

        for (const auto input : inputPorts)
        {
            delete input;
        }
    }

    ObjectState Object::state() const noexcept
    {
        assert(mutex.haveLock());
        return currentState;
    }

    void Object::state(const ObjectState newState)
    {
        assert(mutex.haveLock());

        const auto oldState = currentState;

        if (! validateStateChange(oldState, newState))
        {
            throw StateError(makeString("Object state change is invalid: ", formatStateChange(oldState, newState)));
        }

        currentState = newState;
        OBJECT_LOGGER(trace, "Changed state: ", formatStateChange(oldState, newState));
        events->send(ObjectStateChangedEvent(*this, oldState, currentState));
        condVar.notify_all();
    }

    bool Object::ready() const noexcept
    {
        assert(haveLock());

        OBJECT_LOGGER(trace, "Checking readiness; state=", currentState);

        if (currentState != ObjectState::waiting)
        {
            OBJECT_LOGGER(trace, "Not ready because it is not waiting");
            return false;
        }

        for (const auto port : outputPorts)
        {
            if (! port->ready())
            {
                OBJECT_LOGGER(trace, "Port ", *port, " is not ready");
                return false;
            }
        }

        for (const auto port : inputPorts)
        {
            if (! port->ready())
            {
                OBJECT_LOGGER(trace, "Port ", *port, " is not ready");
                return false;
            }
        }

        OBJECT_LOGGER(trace, "Ready");
        return true;
    }

    std::map<SharedObject, bool> Object::linkedObjects() const noexcept
    {
        assert(haveLock());

        std::map<SharedObject, bool> objects;

        for (const auto port : outputPorts)
        {
            for (const auto link : port->links())
            {
                objects[link->to.parent.shared_from_this()] = true;
            }
        }

        for (const auto port : inputPorts)
        {
            for (const auto link : port->links())
            {
                objects[link->from.parent.shared_from_this()] = true;
            }
        }

        return objects;
    }

    void Object::wait(const std::function<bool ()> tester)
    {
        assert(haveLock());

        condVar.wait(mutex, tester);
    }

    void Object::fault(const std::string& message)
    {
        assert(mutex.haveLock());

        try {
            OBJECT_LOGGER(error, "Faulted: ", message);
            state(ObjectState::faulted);
            events->send(ObjectFaultedEvent(*this, message));
            shutdown();
        } catch (std::exception& e)
        {
            FATAL_ERROR(makeString("Error encountered in fault handler: ", e.what()));
            throw;
        }
        catch (...)
        {
            FATAL_ERROR("Unknown exception in fault handler");
        }
    }

    void Object::init(const ObjectConfig& config)
    {
        assert(mutex.haveLock());

        OBJECT_LOGGER(trace, "Initializing");

        try
        {
            if (currentState != ObjectState::initializing)
            {
                objectStateError(currentState);
            }

            handleInit(config);
            state(ObjectState::configuring);
        }
        catch (const std::exception& e)
        {
            fault(e.what());
            throw;
        }
        catch (...)
        {
            FATAL_ERROR("Caught unknown exception");
        }
    }

    void Object::handleInit(const ObjectConfig&)
    {
        assert(mutex.haveLock());
        assert(currentState == ObjectState::initializing);
    }

    void Object::configure(const ObjectConfig& config)
    {
        assert(mutex.haveLock());

        OBJECT_LOGGER(trace, "Configuring");

        try
        {
            if (currentState == ObjectState::initializing)
            {
                init(config);
            }

            if (currentState != ObjectState::configuring)
            {
                objectStateError(currentState);
            }

            handleConfigure(config);
            state(ObjectState::paused);
        }
        catch (const std::exception& e)
        {
            fault(e.what());
            throw;
        }
        catch (...)
        {
            FATAL_ERROR("Caught unknown exception");
        }
    }

    void Object::handleConfigure(const ObjectConfig&)
    {
        assert(mutex.haveLock());
        assert(currentState == ObjectState::configuring);
    }

    Property& Object::addProperty(const PropertyConfig& config)
    {
        assert(haveLock());

        OBJECT_LOGGER(debug, "Adding property: ", config.name);

        if (! objectIsPreparing(currentState))
        {
            objectStateError(currentState);
        }

        const auto& [iterator, result] = objectProperties.try_emplace(
            config.name,
            *this,
            config.name,
            config.type,
            config.publicMutable,
            config.initial
        );

        if (! result)
        {
            throw KeyError(makeString("Duplicate property name: ", config.name), config.name);
        }

        return iterator->second;
    }

    void Object::addProperties(const PropertyList& list)
    {
        assert(haveLock());

        for (const auto& config : list)
        {
            addProperty(config);
        }
    }

    bool Object::hasProperty(const std::string& name) const noexcept
    {
        assert(mutex.haveLock());

        return objectProperties.contains(name);
    }

    Property& Object::property(const std::string& name)
    {
        assert(mutex.haveLock());

        if (! objectProperties.contains(name))
        {
            throw KeyError(makeString("Unknown property name: ", name), name);
        }

        return objectProperties.at(name);
    }

    const std::map<std::string, Property>& Object::properties() const noexcept
    {
        assert(haveLock());

        return objectProperties;
    }

    size_t& Object::propertySizeRef(const std::string& name)
    {
        assert(mutex.haveLock());

        return property(name).sizeRef();
    }

    void Object::start()
    {
        assert(mutex.haveLock());

        try
        {
            state(ObjectState::waiting);
        }
        catch (const std::exception& e)
        {
            fault(e.what());
            throw;
        }
        catch (...)
        {
            FATAL_ERROR("Unknown exception");
        }
    }

    void Object::schedule()
    {
        assert(mutex.haveLock());

        try
        {
            state(ObjectState::scheduled);
        }
        catch (const std::exception& e)
        {
            fault(e.what());
            throw;
        }
        catch (...)
        {
            FATAL_ERROR("Unknown exception");
        }
    }

    bool Object::execute()
    {
        assert(haveLock());

        try
        {
            state(ObjectState::executing);

            const bool finished = process();

            if (finished)
            {
                state(ObjectState::waiting);
            }
            else
            {
                // In the future the process() method could enter a blocked state and finish executing
                // with another job in the thread queue later.
                FATAL_ERROR("Blocked state is not yet implemented");
            }

            return finished;
        }
        catch (const std::exception& e)
        {
            fault(e.what());
            throw;
        }
        catch (...)
        {
            FATAL_ERROR("Unknown exception");
        }
    }

    void Object::pause()
    {
        assert(mutex.haveLock());

        try
        {
            state(ObjectState::paused);
        }
        catch (const std::exception& e)
        {
            fault(e.what());
            throw;
        }
        catch (...)
        {
            FATAL_ERROR("Unknown exception");
        }
    }

    void Object::stop()
    {
        assert(mutex.haveLock());

        OBJECT_LOGGER(debug, "Stopping");

        try
        {
            state(ObjectState::stopped);
            events->send(ObjectStoppedEvent(*this));
            shutdown();
        }
        catch (std::exception& e)
        {
            fault(e.what());
            throw;
        }
        catch (...)
        {
            FATAL_ERROR("Unknown exception");
        }
    }

    void Object::shutdown()
    {
        assert(mutex.haveLock());

        try
        {
            OBJECT_LOGGER(trace, "Shutting down");

            if (! objectIsShutdown(currentState))
            {
                OBJECT_LOGGER(error, "Attempt to shutdown object that was not in shutdown states");
                objectStateError(currentState);
            }

            events->send(ObjectShutdownEvent(*this));
        }
        catch (std::exception& e)
        {
            fault(e.what());
            throw;
        }
        catch (...)
        {
            FATAL_ERROR("Unknown exception");
        }
    }

    const std::vector<OutputPort*>& Object::outputs() const noexcept
    {
        assert(haveLock());

        return outputPorts;
    }

    bool Object::hasOutput(const std::string& name) noexcept
    {
        assert(mutex.haveLock());

        for (const auto& output : outputPorts)
        {
            if (output->name == name)
            {
                return true;
            }
        }

        return false;
    }

    OutputPort& Object::output(const std::string& name)
    {
        assert(mutex.haveLock());

        for(auto& output : outputPorts)
        {
            if (output->name == name)
            {
                return *output;
            }
        }

        throw KeyError(makeString("No such output port: ", name), name);
    }

    const std::vector<InputPort*>& Object::inputs() const noexcept
    {
        assert(mutex.haveLock());

        return inputPorts;
    }

    OutputPort& Object::addOutput(OutputPort* output)
    {
        assert(mutex.haveLock());

        OBJECT_LOGGER(trace, "Adding output: ", output->name);

        if (! objectIsPreparing(currentState))
        {
            objectStateError(currentState);
        }

        if (hasOutput(output->name))
        {
            throw KeyError(makeString("Duplicate output port name: ", output->name), output->name);
        }

        outputPorts.push_back(output);
        return *output;
    }

    bool Object::hasInput(const std::string& name) noexcept
    {
        assert(mutex.haveLock());

        for (const auto& input : inputPorts)
        {
            if (input->name == name)
            {
                return true;
            }
        }

        return false;
    }

    InputPort& Object::input(const std::string& name)
    {
        assert(mutex.haveLock());

        for(auto& input : inputPorts)
        {
            if (input->name == name)
            {
                return *input;
            }
        }

        throw KeyError(makeString("No such input port: ", name), name);
    }

    InputPort& Object::addInput(InputPort* input)
    {
        assert(mutex.haveLock());

        if (! objectIsPreparing(currentState))
        {
            objectStateError(currentState);
        }

        OBJECT_LOGGER(trace, "Adding input: ", input->name);

        if (hasInput(input->name))
        {
            throw KeyError(makeString("Duplicate input port name: ", input->name), input->name);
        }

        inputPorts.push_back(input);
        return *input;
    }

    bool objectIsShutdown(const ObjectState state) noexcept
    {
        switch (state)
        {
            case ObjectState::configuring: return false;
            case ObjectState::executing: return false;
            case ObjectState::faulted: return true;
            case ObjectState::initializing: return false;
            case ObjectState::paused: return false;
            case ObjectState::scheduled: return false;
            case ObjectState::stopped: return true;
            case ObjectState::waiting: return false;
        }

        FATAL_ERROR(makeString("Unhandled object state:", static_cast<int>(state)));
    }

    bool objectIsBusy(const ObjectState state) noexcept
    {
        switch (state)
        {
            case ObjectState::configuring: return false;
            case ObjectState::executing: return true;
            case ObjectState::faulted: return false;
            case ObjectState::initializing: return false;
            case ObjectState::paused: return false;
            case ObjectState::scheduled: return true;
            case ObjectState::stopped: return false;
            case ObjectState::waiting: return false;
        }

        FATAL_ERROR(makeString("Unhandled object state:", static_cast<int>(state)));
    }

    bool objectIsPreparing(const ObjectState state) noexcept
    {
        switch (state)
        {
            case ObjectState::configuring: return true;
            case ObjectState::executing: return false;
            case ObjectState::faulted: return false;
            case ObjectState::initializing: return true;
            case ObjectState::paused: return false;
            case ObjectState::scheduled: return false;
            case ObjectState::stopped: return false;
            case ObjectState::waiting: return false;
        }

        FATAL_ERROR(makeString("Unhandled object state:", static_cast<int>(state)));
    }

    /**
     * @brief Block until the Object is in the paused state.
     * @param object
     * @return True if the object became paused because of this function or false if the object was already paused.
     * @throws StateError if the object is in a state where it can't be paused.
     */
    bool pauseObject(const SharedObject& object)
    {
        assert(object->haveLock());

        bool doPause = false;

        object->wait([&object, &doPause]
        {
            const auto objectState = object->state();

            LOGGER(trace, "Checking if pauseObject() for ", *object, " should stop waiting; state=", objectState);

            if (objectIsPreparing(objectState))
            {
                throw StateError(makeString("Can't pause object that is preparing: ", *object));
            }

            if (objectIsShutdown(objectState))
            {
                throw StateError(makeString("Can't pause object that is shutdown: ", *object));
            }

            if (objectState == ObjectState::paused)
            {
                return true;
            }

            if (objectState == ObjectState::waiting)
            {
                doPause = true;
                return true;
            }

            return false;
        });

        if (doPause)
        {
            object->pause();
        }

        return doPause;
    }

    /**
     * @brief Start the Object and schedule it for execution if it is ready.
     * @param object The object to start.
     * @throws StateError if the object is not in a state where it can be started.
     */
    void startObject(const SharedObject& object)
    {
        assert(object->haveLock());

        object->start();

        if (object->ready())
        {
            scheduleObject(object);
        }
    }

    /**
     * @brief Schedule an Object for execution.
     * @param object The object to schedule.
     * @throws StateError if the object is not in a state where it can be scheduled.
     * @throws Any other exceptions and sets the object to faulted if an error is encountered.
     */
    void scheduleObject(const SharedObject& object)
    {
        assert(object->haveLock());

        object->schedule();

        threadQueuePost([object]
        {
            LOGGER(debug, "Executing ", *object, " from inside the thread queue.");

            std::unique_lock lock(*object);
            const bool finishedExecuting = object->execute();

            if (! finishedExecuting)
            {
                return;
            }

            auto checkObjects = object->linkedObjects();
            lock.unlock();

            for (const auto& [check, _] : checkObjects)
            {
                lock = std::unique_lock(*check);

                if (check->ready())
                {
                    scheduleObject(check);
                }
            }
        });
    }

    void stopObject(const SharedObject& object)
    {
        assert(object->haveLock());

        const auto state = object->state();

        if (objectIsShutdown(state))
        {
            return;
        }

        if (! objectIsPreparing(state))
        {
            pauseObject(object);
        }

        object->stop();
    }

    bool validateStateChange(const ObjectState oldState, const ObjectState newState) noexcept
    {
        // Object can always move into a faulted state unless it's already faulted which
        // makes no sense.
        if (newState == ObjectState::faulted)
        {
            if (oldState == ObjectState::faulted)
            {
                return false;
            }

            return true;
        }

        switch (oldState)
        {
            case ObjectState::configuring: if (newState == ObjectState::paused || objectIsShutdown(newState)) return true; return false;
            case ObjectState::executing: if (newState == ObjectState::waiting) return true; return false;
            case ObjectState::faulted: return false;
            case ObjectState::initializing: if (newState == ObjectState::configuring || objectIsShutdown(newState)) return true; return false;
            case ObjectState::paused:
                switch (newState)
                {
                    case ObjectState::configuring: return false;
                    case ObjectState::executing: return false;
                    case ObjectState::faulted: return true;
                    case ObjectState::initializing: return false;
                    case ObjectState::paused: return false;
                    case ObjectState::scheduled: return false;
                    case ObjectState::stopped: return true;
                    case ObjectState::waiting: return true;
                }
            case ObjectState::scheduled: if (newState == ObjectState::executing) return true; return false;
            case ObjectState::stopped:
                switch (newState)
                {
                    case ObjectState::configuring: return true;
                    case ObjectState::executing: return false;
                    case ObjectState::faulted: return false;
                    case ObjectState::initializing: return true;
                    case ObjectState::paused: return true;
                    case ObjectState::scheduled: return false;
                    case ObjectState::stopped: return false;
                    case ObjectState::waiting: return false;
                }

            case ObjectState::waiting:
                switch (newState)
                {
                    case ObjectState::configuring: return false;
                    case ObjectState::executing: return false;
                    case ObjectState::faulted: return true;
                    case ObjectState::initializing: return false;
                    case ObjectState::paused: return true;
                    case ObjectState::scheduled: return true;
                    case ObjectState::stopped: return false;
                    case ObjectState::waiting: return false;
                }
        }

        FATAL_ERROR(makeString("Unhandled state transition validation: ", formatStateChange(oldState, newState)));
    }

    std::string asString(const Object& object) noexcept
    {
        return std::string("Object #") + std::to_string(object.id);
    }

    std::ostream& operator<<(std::ostream& os, const Object& object) noexcept
    {
        os << asString(object);
        return os;
    }

    std::string asString(const ObjectState state) noexcept
    {
        switch (state)
        {
            case ObjectState::configuring: return "configuring";
            case ObjectState::executing: return "executing";
            case ObjectState::faulted: return "faulted";
            case ObjectState::initializing: return "initializing";
            case ObjectState::paused: return "paused";
            case ObjectState::scheduled: return "scheduled";
            case ObjectState::stopped: return "stopped";
            case ObjectState::waiting: return "waiting";
        }

        FATAL_ERROR(makeString("Unhandled ObjectState: ", static_cast<int>(state)));
    }

    std::ostream& operator<<(std::ostream& os, const ObjectState state) noexcept
    {
        os << asString(state);
        return os;
    }

    std::string formatStateChange(const ObjectState oldState, const ObjectState newState) noexcept
    {
        return makeString(oldState, " -> ", newState);
    }

    std::string formatStateChange(const ObjectStateChangedEvent& event) noexcept
    {
        return formatStateChange(event.oldState, event.newState);
    }

    std::ostream& operator<<(std::ostream& os, const ObjectStateChangedEvent event) noexcept
    {
        os << formatStateChange(event);
        return os;
    }
}
