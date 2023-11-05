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

#pragma once

#include <QFrame>

#include <clypsalot/logging.hxx>

namespace Ui {
    class LogWindow;
}

class LogWindowDestination : public QObject, public Clypsalot::LogDestination
{
    Q_OBJECT

    protected:
    virtual void handleLogEvent(const Clypsalot::LogEvent& event) noexcept;

    Q_SIGNALS:
    void newMessage(const QString& message);

    public:
    LogWindowDestination(const Clypsalot::LogSeverity severity);
};

class LogWindow : public QFrame
{
    Q_OBJECT

    Ui::LogWindow *ui = nullptr;
    LogWindowDestination& destination;

    void initLogSeverities();

    protected Q_SLOTS:
    void updateMaxMessages();

    public:
    explicit LogWindow(QWidget *parent = nullptr);
    ~LogWindow();

    public Q_SLOTS:
    void jumpFirstMessage();
    void jumpLastMessage();
    void setSeverity(const QString& severityName);
};
