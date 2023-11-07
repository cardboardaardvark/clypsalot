/* Copyright 2023 Tyler Riddle
 *
 * This file is part of Clypsalot. Clypsalot is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version. Clypsalot is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with Clypsalot. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include <csignal>
#include <iostream>
#include <unistd.h>

#include <QString>
#include <QStyleFactory>

#include <clypsalot/error.hxx>
#include <clypsalot/logging.hxx>
#include <clypsalot/macros.hxx>
#include <clypsalot/module.hxx>
#include <clypsalot/util.hxx>

#include "main.hxx"
#include "util.hxx"

using namespace Clypsalot;
using namespace std::placeholders;

static const QString logLevelArg("log-level");
static const QString showLogWindowArg("show-log-window");
static const QString threadsArg("threads");
static const QString windowLogLevelArg("window-log-level");

static void parseCommandLine(QCommandLineParser& parser, const QApplication& application);
static void shutdown();

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QCoreApplication::setApplicationName("Clypsalot");
    QCoreApplication::setApplicationVersion(CLYPSALOT_GUI_VERSION);

    QCommandLineParser args;
    parseCommandLine(args, application);

    logEngine().makeDestination<ConsoleDestination>(logSeverity(args.value(logLevelArg).toStdString()));
    if (args.isSet(windowLogLevelArg)) logWindow()->setSeverity(args.value(windowLogLevelArg));
    // Make sure the LogWindow singleton exists before things start logging
    logWindow();

    LOGGER(info, "Clypsalot GUI version ", CLYPSALOT_GUI_VERSION, " starting");
    LOGGER(info, "Using Clypsalot version ", version());

    initMetaTypes();
    initThreadQueue(args.value(threadsArg).toUInt());

    openWindow(mainWindow());
    if (args.isSet(showLogWindowArg)) openWindow(logWindow());

    QMetaObject::invokeMethod(mainWindow(), "loadModules", Qt::QueuedConnection);
    auto result = application.exec();
    shutdown();
    return result;
}

MainWindow* mainWindow()
{
    static auto singleton = new MainWindow();
    return singleton;
}

LogWindow* logWindow()
{
    static auto singleton = new LogWindow();
    return singleton;
}

void initMetaTypes()
{
    qRegisterMetaType<const ObjectDescriptor*>();
}

static void shutdownFailed(int)
{
    FATAL_ERROR("Clean shutdown was not achieved");
}

// Take no more than 2 seconds to stop all the objects, shutdown the thread queue, and run
// all the destructors. This happens after the user requested the application be quit and
// the user interface is gone so the program needs to exit under any circumstances.
static void shutdown()
{
    LOGGER(verbose, "Shutting down");

    const int shutdownTime = 2;

    std::signal(SIGALRM, shutdownFailed);
    alarm(shutdownTime);

    LOGGER(debug, "Stopping objects");
    mainWindow()->workArea()->stopObjects();
    LOGGER(debug, "Shutting down thread queue");
    shutdownThreadQueue();
    LOGGER(debug, "Done shutting down");
}

static QString makeLogLevelsText()
{
    QString levels;

    for (const auto& name : logSeverityNames)
    {
        levels += QString::fromStdString(name) + ", ";
    }

    levels.truncate(levels.size() - 2);

    return levels;
}

[[noreturn]] static void commandLineError(const std::string& message)
{
    std::cerr << "Error parsing command line arguments: " << message << std::endl;
    exit(1);
}

static void validateLogSeverity(const QString& severityName)
{
    try
    {
        logSeverity(severityName.toStdString());
    }
    catch (...)
    {
        commandLineError(makeString("'", severityName, "' is not a valid log severity"));
    }
}

static void parseCommandLine(QCommandLineParser& parser, const QApplication& application)
{
    parser.setApplicationDescription("Clypsalot graphical user interface");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOption(QCommandLineOption
    (
        QStringList({logLevelArg, "l"}),
        "Console log level (" + makeLogLevelsText() + ")",
        "severity",
        QString::fromStdString(asString(LogSeverity::fatal))
    ));

    parser.addOption(QCommandLineOption
    (
        QStringList({showLogWindowArg, "S"}),
        "Show the log window at start"
    ));

    parser.addOption(QCommandLineOption
    (
        QStringList({threadsArg, "t"}),
        "Force number of threads in the thread pool",
        "num threads",
        QString::number(0)
    ));

    parser.addOption(QCommandLineOption
    (
        QStringList({windowLogLevelArg, "L"}),
        "Window log level (" + makeLogLevelsText() + ")",
        "severity"
    ));

    parser.process(application);

    {
        bool ok = false;
        parser.value(threadsArg).toUInt(&ok);

        if (! ok) commandLineError("Number of threads must be an unsigned integer");
    }

    validateLogSeverity(parser.value(logLevelArg));

    if (parser.isSet(windowLogLevelArg)) validateLogSeverity(parser.value(windowLogLevelArg));
}
