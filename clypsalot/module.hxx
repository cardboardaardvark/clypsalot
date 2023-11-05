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

#include <initializer_list>
#include <string>

#include <clypsalot/object.hxx>
#include <clypsalot/port.hxx>

namespace Clypsalot
{
    struct ObjectDescriptor
    {
        const std::string& kind;
        const ObjectConstructor make;
    };

    struct PortTypeDescriptor
    {
        const std::string& name;
        const PortType& instance;
        const OutputPortConstructor makeOutput;
        const InputPortConstructor makeInput;
    };

    struct ModuleDescriptor
    {
        const std::initializer_list<PortTypeDescriptor>& types;
        const std::initializer_list<ObjectDescriptor>& objects;
    };

    void importModule(const ModuleDescriptor* module);
}
