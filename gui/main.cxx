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

#include <QApplication>
#include <QCommandLineParser>
#include <QObject>
#include <QString>

#include <clypsalot/error.hxx>
#include <clypsalot/logging.hxx>
#include <clypsalot/macros.hxx>
#include <clypsalot/module.hxx>
#include <clypsalot/util.hxx>

#include "logger.hxx"
#include "logwindow.hxx"
#include "mainwindow.hxx"
#include "util.hxx"

using namespace Clypsalot;
using namespace std::placeholders;

static const QString logLevelArg("log-level");
static const QString showLogWindowArg("show-log-window");
static const QString threadsArg("threads");
static const QString windowLogLevelArg("window-log-level");

void initMetaTypes();
static void parseCommandLine(QCommandLineParser& parser, const QApplication& application);
static void shutdown();
QPalette darkTheme();

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QCoreApplication::setApplicationName("Clypsalot");
    QCoreApplication::setApplicationVersion(CLYPSALOT_GUI_VERSION);

    QCommandLineParser args;
    parseCommandLine(args, application);

    logEngine().makeDestination<ConsoleDestination>(logSeverity(args.value(logLevelArg).toStdString()));
    if (args.isSet(windowLogLevelArg)) LogWindow::instance()->setSeverity(args.value(windowLogLevelArg));
    // Make sure the LogWindow singleton exists before things start logging
    LogWindow::instance();

    LOGGER(info, "Clypsalot GUI version ", CLYPSALOT_GUI_VERSION, " starting");
    LOGGER(info, "Using Clypsalot version ", version());

    application.setPalette(darkTheme());
    initMetaTypes();
    initThreadQueue(args.value(threadsArg).toUInt());

    openWindow(MainWindow::instance());
    if (args.isSet(showLogWindowArg)) openWindow(LogWindow::instance());

    QMetaObject::invokeMethod(MainWindow::instance(), "loadModules", Qt::QueuedConnection);
    auto result = application.exec();
    shutdown();
    return result;
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
    MainWindow::instance()->workArea()->stopObjects();
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
        QString::fromStdString(toString(LogSeverity::fatal))
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

QPalette darkTheme()
{
    QPalette palette;

    // Originally from https://stackoverflow.com/a/56851493
    palette.setColor(QPalette::Window, QColor(53, 53, 53));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, QColor(25, 25, 25));
    palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    palette.setColor(QPalette::ToolTipBase, Qt::black);
    palette.setColor(QPalette::ToolTipText, Qt::white);
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Button, QColor(53, 53, 53));
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, QColor(42, 130, 218));
    palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    palette.setColor(QPalette::HighlightedText, Qt::black);

    return palette;
}
