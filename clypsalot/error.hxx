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

#include <exception>
#include <string>

#include <clypsalot/forward.hxx>

/// @file
namespace Clypsalot
{
    /**
     * @brief The base class for all Clypsalot exceptions
     *
     * All exceptions thrown by the Clypsalot library will derive from this class.
     */
    struct Error : public std::exception
    {
        /**
         * @brief A human readable error message describing what went wrong.
         */
        const std::string m_message;

        /// @cond NO_DOCUMENT
        Error(const std::string& message);
        /// @endcond
        virtual ~Error() = default;
        virtual const char* what() const noexcept override;
    };

    struct DuplicateLinkError : public Error
    {
        const OutputPort& m_from;
        const InputPort& m_to;

        DuplicateLinkError(const OutputPort& from, const InputPort& to);
    };

    struct ImmutableError : public Error
    {
        ImmutableError(const std::string& message);
    };

    /**
     * @brief An exception for handling errors when using a named key.
     */
    struct KeyError : public Error
    {
        /// @brief The key name related to the error.
        const std::string m_key;

        /// @cond NO_DOCUMENT
        KeyError(const std::string& message, const std::string& key);
        /// @endcond
    };

    struct MutexLockError : public Error
    {
        MutexLockError(const std::string& message);
    };

    struct MutexUnlockError : public Error
    {
        MutexUnlockError(const std::string& message);
    };

    struct ObjectStateChangeError : public Error
    {
        const SharedObject m_object;
        const ObjectState m_oldState;
        const ObjectState m_newState;

        ObjectStateChangeError(const SharedObject& object, const ObjectState oldState, const ObjectState newState);
    };

    /**
     * @brief Exception for when an operation is invalid given the current Object state
     * @ref ObjectState "state".
     */
    struct ObjectStateError : public Error
    {
        const SharedObject m_object;
        const ObjectState m_state;

        ObjectStateError(const SharedObject& object, const ObjectState state, const std::string& message);
    };

    /**
     * @brief A generic error for problems that arrise at runtime.
     */
    struct RuntimeError : public Error
    {
        /// @cond NO_DOCUMENT
        RuntimeError(const std::string& message);
        /// @endcond
    };

    struct TypeError : public Error
    {
        TypeError(const std::string& message);
    };

    struct UndefinedError : public Error
    {
        UndefinedError(const std::string& message);
    };

    struct ValueError : public Error
    {
        ValueError(const std::string& message);
    };

    [[noreturn]] void fatalError(const std::string& message, const char* file, const std::size_t line);
}
