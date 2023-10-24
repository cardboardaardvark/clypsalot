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
#include <filesystem>
#include <functional>
#include <sstream>
#include <string>
#include <typeinfo>

/// @file
namespace Clypsalot
{
    /**
     * @brief Execute a std::function during destruction.
     *
     * This class is intended to be used to execute a std::function when an instance
     * of it goes out ot scope. The function is stored as a reference so care must be
     * taken to ensure that the registered function always exists alongside the instance.
     */
    class Finally
    {
        using Function = std::function<void ()>;

        const Function& finally;

        public:
        Finally(const Function& finally);
        ~Finally();
    };

    std::string typeName(const std::type_info& type);
    bool anyIsStringType(const std::any& value);
    bool anyToBool(const std::any& value);
    float anyToFloat(const std::any& value);
    int anyToInt(const std::any& value);
    std::filesystem::path anyToPath(const std::any& value);
    size_t anyToSize(const std::any& value);
    std::string anyToString(const std::any& vaue);
    bool stringToBool(const std::string& value);
    float stringToFloat(const std::string& value);
    int stringToInt(const std::string& value);
    size_t stringToSize(const std::string& value);

    ///@cond NO_DOCUMENT
    template <typename Arg>
    void _makeStringAppender(std::stringstream& sstream, const Arg& arg) noexcept {
        sstream << arg;
    }

    template <typename Arg, typename... Args>
    void _makeStringAppender(std::stringstream& sstream, const Arg& arg, const Args&... args) noexcept {
        _makeStringAppender(sstream, arg);
        _makeStringAppender(sstream, args...);
    }
    ///@endcond

    /**
     * @brief Create a string from the variable length arguments given.
     * @param args Any number of values that are compatible with the streams system.
     * @return A string of all the values converted to a string and concatenated together.
     */
    template <typename... Args>
    std::string makeString(const Args&... args) noexcept {
        std::stringstream buffer;
        _makeStringAppender(buffer, args...);
        return buffer.str();
    }
}
