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

        ObjectStateChangedEvent(const SharedObject& sender, const ObjectState previous, const ObjectState current);
    };

    struct ObjectStoppedEvent : public ObjectEvent
    {
        ObjectStoppedEvent(const SharedObject& sender);
    };

    class Object : public Lockable, public Eventful, public std::enable_shared_from_this<Object>
    {
        ObjectState currentState = ObjectState::initializing;

        void state(const ObjectState newState);

        protected:
        std::vector<OutputPort*> outputPorts;
        std::vector<InputPort*> inputPorts;

        void fault(const std::string& message);
        virtual void handleInit(const ObjectConfig& config);
        virtual void handleConfigure(const ObjectConfig& config);
        void shutdown();
        bool hasOutput(const size_t number) noexcept;
        bool hasOutput(const std::string& name) noexcept;
        OutputPort& output(const size_t number);
        OutputPort& output(const std::string& name);
        OutputPort& addOutput(OutputPort* output);

        template <typename T>
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

        bool hasInput(const size_t number) noexcept;
        bool hasInput(const std::string& name) noexcept;
        InputPort& input(const size_t number);
        InputPort& input(const std::string& name);
        InputPort& addInput(InputPort* input);

        template <typename T>
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
        Object();
        Object(const Object&) = delete;
        virtual ~Object();
        void operator=(const Object&) = delete;
        virtual const std::string& kind() noexcept = 0;
        ObjectState state() const noexcept;
        void init(const ObjectConfig& config = {});
        void configure(const ObjectConfig& config = {});
        void stop();
    };

    bool validateStateChange(const ObjectState oldState, const ObjectState newState) noexcept;
    std::string asString(const ObjectState state) noexcept;
    std::ostream& operator<<(std::ostream& os, const ObjectState state) noexcept;
    std::string formatStateChange(const ObjectState oldState, const ObjectState newState) noexcept;
    std::string formatStateChange(const ObjectStateChangedEvent& event) noexcept;
    std::ostream& operator<<(std::ostream& os, const ObjectStateChangedEvent event) noexcept;
}
