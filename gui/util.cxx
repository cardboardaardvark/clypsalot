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

#include <clypsalot/object.hxx>
#include <clypsalot/module.hxx>

#include "util.hxx"

QSpacerItem* makeSpacer()
{
    return new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void openWindow(QWidget* window)
{
    window->show();
    window->setWindowState(window->windowState() & ~Qt::WindowMinimized);
    window->raise();
}

Clypsalot::SharedObject makeObject(
    const Clypsalot::ObjectDescriptor& descriptor,
    const std::vector<std::pair<QString, QString>>& outputs,
        const std::vector<std::pair<QString, QString>>& inputs,
    const Clypsalot::ObjectConfig& config
) {
    auto object = descriptor.make();
    std::scoped_lock lock(*object);

    for (const auto& [type, name] : outputs) object->addOutput(type.toStdString(), name.toStdString());
    for (const auto& [type, name] : inputs) object->addInput(type.toStdString(), name.toStdString());

    object->configure(config);

    return object;
}

std::ostream& operator<<(std::ostream& os, const QPoint& point) noexcept
{
    os << point.x() << "x" << point.y();
    return os;
}

std::ostream& operator<<(std::ostream& os, const QPointF& point) noexcept
{
    os << point.x() << "x" << point.y();
    return os;
}

std::ostream& operator<<(std::ostream& os, const QSize& size) noexcept
{
    os << size.width() << "x" << size.height();
    return os;
}

std::ostream& operator<<(std::ostream& os, const QSizeF& size) noexcept
{
    os << size.width() << "x" << size.height();
    return os;
}

std::ostream& operator<<(std::ostream& os, const QString& string) noexcept
{
    os << string.toStdString();
    return os;
}
