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

#include <any>

#include <QComboBox>
#include <QLineEdit>

#include <clypsalot/property.hxx>

struct AnyDataSource
{
    virtual ~AnyDataSource() = default;
    virtual std::any any() = 0;
    virtual void any(const std::any& value) = 0;
};

struct BooleanComboBox : public QComboBox, public AnyDataSource
{
    Q_OBJECT

    public:
    BooleanComboBox(QWidget* parent);
    operator bool();
    Clypsalot::Property::BooleanType booleanValue();
    virtual std::any any() override;
    virtual void any(const std::any& value) override;

    Q_SIGNALS:
    void changed();
};

struct RegexValidatingLineEdit : public QLineEdit
{
    Q_OBJECT

    public:
    const QRegularExpressionValidator validator;
    RegexValidatingLineEdit(const QRegularExpression& regexp, QWidget* parent = nullptr);

    Q_SIGNALS:
    void changed();
};

struct IntegerLineEdit : public RegexValidatingLineEdit, public AnyDataSource
{
    IntegerLineEdit(QWidget* parent);
    Clypsalot::Property::IntegerType integerValue();
    virtual std::any any() override;
    virtual void any(const std::any& value) override;
};

struct RealLineEdit : public RegexValidatingLineEdit, public AnyDataSource
{
    RealLineEdit(QWidget* parent);
    Clypsalot::Property::RealType realValue();
    virtual std::any any() override;
    virtual void any(const std::any& value) override;
};

struct StringLineEdit : public QLineEdit, public AnyDataSource
{
    StringLineEdit(QWidget* parent);
    Clypsalot::Property::StringType stringValue();
    virtual std::any any() override;
    virtual void any(const std::any& value) override;
};

struct SizeLineEdit : public RegexValidatingLineEdit, public AnyDataSource
{
    SizeLineEdit(QWidget* parent);
    Clypsalot::Property::SizeType sizeValue();
    virtual std::any any() override;
    virtual void any(const std::any& value) override;
};
