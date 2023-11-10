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
#include <functional>
#include <list>
#include <typeindex>

#include <clypsalot/thread.hxx>

namespace Clypsalot
{
    struct Message
    {
        public:
        Message() = default;
        virtual ~Message() = default;
    };

    class MessageProcessor : protected Lockable
    {
        public:
        using BaseHandler = std::function<void (const Message&)>;

        private:
        std::condition_variable_any m_condVar;
        bool m_receiving = true;
        bool m_processing = false;
        std::list<const Message *> m_queue;
        std::map<std::type_index, BaseHandler> m_handlers;

        void _registerHandler(const std::type_info& type, BaseHandler handler);
        void process() noexcept;

        public:
        MessageProcessor() = default;
        MessageProcessor(const MessageProcessor&) = delete;
        ~MessageProcessor();
        void operator=(const MessageProcessor&) = delete;
        bool registered(const std::type_info& type);
        bool receive(const Message* message);

        template <std::derived_from<Message> T>
        void registerHandler(std::function<void (const T&)> handler)
        {
            _registerHandler(typeid(T), [handler](const Message& baseMessage)
            {
                handler(dynamic_cast<const T&>(baseMessage));
            });
        }
    };
}
