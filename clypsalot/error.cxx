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

#include <cstdlib>
#include <iostream>

#include <clypsalot/error.hxx>
#include <clypsalot/util.hxx>

/// @file
namespace Clypsalot
{
    /**
     * @brief Retrieve the contents of \ref Error::message
     * @return A pointer to a C string version of the error message.
     */
    const char* Error::what() const noexcept
    {
        return message.c_str();
    }

    /**
     * @brief Output a message to the console and then terminate the program.
     * @param message The message to send to the console.
     * @param file The file where the error occured or a default of nullptr.
     * @param line The line number in the file where the error occured or a default of 0.
     *
     * This function writes a message to the console then terminates the program
     * in a way that causes a core file to be generated. If file is not nullptr then the
     * given file and line information is incorporated into the message.
     */
    [[noreturn]] void fatalError(const std::string& message, const char* file, const std::size_t line)
    {
        std::string output("FATAL ERROR");

        if (file == nullptr) {
            output += ": " + message;
        } else {
            output += makeString(" ", file, ":", line, ": ", message);
        }

        std::cout << output << std::endl;
        std::abort();
    }
}
