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
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include <clypsalot/event.hxx>
#include <clypsalot/forward.hxx>
#include <clypsalot/port.hxx>
#include <clypsalot/property.hxx>
#include <clypsalot/thread.hxx>

#include <clypsalot/logging.hxx>
#include <clypsalot/macros.hxx>
#include <clypsalot/util.hxx>

/// @file
namespace Clypsalot
{
    using ObjectConfig = std::initializer_list<std::tuple<std::string, std::any>>;
    using SharedObject = std::shared_ptr<Object>;
    using ObjectConstructor = std::function<SharedObject ()>;

    enum class ObjectState : uint_fast8_t
    {
        configuring,
        faulted,
        initializing,
        paused,
        waiting,
        scheduled,
        executing,
        stopped,
    };

    enum class ObjectProcessResult : uint_fast8_t
    {
        blocked,
        finished,
        endOfData
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

        ObjectStateChangedEvent(const SharedObject&, const ObjectState previous, const ObjectState current);
    };

    struct ObjectStoppedEvent : public ObjectEvent
    {
        ObjectStoppedEvent(const SharedObject& sender);
    };

    class Object : public Lockable, public Eventful, public std::enable_shared_from_this<Object>
    {
        ObjectState currentState = ObjectState::initializing;

        void state(const ObjectState newState);
        void shutdown();

        protected:
        std::condition_variable_any condVar;
        std::map<std::string, Property> objectProperties;
        std::vector<OutputPort*> outputPorts;
        std::vector<InputPort*> inputPorts;

        bool endOfData() const noexcept;
        void fault(const std::string& message) noexcept;
        virtual ObjectProcessResult process() = 0;
        virtual void handleInit(const ObjectConfig& config);
        virtual void handleConfigure(const ObjectConfig& config);
        virtual void handleEndOfData() noexcept;
        Property& addProperty(const PropertyConfig& config);
        void addProperties(const PropertyList& list);
        size_t& propertySizeRef(const std::string& name);
        OutputPort& addOutput(OutputPort* output);
        InputPort& addInput(InputPort* input);

        template <std::derived_from<OutputPort> T>
        OutputPort& addOutput(const std::string& portName)
        {
            auto output = new T(portName, *this);

            try
            {
                return addOutput(output);
            }
            catch (...)
            {
                delete output;
                throw;
            }
        }

        template <std::derived_from<InputPort> T>
        InputPort& addInput(const std::string& portName)
        {
            auto input = new T(portName, *this);

            try
            {
                return addInput(input);
            }
            catch (...)
            {
                delete input;
                throw;
            }
        }

        public:
        using Id = std::size_t;

        const Id id;
        const std::string& kind;

        Object(const std::string& kind);
        Object(const Object&) = delete;
        virtual ~Object() noexcept;
        void operator=(const Object&) = delete;
        ObjectState state() const noexcept;
        virtual bool ready() const noexcept;
        std::vector<PortLink*> links() const noexcept;
        std::vector<SharedObject> linkedObjects() const noexcept;
        const std::map<std::string, Property>& properties() const noexcept;
        bool hasProperty(const std::string& name) const noexcept;
        Property& property(const std::string& name);
        void wait(const std::function<bool ()> tester);
        void init(const ObjectConfig& config = {});
        void configure(const ObjectConfig& config = {});
        void start();
        void schedule();
        ObjectProcessResult execute();
        void pause();
        void stop();
        const std::vector<OutputPort*>& outputs() const noexcept;
        bool hasOutput(const size_t number) noexcept;
        bool hasOutput(const std::string& name) noexcept;
        OutputPort& output(const size_t number);
        OutputPort& output(const std::string& name);
        const std::vector<InputPort*>& inputs() const noexcept;
        bool hasInput(const size_t number) noexcept;
        bool hasInput(const std::string& name) noexcept;
        InputPort& input(const size_t number);
        InputPort& input(const std::string& name);
    };

    // Stop the object when the shared_ptr goes out of scope if the object is not
    // already shutdown so any subscriptions to the state change, stopped and shutdown
    // events receive the events they'll expect from an object even if the object user did
    // not explicitly shut it down before destruction.
    //
    // This happens in a deleter instead of the destructor because there is no way to
    // get a valid shared_ptr from shared_from_this() in the destructor and creating a new
    // shared_ptr in the destructor would cause the pointer to get deleted twice.
    template <std::derived_from<Object> T>
    void _destroyObject(T* object) noexcept
    {
        LOGGER(trace, "Object deleter invoked for ", *object);
        // By the time this deleter is called the original owning shared_ptr has already
        // given up ownership of the object so it is safe to create a new owning shared_ptr.
        // The shared_ptr has to be created here so shared_from_this() works after calling
        // object->stop() which will cause shared_from_this() to be called many times as a
        // part of sending the events.
        std::shared_ptr<T> resurrectedObject;

        try
        {
            std::unique_lock lock(*object);

            if (! objectIsShutdown(object->state()))
            {
                LOGGER(debug, "Shutting down object in object deleter: ", *object);

                resurrectedObject = std::shared_ptr<T>(object);
                stopObject(object->shared_from_this());
            }

            auto links = object->links();

            if (links.size() > 0)
            {
                LOGGER(debug, "Unlinking object in object deleter: ", *object);

                if (! resurrectedObject) resurrectedObject = std::shared_ptr<T>(object);

                auto linkedObjects = object->linkedObjects();
                std::vector<std::unique_lock<Object>> locks;
                std::vector<std::pair<OutputPort&, InputPort&>> ports;

                locks.reserve(linkedObjects.size());
                ports.reserve(links.size());

                for (const auto link : links)
                {
                    ports.emplace_back(link->from, link->to);
                }

                for (const auto& linked : linkedObjects)
                {
                    LOGGER(debug, "Getting lock on ", *linked);
                    locks.emplace_back(*linked);
                }

                unlinkPorts(ports);
            }
        }
        catch (const std::exception& e)
        {
            LOGGER(error, "Caught exception when destroying object ", *object, ": ", e.what());
        }
        catch (...)
        {
            LOGGER(error, "Caught unknown exception when destroying object ", *object);
        }

        // Have to let the newly created shared_ptr delete the object
        if (! resurrectedObject)
        {
            LOGGER(debug, "_destroyObject(): calling delete() on object pointer");
            delete object;
        }
    }

    // This function is intended to be called by the per object static make() method
    // which passes in any required arguments that specific object class has.
    template <std::derived_from<Object> T = Object, typename... Args>
    std::shared_ptr<T> _makeObject(Args&... args)
    {
        return std::shared_ptr<T>(new T(args...), _destroyObject<T>);
    }

    bool objectIsShutdown(const ObjectState state) noexcept;
    bool objectIsBusy(const ObjectState state) noexcept;
    bool objectIsPreparing(const ObjectState state) noexcept;
    bool pauseObject(const SharedObject& object);
    void startObject(const SharedObject& object);
    void scheduleObject(const SharedObject object);
    void stopObject(const SharedObject& object);
    bool validateStateChange(const ObjectState oldState, const ObjectState newState) noexcept;
    std::string asString(const Object& object) noexcept;
    std::ostream& operator<<(std::ostream& os, const Object& object) noexcept;
    std::string asString(const ObjectState state) noexcept;
    std::ostream& operator<<(std::ostream& os, const ObjectState state) noexcept;
    std::string formatStateChange(const ObjectState oldState, const ObjectState newState) noexcept;
    std::string formatStateChange(const ObjectStateChangedEvent& event) noexcept;
    std::ostream& operator<<(std::ostream& os, const ObjectStateChangedEvent event) noexcept;
}
