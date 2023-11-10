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

#include <chrono>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <string>
#include <thread>
#include <vector>

#include <clypsalot/forward.hxx>
#include <clypsalot/thread.hxx>

/// @file
namespace Clypsalot {
    /// @brief The prototype for a function that can be used as part of a LogEvent. These can
    /// be passed to the LLOGGER() macro and the generator version of the deliverLogEvent() method.
    using LogMessageGenerator = std::function<std::string ()>;

    /// @brief The set of severities of log messages ordered from least to most severe.
    enum class LogSeverity : uint_fast8_t {
        // These must be in order from least to most severe
        trace = 1,
        debug,
        verbose,
        info,
        notice,
        warn,
        error,
        fatal,
    };

    static std::initializer_list<std::string> logSeverityNames =
    {
        toString(LogSeverity::trace),
        toString(LogSeverity::debug),
        toString(LogSeverity::verbose),
        toString(LogSeverity::info),
        toString(LogSeverity::notice),
        toString(LogSeverity::warn),
        toString(LogSeverity::error),
        toString(LogSeverity::fatal),
    };

    /// @brief All of the data associated with a log message.
    struct LogEvent
    {
        /// @cond NO_DOCUMENT
        using Clock = std::chrono::system_clock;
        using Timestamp = std::chrono::time_point<Clock>;
        /// @endcond

        /// @brief The plain text name of the subsystem where the log event was created.
        const char *source;
        /// @brief The name of the file where the log event was created or nullptr if the file is
        /// not known.
        const char *file;
        /// @brief The line number in the file where the log event was created or 0 if the line
        /// number is not known.
        const uint32_t line;
        /// @brief The thread that created the log event.
        const std::thread::id thread;
        /// @brief The point in time when the log event happened.
        const Timestamp when;
        /// @brief The severity of the log event.
        const LogSeverity severity;
        /// @brief Plain text message associated with the log event.
        const std::string message;

        static std::string toString(const LogEvent& event) noexcept;
    };

    /// @brief Base class for objects that receive LogEvents from the LogEngine.
    class LogDestination : protected SharedLockable
    {
        friend LogEngine;

        LogSeverity m_minSeverity;

        protected:
        LogDestination(const LogSeverity severity);
        /// @brief Classes that implement a log destination must override this method
        /// and handle a log event when it is invoked by the LogEngine.
        ///
        /// When this method is invoked by the log engine the engine holds a shared
        /// lock on the mutex.
        virtual void handleLogEvent(const LogEvent& event) noexcept = 0;

        public:
        LogDestination(const LogDestination&) = delete;
        virtual ~LogDestination() = default;
        void operator=(const LogDestination&) = delete;
        LogSeverity severity() noexcept;
        void severity(const LogSeverity newSeverity) noexcept;
    };

    /// @brief Object for managing LogDestinations and delivery of log messages. There
    /// is a singleton instance of this object that is accessed via the logEngine() function.
    class LogEngine : private SharedLockable
    {
        const LogEvent::Timestamp m_programStart = LogEvent::Clock::now();
        std::vector<LogDestination*> m_destinations;

        public:
        LogEngine() = default;
        LogEngine(const LogEngine&) = delete;
        // The log engine is a singleton that exists for the life of the program.
        ~LogEngine() = delete;
        void operator=(const LogEngine&) = delete;
        bool shouldLog(const LogSeverity severity) noexcept;
        void deliver(const LogEvent& event) noexcept;
        std::chrono::microseconds runDuration(const LogEvent::Timestamp& when) noexcept;

        /// @brief Create an instance of a LogDestination, take ownership of it, and return
        /// a reference it.
        template <std::derived_from<LogDestination> T, typename... Args>
        T& makeDestination(Args&&... args) noexcept
        {
            auto destination = new T(args...);
            std::scoped_lock lock(m_mutex);
            m_destinations.push_back(destination);
            return *destination;
        }
    };

    /// @brief A log destination that sends log messages to stderr.
    struct ConsoleDestination : public LogDestination
    {
        ConsoleDestination(const LogSeverity severity);
        virtual void handleLogEvent(const LogEvent& event) noexcept override;
    };

    LogEngine& logEngine() noexcept;
    LogSeverity logSeverity(const std::string& name);
    void deliverLogEvent(const char* source, const char* file, const uint32_t line, const LogSeverity severity, const std::string message) noexcept;
    void deliverLogEvent(const char* source, const char* file, const uint32_t line, const LogSeverity severity, const LogMessageGenerator& generator) noexcept;
    std::string toString(const LogSeverity severity) noexcept;
    /// @cond NO_DOCUMENT
    std::ostream& operator<<(std::ostream& os, const LogEvent& event) noexcept;
    std::ostream& operator<<(std::ostream& os, const LogSeverity severity) noexcept;
    /// @endcond
}
