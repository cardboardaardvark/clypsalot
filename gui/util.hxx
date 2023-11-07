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
#include <QSpacerItem>
#include <QString>
#include <QWidget>

#include <clypsalot/forward.hxx>
#include <clypsalot/util.hxx>

QSpacerItem* makeSpacer();
void openWindow(QWidget* window);
Clypsalot::SharedObject makeObject(
    const Clypsalot::ObjectDescriptor& descriptor,
    const std::vector<std::pair<QString, QString>>& outputs,
    const std::vector<std::pair<QString, QString>>& inputs,
    const Clypsalot::ObjectConfig& config
);

std::ostream& operator<<(std::ostream& os, const QPoint& point) noexcept;
std::ostream& operator<<(std::ostream& os, const QPointF& point) noexcept;
std::ostream& operator<<(std::ostream& os, const QSize& size) noexcept;
std::ostream& operator<<(std::ostream& os, const QSizeF& size) noexcept;
std::ostream& operator<<(std::ostream& os, const QString& string) noexcept;

template <typename... Args>
QString makeQString(const Args&... args) noexcept {
    return QString::fromStdString(Clypsalot::makeString(args...));
}
