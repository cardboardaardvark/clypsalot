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

#include <QRegularExpressionValidator>
#include <QScrollBar>

#include <clypsalot/error.hxx>

#include "logging.hxx"
#include "logwindow.hxx"
#include "ui_logwindow.h"

#define DEFAULT_LOG_SEVERITY Clypsalot::LogSeverity::info

LogWindowDestination::LogWindowDestination(const Clypsalot::LogSeverity severity) :
    Clypsalot::LogDestination(severity)
{ }

// This method could be called by any thread
void LogWindowDestination::handleLogEvent(const Clypsalot::LogEvent& event) noexcept
{
    Q_EMIT newMessage(QString::fromStdString(Clypsalot::asString(event)));
}

LogWindow::LogWindow(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::LogWindow),
    destination(Clypsalot::logEngine().makeDestination<LogWindowDestination>(DEFAULT_LOG_SEVERITY))
{
    ui->setupUi(this);

    updateMaxMessages();
    initLogSeverities();
    connect(&destination, SIGNAL(newMessage(const QString&)), ui->logMessages, SLOT(appendPlainText(const QString&)));
}

LogWindow::~LogWindow()
{
    delete ui;
}

void LogWindow::initLogSeverities()
{
    auto picker = ui->logSeverityPicker;

    for (const auto& severityName : Clypsalot::logSeverityNames)
    {
        picker->addItem(QString::fromStdString(severityName));
    }

    picker->setCurrentText(QString::fromStdString(Clypsalot::asString(destination.severity())));
}

void LogWindow::setSeverity(const QString& severityName)
{
    try
    {
        destination.severity(Clypsalot::logSeverity(severityName.toStdString()));
        ui->logSeverityPicker->setCurrentText(severityName);
    }
    catch (const std::exception& e)
    {
        LOGGER(error, "Caught exception while setting log severity", e.what());
    }
    catch (...)
    {
        FATAL_ERROR("Unknown exception when setting log severity");
    }
}

void LogWindow::jumpFirstMessage()
{
    auto scrollBar = ui->logMessages->verticalScrollBar();
    scrollBar->setSliderPosition(scrollBar->minimum());
}

void LogWindow::jumpLastMessage()
{
    auto scrollBar = ui->logMessages->verticalScrollBar();
    scrollBar->setSliderPosition(scrollBar->maximum());
}

void LogWindow::updateMaxMessages()
{
    ui->logMessages->setMaximumBlockCount(ui->maxMessages->sizeValue());
}
