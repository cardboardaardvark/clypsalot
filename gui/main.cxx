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

#include <QApplication>

#include <clypsalot/logging.hxx>
#include <clypsalot/module.hxx>
#include <clypsalot/thread.hxx>

#include "main.hxx"

using namespace std::placeholders;

int main(int argc, char *argv[])
{
    Clypsalot::logEngine().makeDestination<Clypsalot::ConsoleDestination>(Clypsalot::LogSeverity::trace);
    Clypsalot::initThreadQueue(0);

    initMetaTypes();

    QApplication application(argc, argv);
    mainWindow()->show();
    logWindow();

    QMetaObject::invokeMethod(mainWindow(), "loadModules", Qt::QueuedConnection);
    auto result = application.exec();
    Clypsalot::shutdownThreadQueue();
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
    qRegisterMetaType<const Clypsalot::ObjectDescriptor*>();
}
