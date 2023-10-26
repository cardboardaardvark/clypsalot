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

#include <boost/test/unit_test.hpp>

#include <clypsalot/error.hxx>
#include <clypsalot/logging.hxx>
#include <clypsalot/macros.hxx>
#include <clypsalot/util.hxx>

#include "test/lib/test.hxx"
#include "test/module/module.hxx"

namespace Clypsalot
{
    struct LogCounterDestination : LogDestination
    {
        LogCounterDestination() :
            LogDestination(LogSeverity::warn)
        { }

        virtual void handleLogEvent(const LogEvent&) noexcept override
        {
            severeLogEvents++;
        }
    };

    Fixture::Fixture()
    {
        severeLogEvents = 0;
    }

    Fixture::~Fixture()
    {
        if (severeLogEvents > 0)
        {
            // TODO Find a way to handle this cleaner using the Boost framework
            FATAL_ERROR(makeString("Severe log events detected: ", severeLogEvents));
        }
    }

    int runTests(int argc, char* argv[])
    {
        auto result = boost::unit_test::unit_test_main(initBoostUnitTest, argc, argv);
        return result;
    }

    void initTesting(int argc, char* argv[])
    {
        auto& consoleDestination = logEngine().makeDestination<ConsoleDestination>(LogSeverity::info);
        logEngine().makeDestination<LogCounterDestination>();

        if (argc == 2)
        {
            consoleDestination.severity(logSeverity(argv[1]));
        }

        importModule(testModuleDescriptor());
        LOGGER(debug, "Test environment is initialized");
    }

    bool initBoostUnitTest()
    {
        auto argc = boost::unit_test::framework::master_test_suite().argc;
        auto argv = boost::unit_test::framework::master_test_suite().argv;

        initTesting(argc, argv);
        return true;
    }
}
