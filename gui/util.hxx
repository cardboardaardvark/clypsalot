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

#include <QGraphicsLayout>
#include <QGraphicsProxyWidget>
#include <QObject>
#include <QSpacerItem>
#include <QString>
#include <QWidget>

#include <clypsalot/forward.hxx>
#include <clypsalot/thread.hxx>
#include <clypsalot/util.hxx>

class QueueSignals : public QObject
{
    Q_OBJECT

    public:
    explicit QueueSignals(QObject* in_parent = nullptr);

    Q_SIGNALS:
    void ready();
};

template <typename T>
class ThreadSafeQueue : public QueueSignals
{
    public:
    using QueueType = std::list<T>;

    mutable Clypsalot::Mutex m_mutex;
    QueueType m_queue;
    bool m_needToSignal = true;

    void signalIfNeeded()
    {
        assert(m_mutex.haveLock());

        if (m_needToSignal)
        {
            Q_EMIT ready();
            m_needToSignal = false;
        }
    }

    public:
    explicit ThreadSafeQueue(QObject* parent = nullptr) :
        QueueSignals(parent)
    { }

    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    void operator=(const ThreadSafeQueue&) = delete;

    std::size_t push(const T& in_value)
    {
        std::scoped_lock lock(m_mutex);
        m_queue.push_back(in_value);
        signalIfNeeded();
    }

    std::size_t push(T&& in_value)
    {
        std::scoped_lock lock(m_mutex);
        m_queue.push_back(in_value);
        signalIfNeeded();
        return m_queue.size();
    }

    template <typename... Args>
    std::size_t emplace(Args&&... in_args)
    {
        std::scoped_lock lock(m_mutex);
        m_queue.emplace_back(in_args...);
        signalIfNeeded();
        return m_queue.size();
    }

    QueueType drain()
    {
        std::scoped_lock lock(m_mutex);
        QueueType retval;
        retval = std::move(m_queue);
        m_needToSignal = true;
        return retval;
    }
};

QSpacerItem* makeSpacer();
void openWindow(QWidget* in_window);
void initObject(
    const Clypsalot::SharedObject& in_object,
    const std::vector<std::pair<QString, QString>>& in_outputs,
    const std::vector<std::pair<QString, QString>>& in_inputs,
    const Clypsalot::ObjectConfig& in_config
);

std::ostream& operator<<(std::ostream& in_os, const QPoint& in_rhs) noexcept;
std::ostream& operator<<(std::ostream& in_os, const QPointF& in_rhs) noexcept;
std::ostream& operator<<(std::ostream& in_os, const QRect& in_rhs) noexcept;
std::ostream& operator<<(std::ostream& in_os, const QRectF& in_rhs) noexcept;
std::ostream& operator<<(std::ostream& in_os, const QSize& in_rhs) noexcept;
std::ostream& operator<<(std::ostream& in_os, const QSizeF& in_rhs) noexcept;
std::ostream& operator<<(std::ostream& in_os, const QString& in_rhs) noexcept;

template <typename... Args>
QString makeQString(const Args&... in_args) noexcept {
    return QString::fromStdString(Clypsalot::makeString(in_args...));
}
