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

#include <QLineEdit>

#include <clypsalot/property.hxx>

#include "logging.hxx"
#include "objectproperties.hxx"

ObjectPropertiesEditor::ObjectPropertiesEditor(QWidget* parent) :
    QWidget(parent),
    form(new QFormLayout(this))
{ }

void ObjectPropertiesEditor::addProperty(const QString& name, const Clypsalot::PropertyType type, const bool required, const std::any& initial)
{
    assert(! fields.contains(name));

    const QString label(name + ":");
    QWidget* widget = nullptr;

    switch (type)
    {
        case Clypsalot::PropertyType::boolean: widget = new BooleanComboBox(this); break;
        case Clypsalot::PropertyType::file: widget = new StringLineEdit(this); break;
        case Clypsalot::PropertyType::integer: widget = new IntegerLineEdit(this); break;
        case Clypsalot::PropertyType::real: widget = new RealLineEdit(this); break;
        case Clypsalot::PropertyType::size: widget = new SizeLineEdit(this); break;
        case Clypsalot::PropertyType::string: widget = new StringLineEdit(this); break;
    }

    assert(widget != nullptr);
    connect(widget, SIGNAL(changed()), this, SLOT(checkReady()));
    form->addRow(label, widget);
    auto field = dynamic_cast<AnyDataSource*>(widget);
    field->any(initial);
    fields[name] = field;
    fieldRequired[field] = required;
}

size_t ObjectPropertiesEditor::numFields() const
{
    return fields.size();
}

Clypsalot::ObjectConfig ObjectPropertiesEditor::config() const
{
    Clypsalot::ObjectConfig fieldValues;

    for (const auto& [name, field] : fields)
    {
        const auto& any = field->any();

        if (any.type() == typeid(nullptr)) continue;

        fieldValues.emplace_back(name.toStdString(), field->any());
    }

    return fieldValues;
}

void ObjectPropertiesEditor::checkReady()
{
    for (const auto& [name, field] : fields)
    {
        if (! fieldRequired.at(field)) continue;

        if (field->any().type() == typeid(nullptr))
        {
            LOGGER(trace, "Object properties editor is not ready yet because field has no data: ", name.toStdString());
            Q_EMIT ready(false);
            return;
        }
    }

    LOGGER(trace, "Object properties editor is ready");
    Q_EMIT ready(true);
}
