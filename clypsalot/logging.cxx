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

#include <cassert>
#include <cstring>
#include <iostream>

#include <clypsalot/error.hxx>
#include <clypsalot/logging.hxx>
#include <clypsalot/util.hxx>

/// @cond NO_DOCUMENT
#define MICROSECONDS 1000000
/// @endcond

/// @file
namespace Clypsalot {
    /// @brief Return a default formatted version of the log event.
    std::string LogEvent::toString(const LogEvent& event) noexcept
    {
        const auto microseconds = logEngine().runDuration(event.when);
        const auto ticks = microseconds.count();
        const auto seconds = ticks / MICROSECONDS;
        const auto fractional = ticks - seconds;
        std::string fileInfo;

        if (event.file != nullptr)
        {
            std::string_view file(event.file);

            if (file.starts_with(CLYPSALOT_PROJECT_DIR)) {
                file.remove_prefix(std::strlen(CLYPSALOT_PROJECT_DIR));
            }

            fileInfo = makeString(file, ":", event.line, " ");
        }

        return makeString(
            seconds, ".", fractional, " ",
            event.thread, " ",
            fileInfo,
            event.severity, ": ",
            event.message
        );
    }

    /// @brief Construct a new log destination setting the initial severity.
    LogDestination::LogDestination(const LogSeverity severity) :
        minSeverity(severity)
    { }

    /// @brief Get the current minimum severity of the log destination.
    LogSeverity LogDestination::severity() noexcept
    {
        std::shared_lock lock(mutex);
        return minSeverity;
    }

    /// @brief Set a new minimum severity on the log destination.
    void LogDestination::severity(const LogSeverity severity) noexcept
    {
        std::scoped_lock lock(mutex);
        minSeverity = severity;
    }

    /// @brief Returns true if the given severity is at least as severe as any of the
    /// registered log destinations. Returns false otherwise.
    bool LogEngine::shouldLog(const LogSeverity severity) noexcept
    {
        std::shared_lock lock(mutex);

        for (const auto destination : destinations)
        {
            if (severity >= destination->severity())
            {
                return true;
            }
        }

        return false;
    }

    /// @brief Invoke LogDestination::handleLogEvent() on all registered log destinations
    /// with a current minimum severity at least as great as the severity of the log event.
    void LogEngine::deliver(const LogEvent& event) noexcept
    {
        std::shared_lock engineLock(mutex);

        for (auto destination : destinations)
        {
            std::shared_lock destinationLock(destination->mutex);

            if (event.severity >= destination->minSeverity)
            {
                destination->handleLogEvent(event);
            }
        }
    }

    /// @brief Return the number of microseconds since the LogEngine singleton was created
    /// to the timestamp given.
    std::chrono::microseconds LogEngine::runDuration(const LogEvent::Timestamp& when) noexcept
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(when - programStart);
    }

    /// @brief A LogDestination that sends log messages to stderr.
    ConsoleDestination::ConsoleDestination(const LogSeverity severity) :
        LogDestination(severity)
    { }

    /// @cond NO_DOCUMENT
    void ConsoleDestination::handleLogEvent(const LogEvent& event) noexcept
    {
        assert(mutex.haveSharedLock());

        // The new line is included instead of using std::endl so writing the entire line
        // is an atomic operation otherwise if two threads are writing to the console at
        // the same time the new lines could get interleaved.
        std::cerr << makeString(event, "\n");
    }
    /// @endcond

    /// @brief Return a reference to the LogEngine singleton.
    LogEngine& logEngine() noexcept
    {
        static auto engine = new LogEngine;
        return *engine;
    }

    LogSeverity logSeverity(const std::string& name)
    {
        if (name == "trace") return LogSeverity::trace;
        if (name == "debug") return LogSeverity::debug;
        if (name == "verbose") return LogSeverity::verbose;
        if (name == "info") return LogSeverity::info;
        if (name == "notice") return LogSeverity::notice;
        if (name == "warn") return LogSeverity::warn;
        if (name == "error") return LogSeverity::error;
        if (name == "fatal") return LogSeverity::fatal;

        throw KeyError(makeString("Unknown log severity: ", name), name);
    }

    /// @brief If at least one registered LogDestination will accept the log message create
    /// a LogEvent and deliver it using the LogEngine singleton.
    void deliverLogEvent(
            const char* source,
            const char* file,
            const uint32_t line,
            const LogSeverity severity,
            const std::string message
    ) noexcept
    {
        // Capture the time at the first opportunity so it is as accurate as possible.
        auto when = LogEvent::Clock::now();
        auto& engine = logEngine();

        if (! engine.shouldLog(severity))
        {
            return;
        }

        // Because the minimum severity level of a destination can change while the log
        // system is running it is possible that the severity has changed since the call
        // to shouldLog(). This race condition is allowed to exist for the following
        // reasons:
        //
        //   1. Removing the race condition means either mutexs have to be locked for a relatively
        //      long time or a LogEvent has to be constructed and could be discarded with only
        //      the severity being read from it if no destination would accept it. Given the sheer
        //      number of places LogEvents could be generated this overhead is highly undesirable.
        //
        //   2. The minimum severity of a destination is checked again before the LogEvent
        //      is given to it which means the destination will never handle an event
        //      lower than it's current severity.
        //
        //   3. The destination could potentially miss a few LogEvents if there is a lot
        //      of logging activity going on and the minimum severity of the destination
        //      is decreased. This is an accepted tradeoff to reduce the cost of the
        //      logging system in the most common case: log messages are not going to be
        //      handled by any destination.
        engine.deliver({ source, file, line, std::this_thread::get_id(), when, severity, message });
    }

    /// @brief If at least one LogDestination will accept the log message create a new LogEvent
    /// and deliver it using the LogSingleton. The generator function will only be invoked if
    /// a log event is created.
    void deliverLogEvent(
        const char* source,
        const char* file,
        const uint32_t line,
        const LogSeverity severity,
        const LogMessageGenerator& generator
    ) noexcept
    {
        // Capture the time at the first opportunity so it is as accurate as possible.
        auto when = LogEvent::Clock::now();
        auto& engine = logEngine();

        if (! engine.shouldLog(severity))
        {
            return;
        }

        // See the comment about the race condition in the other deliverLogEvent() function
        engine.deliver({ source, file, line, std::this_thread::get_id(), when, severity, generator() });
    }

    /// @brief Return a human readable version of the given log severity.
    std::string toString(const LogSeverity severity) noexcept
    {
        switch (severity)
        {
            case LogSeverity::debug: return "debug";
            case LogSeverity::error: return "error";
            case LogSeverity::fatal: return "fatal";
            case LogSeverity::info: return "info";
            case LogSeverity::notice: return "notice";
            case LogSeverity::trace: return "trace";
            case LogSeverity::verbose: return "verbose";
            case LogSeverity::warn: return "warn";
        }
    }

    /// @cond NO_DOCUMENT
    std::ostream& operator<<(std::ostream& os, const LogSeverity severity) noexcept
    {
        os << toString(severity);
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const LogEvent& event) noexcept
    {
        os << toString(event);
        return os;
    }
    /// @endcond
}
