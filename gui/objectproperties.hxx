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

#include <map>

#include <QFormLayout>
#include <QWidget>

#include <clypsalot/object.hxx>

#include "data.hxx"

class ObjectPropertiesEditor : public QWidget
{
    Q_OBJECT

    std::map<QString, AnyDataSource*> fields;
    std::map<AnyDataSource*, bool> fieldRequired;
    QFormLayout* form = nullptr;

    public:
    explicit ObjectPropertiesEditor(QWidget* parent = nullptr);
    void addProperty(const QString& name, const Clypsalot::PropertyType type, const bool required, const std::any& initial);
    size_t numFields() const;
    Clypsalot::ObjectConfig config() const;

    public Q_SLOTS:
    void checkReady();

    Q_SIGNALS:
    void ready(bool);
};
