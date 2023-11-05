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
    const std::string ProcessingTestObject::kindName = "Test::Processing Object";
    const std::string FilterTestObject::kindName = "Test::Filter Object";
    static const std::string processCounterPropertyName = "Process Counter";
    static const std::string maxProcessPropertyName = "Max Process";
    static const PropertyList processingProperties = {
        { processCounterPropertyName, PropertyType::size, false, false, false, 0 },
        { maxProcessPropertyName, PropertyType::size, true, false, true, nullptr },
    };

    std::shared_ptr<TestObject> TestObject::make()
    {
        return _makeObject<TestObject>(kindName);
    }

    TestObject::TestObject(const std::string& kind) :
        Object(kind)
    { }

    void TestObject::publicAddProperties(const PropertyList& list)
    {
        assert(mutex.haveLock());

        addProperties(list);
    }

    ObjectProcessResult TestObject::process()
    {

        return ObjectProcessResult::finished;
    }

    std::shared_ptr<ProcessingTestObject> ProcessingTestObject::make()
    {
        return _makeObject<ProcessingTestObject>(kindName);
    }

    ProcessingTestObject::ProcessingTestObject(const std::string& kind) :
        TestObject(kind)
    {
        std::unique_lock lock(*this);

        addProperties(processingProperties);

        userOutputPortTypes[PTestPortType::typeName] = true;
        userInputPortTypes[PTestPortType::typeName] = true;

        processCounterProperty = &propertySizeRef(processCounterPropertyName);
        maxProcessProperty = &propertySizeRef(maxProcessPropertyName);
    }

    ObjectProcessResult ProcessingTestObject::process()
    {
        assert(haveLock());

        TestObject::process();

        if (done)
        {
            OBJECT_LOGGER(trace, "Object is done; returning endOfData");
            return ObjectProcessResult::endOfData;
        }

        (*processCounterProperty)++;

        OBJECT_LOGGER(trace, "Process counter: ", *processCounterProperty);

        if (*maxProcessProperty && *processCounterProperty >= *maxProcessProperty)
        {
            OBJECT_LOGGER(trace, "Reached max process value: ", *maxProcessProperty);
            done = true;
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

        return ObjectProcessResult::finished;
    }

    std::shared_ptr<FilterTestObject> FilterTestObject::make()
    {
        return _makeObject<FilterTestObject>(kindName);
    }

    FilterTestObject::FilterTestObject(const std::string& kind) :
        TestObject(kind)
    {
        std::scoped_lock lock(*this);

        addOutput<MTestOutputPort>("output");
        addInput<MTestInputPort>("input");
    }
}
