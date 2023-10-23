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

#include <clypsalot/logging.hxx>
#include <clypsalot/macros.hxx>
#include <clypsalot/util.hxx>

#include "test/module/module.hxx"
#include "test/module/object.hxx"
#include "test/module/port.hxx"

namespace Clypsalot
{
    static const std::initializer_list<PortTypeDescriptor> portTypeDescriptors
    {
        { MTestPortType::typeName, MTestPortType::singleton },
    };

    static const std::initializer_list<ObjectDescriptor> objectDescriptors
    {
        { TestObject::kindName, TestObject::make },
        { ProcessingTestObject::kindName, ProcessingTestObject::make },
    };

    static const ModuleDescriptor moduleDescriptor
    {
        portTypeDescriptors,
        objectDescriptors,
    };

#ifdef LOADABLE_MODULE
    extern "C"
    {
        const ModuleDescriptor* clypsalot_module_descriptor()
        {
            return &moduleDescriptor;
        };
    }
#else
    const ModuleDescriptor* testModuleDescriptor()
    {
        return &moduleDescriptor;
    }
#endif
}
