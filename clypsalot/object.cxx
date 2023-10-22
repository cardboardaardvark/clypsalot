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

    Object::Object() :
        id(nextObjectId++)
    {
        LOGGER(debug, "Constructing object ", *this);

        std::unique_lock lock(mutex);

        events->add(objectEvents);
    }

    Object::~Object() noexcept
    {
        LOGGER(debug, "Destroying object ", *this);

        std::unique_lock lock(*this);

        if (! objectIsPreparing(*this))
        {
            stopObject(*this);
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
        LOGGER(trace, "Object changed state: ", formatStateChange(oldState, newState));
        events->send(ObjectStateChangedEvent(*this, oldState, currentState));
        condVar.notify_all();
    }

    void Object::wait(const std::function<bool ()> tester)
    {
        assert(haveLock());

        condVar.wait(mutex, tester);
    }

    void Object::fault(const std::string& message)
    {
        assert(mutex.haveLock());

        LOGGER(error, "Object faulted: ", message);
        state(ObjectState::faulted);
        events->send(ObjectFaultedEvent(*this, message));
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

        if (objectIsShutdown(*this))
        {
            return;
        }

        LOGGER(debug, "Stopping object");

        if (objectIsBusy(*this))
        {
            LOGGER(error, "Attempt to stop object ", *this, " that was busy");
            objectStateError(currentState);
        }

        state(ObjectState::stopped);
        events->send(ObjectStoppedEvent(*this));
        shutdown();
    }

    void Object::shutdown()
    {
        assert(mutex.haveLock());

        LOGGER(trace, "Shutting down object");

        if (! objectIsShutdown(*this))
        {
            LOGGER(error, "Attempt to shutdown object ", *this, " that was not in shutdown states");
            objectStateError(currentState);
        }

        events->send(ObjectShutdownEvent(*this));
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
            if (output->name() == name)
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
            if (output->name() == name)
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

        LOGGER(trace, "Adding output: ", output->name());

        if (hasOutput(output->name()))
        {
            throw KeyError(makeString("Duplicate output port name: ", output->name()), output->name());
        }

        outputPorts.push_back(output);
        return *output;
    }

    bool Object::hasInput(const std::string& name) noexcept
    {
        assert(mutex.haveLock());

        for (const auto& input : inputPorts)
        {
            if (input->name() == name)
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
            if (input->name() == name)
            {
                return *input;
            }
        }

        throw KeyError(makeString("No such input port: ", name), name);
    }

    InputPort& Object::addInput(InputPort* input)
    {
        assert(mutex.haveLock());

        LOGGER(trace, "Adding input: ", input->name());

        if (hasInput(input->name()))
        {
            throw KeyError(makeString("Duplicate input port name: ", input->name()), input->name());
        }

        inputPorts.push_back(input);
        return *input;
    }

    bool objectIsShutdown(const Object& object) noexcept
    {
        assert(object.haveLock());

        const auto state = object.state();

        switch (state)
        {
            case ObjectState::configuring: return false;
            case ObjectState::faulted: return true;
            case ObjectState::initializing: return false;
            case ObjectState::paused: return false;
            case ObjectState::stopped: return true;
        }

        FATAL_ERROR(makeString("Unhandled object state:", static_cast<int>(state)));
    }

    bool objectIsBusy(const Object& object) noexcept
    {
        assert(object.haveLock());

        const auto state = object.state();

        switch (state)
        {
            case ObjectState::configuring: return false;
            case ObjectState::faulted: return false;
            case ObjectState::initializing: return false;
            case ObjectState::paused: return false;
            case ObjectState::stopped: return false;
        }

        FATAL_ERROR(makeString("Unhandled object state:", static_cast<int>(state)));
    }

    bool objectIsPreparing(const Object& object) noexcept
    {
        assert(object.haveLock());

        const auto state = object.state();

        switch (state)
        {
            case ObjectState::configuring: return true;
            case ObjectState::faulted: return false;
            case ObjectState::initializing: return true;
            case ObjectState::paused: return false;
            case ObjectState::stopped: return false;
        }

        FATAL_ERROR(makeString("Unhandled object state:", static_cast<int>(state)));
    }

    void stopObject(Object& object)
    {
        assert(object.haveLock());

        object.wait([&object]
        {
            LOGGER(trace, "Checking if object is busy; state=", object.state());
            return ! objectIsBusy(object);
        });

        object.stop();
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
