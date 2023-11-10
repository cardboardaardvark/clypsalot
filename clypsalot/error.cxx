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
#include <thread>

#include <clypsalot/error.hxx>
#include <clypsalot/object.hxx>
#include <clypsalot/util.hxx>

/// @file
namespace Clypsalot
{
    /// @cond NO_DOCUMENT
    Error::Error(const std::string& message) :
        m_message(message)
    { }
    /// @endcond

    /**
     * @brief Retrieve the contents of \ref Error::message
     * @return A pointer to a C string version of the error message.
     */
    const char* Error::what() const noexcept
    {
        return m_message.c_str();
    }

    DuplicateLinkError::DuplicateLinkError(const OutputPort& from, const InputPort& to) :
        Error("Ports are already linked"),
        m_from(from),
        m_to(to)
    { }

    ImmutableError::ImmutableError(const std::string& message) :
        Error(message)
    { }

    /// @cond NO_DOCUMENT
    KeyError::KeyError(const std::string& message, const std::string& keyName) :
        Error(message),
        m_key(keyName)
    { }
    /// @endcond

    MutexLockError::MutexLockError(const std::string& message) :
        Error(message)
    { }

    MutexUnlockError::MutexUnlockError(const std::string& message) :
        Error(message)
    { }

    ObjectStateChangeError::ObjectStateChangeError(const SharedObject& object, const ObjectState oldState, const ObjectState newState) :
        Error(makeString("State change is invalid: ", formatStateChange(oldState, newState))),
        m_object(object),
        m_oldState(oldState),
        m_newState(newState)
    { }

    ObjectStateError::ObjectStateError(const SharedObject& object, const ObjectState state, const std::string& message) :
        Error(message),
        m_object(object),
        m_state(state)
    { }

    /// @cond NO_DOCUMENT
    RuntimeError::RuntimeError(const std::string& message) :
        Error(message)
    { }
    /// @endcond

    TypeError::TypeError(const std::string& message) :
        Error(message)
    { }

    UndefinedError::UndefinedError(const std::string& message) :
        Error(message)
    { }

    ValueError::ValueError(const std::string& message) :
        Error(message)
    { }

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
        auto output = makeString("FATAL ERROR ", std::this_thread::get_id());

        if (file == nullptr) {
            output += ": " + message;
        } else {
            output += makeString(" ", file, ":", line, ": ", message);
        }

        output += "\n";

        std::cout << output;
        std::abort();
    }
}
