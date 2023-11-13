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

#include "logger.hxx"
#include "logwindow.hxx"
#include "mainwindow.hxx"
#include "util.hxx"

#include "./ui_mainwindow.h"

MainWindow* MainWindow::instance()
{
    static auto singleton = new MainWindow();
    return singleton;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);

    connect(this, SIGNAL(statusUpdate(const QString&)), m_ui->statusBar, SLOT(showMessage(const QString&)));

    connect(m_ui->workArea->scene(), SIGNAL(selectionChanged()), this, SLOT(workAreaSelectionChanged()));
    workAreaSelectionChanged();
}

MainWindow::~MainWindow()
{
    delete m_ui;
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

std::initializer_list<QPushButton*> MainWindow::selectionDependentButtons()
{
    static const std::initializer_list<QPushButton*> list {
        m_ui->startButton,
        m_ui->pauseButton,
        m_ui->stopButton,
        m_ui->removeButton,
    };

    return list;
}

void MainWindow::loadModules()
{
    Clypsalot::threadQueuePost([this]
    {
        statusMessage("Loading modules");
        Clypsalot::importModule(Clypsalot::testModuleDescriptor());
        statusMessage("Modules loaded");
    });
}

void MainWindow::showLogWindow()
{
    openWindow(LogWindow::instance());
}

void MainWindow::workAreaSelectionChanged()
{
    auto buttonsEnabled = m_ui->workArea->scene()->selectedItems().size() > 0;

    for (auto button : selectionDependentButtons())
    {
        button->setEnabled(buttonsEnabled);
    }
}

WorkArea* MainWindow::workArea()
{
    return m_ui->workArea;
}

void MainWindow::statusMessage(const QString& message)
{
    Q_EMIT statusUpdate(message);
}

void MainWindow::errorMessage(const QString& message)
{
    statusMessage(makeQString("ERROR: ", message));
}
