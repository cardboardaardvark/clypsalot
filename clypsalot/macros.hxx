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

/// @brief The plain text name of a LogEvent source that will be used by the LOGGER and LLOGGER macros.
#ifndef LOG_SOURCE
#define LOG_SOURCE "Clypsalot"
#endif

/// @brief A convenience wrapper around the string version of the deliverLogEvent() function that
/// automatically adds in the file name and line number of the locaton where the message is sent.
#ifndef LOGGER
#define LOGGER(severity, ...) Clypsalot::deliverLogEvent(LOG_SOURCE, __FILE__, __LINE__, Clypsalot::LogSeverity::severity, [&]() -> std::string { return Clypsalot::makeString(__VA_ARGS__); })
#endif

/// @brief A convenience wrappar around the generator version of the deliverLogEvent() function
/// that automatically adds in the file name and line number of the location where the message is
/// sent.
#ifndef LLOGGER
#define LLOGGER(severity, block) Clypsalot::deliverLogEvent(LOG_SOURCE, __FILE__, __LINE__, Clypsalot::LogSeverity::severity, [&]() -> std::string block)
#endif

/**
 * @brief A convience macro for automatically giving the file and line data to the \ref fatalError() function.
 */
#define FATAL_ERROR(message) Clypsalot::fatalError(message, __FILE__, __LINE__)
