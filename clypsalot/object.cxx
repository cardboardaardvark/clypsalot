#include <cassert>

#include <clypsalot/error.hxx>
#include <clypsalot/logging.hxx>
#include <clypsalot/object.hxx>
#include <clypsalot/macros.hxx>
#include <clypsalot/util.hxx>

/// @file
namespace Clypsalot
{
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

    ObjectEvent::ObjectEvent(const SharedObject& sender) :
        Event(),
        object(sender)
    { }

    ObjectFaultedEvent::ObjectFaultedEvent(const SharedObject& sender, const std::string& reason) :
        ObjectEvent(sender),
        message(reason)
    { }

    ObjectShutdownEvent::ObjectShutdownEvent(const SharedObject& sender) :
        ObjectEvent(sender)
    { }

    ObjectStateChangedEvent::ObjectStateChangedEvent(const SharedObject& sender, const ObjectState previous, const ObjectState current) :
        ObjectEvent(sender),
        oldState(previous),
        newState(current)
    { }

    ObjectStoppedEvent::ObjectStoppedEvent(const SharedObject& sender) :
        ObjectEvent(sender)
    { }

    Object::Object()
    {
        std::unique_lock lock(mutex);

        events->add(objectEvents);
    }

    Object::~Object()
    { }

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
        LOGGER(trace, "Object changed state: ", formatStateChange(oldState, newState));
        events->send(ObjectStateChangedEvent(shared_from_this(), oldState, currentState));
    }

    void Object::fault(const std::string& message)
    {
        assert(mutex.haveLock());

        LOGGER(error, "Object faulted: ", message);
        state(ObjectState::faulted);
        events->send(ObjectFaultedEvent(shared_from_this(), message));
        shutdown();
    }

    void Object::init(const ObjectConfig& config)
    {
        assert(mutex.haveLock());

        LOGGER(trace, "Initializing object");

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
            FATAL_ERROR("Unknown exception thrown");
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

        LOGGER(trace, "Configuring object");

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
            FATAL_ERROR("Unknown exception thrown");
        }
    }

    void Object::handleConfigure(const ObjectConfig&)
    {
        assert(mutex.haveLock());
        assert(currentState == ObjectState::configuring);
    }

    void Object::stop()
    {
        assert(mutex.haveLock());

        LOGGER(trace, "Stopping object");

        if (currentState != ObjectState::paused)
        {
            objectStateError(currentState);
        }

        state(ObjectState::stopped);
        events->send(ObjectStoppedEvent(shared_from_this()));
        shutdown();
    }

    void Object::shutdown()
    {
        assert(mutex.haveLock());

        LOGGER(trace, "Shutting down object");

        if (currentState != ObjectState::faulted && currentState != ObjectState::stopped)
        {
            objectStateError(currentState);
        }

        events->send(ObjectShutdownEvent(shared_from_this()));
    }

    bool validateStateChange(const ObjectState oldState, const ObjectState newState) noexcept
    {
        switch (oldState)
        {
            case ObjectState::configuring: if (newState == ObjectState::paused) return true; break;
            case ObjectState::faulted: return true;
            case ObjectState::initializing: if (newState == ObjectState::configuring) return true; break;
            case ObjectState::paused:
                switch (newState)
                {
                    case ObjectState::configuring: return false;
                    case ObjectState::faulted: return false;
                    case ObjectState::initializing: return false;
                    case ObjectState::paused: return false;
                    case ObjectState::stopped: return true;
                }

            case ObjectState::stopped: return false;
        }

        return false;
    }

    std::string asString(const ObjectState state) noexcept
    {
        switch (state)
        {
            case ObjectState::configuring: return "configuring";
            case ObjectState::faulted: return "faulted";
            case ObjectState::initializing: return "initializing";
            case ObjectState::paused: return "paused";
            case ObjectState::stopped: return "stopped";
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
