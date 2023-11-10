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

#include <clypsalot/error.hxx>
#include <clypsalot/logger.hxx>
#include <clypsalot/macros.hxx>
#include <clypsalot/message.hxx>
#include <clypsalot/util.hxx>

namespace Clypsalot
{
    MessageProcessor::~MessageProcessor()
    {
        LOGGER(trace, "MessageProcessor is being destroyed");

        std::unique_lock lock(m_mutex);

        m_receiving = false;

        m_condVar.wait(lock, [this]
        {
            if (m_processing)
            {
                LOGGER(trace, "Waiting for MessageProcessor to finish executing inside the ThreadQueue");
                return false;
            }

            return true;
        });
    }

    void MessageProcessor::_registerHandler(const std::type_info& type, BaseHandler handler)
    {
        LOGGER(trace, "Registering new message handler for ", typeName(type));

        std::scoped_lock lock(m_mutex);

        if (m_handlers.contains(type)) throw RuntimeError(makeString("Handler already registered for message type: ", typeName(type)));

        m_handlers[type] = handler;
    }

    bool MessageProcessor::registered(const std::type_info& type)
    {
        std::scoped_lock lock(m_mutex);

        return m_handlers.contains(type);
    }

    bool MessageProcessor::receive(const Message* message)
    {
        const auto& type = typeid(*message);
        LOGGER(trace, "Receiving Message: ", typeName(type));

        std::scoped_lock lock(m_mutex);

        if (! m_handlers.contains(type))
        {
            throw RuntimeError(makeString("No handler registered for message type: ", typeName(type)));
        }

        if (! m_receiving)
        {
            LOGGER(trace, "Will not handle message because MessageProcessor is not receiving messages: ", typeName(type));
            return false;
        }

        m_queue.push_back(message);

        if (! m_processing)
        {
            LOGGER(trace, "Submitting job for MessageProcessor to run from ThreadQueue");
            threadQueuePost(std::bind(&MessageProcessor::process, this));
            m_processing = true;
        }

        return true;
    }

    void MessageProcessor::process() noexcept
    {
        LOGGER(trace, "MessageProcessor is running inside ThreadQueue");

        std::unique_lock lock(m_mutex);

        assert(m_processing);

        while (m_queue.size() > 0)
        {
            assert(m_mutex.haveLock());

            LOGGER(trace, "MessageProcessor queue size: ", m_queue.size());

            auto message = m_queue.front();
            auto handler = m_handlers.at(typeid(*message));

            m_queue.pop_front();

            lock.unlock();
            handler(*message);
            delete message;
            lock.lock();
        }

        assert(m_mutex.haveLock());
        m_processing = false;
        m_condVar.notify_all();

        LOGGER(trace, "MessageProcessor is done running inside ThreadQueue");
    }
}
