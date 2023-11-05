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
    undoHistoryWindow = new QUndoView(&ui->workArea->undoHistory());

    initEditMenu();

    connect(this, SIGNAL(statusUpdate(const QString&)), ui->statusBar, SLOT(showMessage(const QString&)));
}

MainWindow::~MainWindow()
{
    delete undoHistoryWindow;
    delete ui;
}

void MainWindow::initEditMenu()
{
    QMenu* editMenu = nullptr;

    for (auto menu : menuBar()->findChildren<QMenu*>())
    {
        if (menu->title() == "Edit")
        {
            editMenu = menu;
            break;
        }
    }

    if (editMenu == nullptr)
    {
        LOGGER(error, "Could not find Edit menu");
        return;
    }

    const auto& undoHistory = ui->workArea->undoHistory();
    auto undoAction = undoHistory.createUndoAction(editMenu);
    undoAction->setShortcut(QKeySequence("Ctrl+Z"));
    editMenu->addAction(undoAction);

    auto redoAction = undoHistory.createRedoAction(editMenu);
    redoAction->setShortcut(QKeySequence("Ctrl+Y"));
    editMenu->addAction(redoAction);
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

void MainWindow::sizeWorkAreaToContents()
{
    ui->workArea->sizeToContents();
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

void MainWindow::showUndoHistoryWindow()
{
    openWindow(undoHistoryWindow);
}

void MainWindow::redrawWorkArea()
{
    ui->workArea->update();
}

WorkArea* MainWindow::workArea()
{
    return ui->workArea;
}

QScrollArea* MainWindow::workAreaScrollArea()
{
    return ui->workAreaScrollArea;
}

void MainWindow::startObjects()
{
    ui->workArea->startObjects();
}
