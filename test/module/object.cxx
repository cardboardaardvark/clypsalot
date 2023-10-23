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

#include <cassert>

#include <clypsalot/logging.hxx>
#include <clypsalot/macros.hxx>
#include <clypsalot/property.hxx>
#include <clypsalot/util.hxx>

#include "test/module/object.hxx"
#include "test/module/port.hxx"

namespace Clypsalot
{
    const std::string TestObject::kindName = "Test::Object";
    const std::string ProcessingTestObject::kindName = "Test::ProcessingObject";
    static const std::string processCounterPropertyName = "Test::Process Counter";
    static const std::string maxProcessPropertyName = "Test::Max Process";
    static const PropertyList processingProperties = {
        { processCounterPropertyName, PropertyType::size, false, 0 },
        { maxProcessPropertyName, PropertyType::size, true, nullptr },
    };

    SharedObject TestObject::make()
    {
        return std::make_shared<TestObject>(kindName);
    }

    TestObject::TestObject(const std::string& kind) :
        Object(kind)
    { }

    void TestObject::publicAddProperties(const PropertyList& list)
    {
        assert(mutex.haveLock());

        addProperties(list);
    }

    bool TestObject::process()
    {

        return true;
    }

    SharedObject ProcessingTestObject::make()
    {
        return std::make_shared<ProcessingTestObject>(kindName);
    }

    ProcessingTestObject::ProcessingTestObject(const std::string& kind) :
        TestObject(kind)
    {
        std::unique_lock lock(*this);

        addProperties(processingProperties);

        processCounterProperty = &propertySizeRef(processCounterPropertyName);
        maxProcessProperty = &propertySizeRef(maxProcessPropertyName);

    }

    bool ProcessingTestObject::process()
    {
        assert(haveLock());

        TestObject::process();

        (*processCounterProperty)++;

        LOGGER(trace, "Process counter: ", *processCounterProperty);

        if (*processCounterProperty >= *maxProcessProperty)
        {
            LOGGER(trace, "Reached max process value: ", *maxProcessProperty);
            stop();
        }

        for (const auto port : inputPorts)
        {
            for (auto baseLink : port->links())
            {
                auto link = dynamic_cast<PTestPortLink*>(baseLink);

                assert(link->dirty());
                link->dirty(false);
            }
        }

        for(const auto port : outputPorts)
        {
            for (auto baseLink : port->links())
            {
                auto link = dynamic_cast<PTestPortLink*>(baseLink);

                assert(! link->dirty());
                link->dirty(true);
            }
        }

        return true;
    }
}
