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

// There is no pragma once on this include so the LOG_SOURCE can be adjusted as needed during
// inclusion.

/** @file
 * All the pre-processor macros used in the Clypsalot namespace isolated to a single file.
 * The Clypsalot library should not pollute the pre-processor for users of the library so this
 * file must never be included in a header file.
 */

/**
 * @brief A convience macro for automatically giving the file and line data to the \ref fatalError() function.
 */
#define FATAL_ERROR(message) Clypsalot::fatalError(message, __FILE__, __LINE__)

// TODO There has got to be a better way to avoid having to specify the return type
// when invoking threadQueueCall()
#define THREAD_CALL(lambda) Clypsalot::threadQueueCall(std::function([&] lambda))
