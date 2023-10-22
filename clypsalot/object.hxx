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
#include <clypsalot/thread.hxx>

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
        stopped,
    };

    struct ObjectEvent : Event
    {
        Object& object;
        ObjectEvent(Object& sender);
    };

    struct ObjectFaultedEvent : ObjectEvent
    {
        const std::string message;
        ObjectFaultedEvent(Object& sender, const std::string& reason);
    };

    struct ObjectShutdownEvent : ObjectEvent
    {
        ObjectShutdownEvent(Object& sender);
    };

    struct ObjectStateChangedEvent : public ObjectEvent
    {
        ObjectState oldState;
        ObjectState newState;

        ObjectStateChangedEvent(Object& sender, const ObjectState previous, const ObjectState current);
    };

    struct ObjectStoppedEvent : public ObjectEvent
    {
        ObjectStoppedEvent(Object& sender);
    };

    class Object : public Lockable, public Eventful, public std::enable_shared_from_this<Object>
    {
        ObjectState currentState = ObjectState::initializing;

        void state(const ObjectState newState);
        void shutdown();

        protected:
        std::condition_variable_any condVar;
        std::vector<OutputPort*> outputPorts;
        std::vector<InputPort*> inputPorts;

        void fault(const std::string& message);
        virtual void handleInit(const ObjectConfig& config);
        virtual void handleConfigure(const ObjectConfig& config);
        OutputPort& addOutput(OutputPort* output);

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

        InputPort& addInput(InputPort* input);

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
        const size_t id;

        Object();
        Object(const Object&) = delete;
        virtual ~Object() noexcept;
        void operator=(const Object&) = delete;
        virtual const std::string& kind() noexcept = 0;
        ObjectState state() const noexcept;
        void wait(const std::function<bool ()> tester);
        void init(const ObjectConfig& config = {});
        void configure(const ObjectConfig& config = {});
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

    bool objectIsShutdown(const Object& object) noexcept;
    bool objectIsBusy(const Object& object) noexcept;
    bool objectIsPreparing(const Object& object) noexcept;
    void stopObject(Object& object);
    bool validateStateChange(const ObjectState oldState, const ObjectState newState) noexcept;
    std::string asString(const Object& object) noexcept;
    std::ostream& operator<<(std::ostream& os, const Object& object) noexcept;
    std::string asString(const ObjectState state) noexcept;
    std::ostream& operator<<(std::ostream& os, const ObjectState state) noexcept;
    std::string formatStateChange(const ObjectState oldState, const ObjectState newState) noexcept;
    std::string formatStateChange(const ObjectStateChangedEvent& event) noexcept;
    std::ostream& operator<<(std::ostream& os, const ObjectStateChangedEvent event) noexcept;
}
