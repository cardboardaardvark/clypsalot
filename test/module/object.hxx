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

#include <clypsalot/object.hxx>

namespace Clypsalot
{
    class TestObject : public Object
    {
        public:
        static const std::string kindName;

        static std::shared_ptr<TestObject> make();
        TestObject(const std::string& kind);
        void publicAddProperties(const PropertyList& list);

        template <std::derived_from<OutputPort> T, typename... Args>
        T& publicAddOutput(Args... args)
        {
            return dynamic_cast<T&>(addOutput<T>(args...));
        }

        template <std::derived_from<InputPort> T, typename... Args>
        T& publicAddInput(Args... args)
        {
            return dynamic_cast<T&>(addInput<T>(args...));
        }

        virtual ObjectProcessResult process() override;
    };

    class ProcessingTestObject : public TestObject
    {
        protected:
        size_t* processCounterProperty = nullptr;
        size_t* maxProcessProperty = nullptr;
        bool done = false;

        public:
        static const std::string kindName;

        static std::shared_ptr<ProcessingTestObject> make();
        ProcessingTestObject(const std::string& kind);
        virtual ObjectProcessResult process() override;
    };
}
