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

#include "util.hxx"

namespace Ui {
    class LogWindow;
}

class LogWindowDestination : public QObject, public Clypsalot::LogDestination
{
    Q_OBJECT

    protected:
    Clypsalot::Mutex m_queueMutex;
    ThreadSafeQueue<Clypsalot::LogEvent> m_eventQueue;

    void handleLogEvent(const Clypsalot::LogEvent& event) noexcept override;

    protected Q_SLOTS:
    void queueReady();

    Q_SIGNALS:
    void checkMessages();

    public:
    LogWindowDestination(const Clypsalot::LogSeverity severity);
    ThreadSafeQueue<Clypsalot::LogEvent>::QueueType getEvents();
};

class LogWindow : public QFrame
{
    Q_OBJECT

    Ui::LogWindow *m_ui = nullptr;
    LogWindowDestination& m_destination;

    void initLogSeverities();

    protected Q_SLOTS:
    void updateMessages();
    void updateMaxMessages();

    public:
    static LogWindow* instance();
    explicit LogWindow(QWidget *parent = nullptr);
    ~LogWindow();

    public Q_SLOTS:
    void jumpFirstMessage();
    void jumpLastMessage();
    void setSeverity(const QString& severityName);
};
