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

#include <clypsalot/logging.hxx>
#include <clypsalot/util.hxx>

/// @brief The plain text name of a LogEvent source that will be used by the LOGGER and LLOGGER macros.
#ifndef LOG_SOURCE
#define LOG_SOURCE "clypsalot"
#endif

/// @brief A convenience wrapper around the string version of the deliverLogEvent() function that
/// automatically adds in the file name and line number of the locaton where the message is sent.
#define LOGGER(severity, ...) Clypsalot::deliverLogEvent(LOG_SOURCE, __FILE__, __LINE__, Clypsalot::LogSeverity::severity, [&]() -> std::string { return Clypsalot::makeString(__VA_ARGS__); })

/// @brief A convenience wrappar around the generator version of the deliverLogEvent() function
/// that automatically adds in the file name and line number of the location where the message is
/// sent.
#define LLOGGER(severity, block) Clypsalot::deliverLogEvent(LOG_SOURCE, __FILE__, __LINE__, Clypsalot::LogSeverity::severity, [&]() -> std::string block)

#define OBJECT_LOGGER(severity, ...) LOGGER(severity, *this, ": ", __VA_ARGS__)
#define PORT_LOGGER(severity, ...) LOGGER(severity, *this, ": ", __VA_ARGS__)
