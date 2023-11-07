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

#include <QCloseEvent>
#include <QString>

#include <clypsalot/module.hxx>

#include "test/module/module.hxx"

#include "logging.hxx"
#include "main.hxx"
#include "mainwindow.hxx"
#include "util.hxx"

#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(this, SIGNAL(statusUpdate(const QString&)), ui->statusBar, SLOT(showMessage(const QString&)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (! shouldQuit())
    {
        event->ignore();
        return;
    }

    QApplication::quit();
}

// TODO Check for unsaved work, etc
bool MainWindow::shouldQuit()
{
    return true;
}

void MainWindow::loadModules()
{
    Clypsalot::threadQueuePost([this]
    {
        Q_EMIT statusUpdate("Loading modules");
        Clypsalot::importModule(Clypsalot::testModuleDescriptor());
        Q_EMIT statusUpdate("Modules loaded");
    });
}

void MainWindow::showLogWindow()
{
    openWindow(logWindow());
}

WorkArea* MainWindow::workArea()
{
    return ui->workArea;
}

void MainWindow::showError(const QString& message)
{
    Q_EMIT statusUpdate(makeQString("ERROR: ", message));
}
