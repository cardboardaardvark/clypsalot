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

#include <cassert>

#include <clypsalot/object.hxx>
#include <clypsalot/module.hxx>

#include "util.hxx"

QueueSignals::QueueSignals(QObject* in_parent) :
    QObject(in_parent)
{ }

QSpacerItem* makeSpacer()
{
    return new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void openWindow(QWidget* in_window)
{
    in_window->show();
    in_window->setWindowState(in_window->windowState() & ~Qt::WindowMinimized);
    in_window->raise();
}

void initObject(
    const Clypsalot::SharedObject& in_object,
    const std::vector<std::pair<QString, QString>>& in_outputs,
    const std::vector<std::pair<QString, QString>>& in_inputs,
    const Clypsalot::ObjectConfig& in_config
) {
    assert(in_object->haveLock());

    for (const auto& [type, name] : in_outputs) in_object->addOutput(type.toStdString(), name.toStdString());
    for (const auto& [type, name] : in_inputs) in_object->addInput(type.toStdString(), name.toStdString());

    in_object->configure(in_config);
}

std::ostream& operator<<(std::ostream& in_os, const QPoint& in_rhs) noexcept
{
    in_os << in_rhs.x() << "x" << in_rhs.y();
    return in_os;
}

std::ostream& operator<<(std::ostream& in_os, const QPointF& in_rhs) noexcept
{
    in_os << in_rhs.x() << "x" << in_rhs.y();
    return in_os;
}

std::ostream& operator<<(std::ostream& in_os, const QRect& in_rhs) noexcept
{
    in_os << "QRect(" << in_rhs.x() << ", " << in_rhs.y() << ", " << in_rhs.width() << ", " << in_rhs.height() << ")";
    return in_os;
}

std::ostream& operator<<(std::ostream& in_os, const QRectF& in_rhs) noexcept
{
    in_os << "QRectF(" << in_rhs.x() << ", " << in_rhs.y() << ", " << in_rhs.width() << ", " << in_rhs.height() << ")";
    return in_os;
}

std::ostream& operator<<(std::ostream& in_os, const QSize& in_rhs) noexcept
{
    in_os << in_rhs.width() << "x" << in_rhs.height();
    return in_os;
}

std::ostream& operator<<(std::ostream& in_os, const QSizeF& in_rhs) noexcept
{
    in_os << in_rhs.width() << "x" << in_rhs.height();
    return in_os;
}

std::ostream& operator<<(std::ostream& in_os, const QString& in_rhs) noexcept
{
    in_os << in_rhs.toStdString();
    return in_os;
}
