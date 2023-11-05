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

#include <clypsalot/util.hxx>

#include "data.hxx"

static const QString trueString("True");
static const QString falseString("False");

BooleanComboBox::BooleanComboBox(QWidget* parent) :
    QComboBox(parent)
{
    setPlaceholderText("Boolean data type");
    addItem(trueString);
    addItem(falseString);

    connect(this, &QComboBox::currentTextChanged, this, &BooleanComboBox::changed);
}

BooleanComboBox::operator bool()
{
    if (currentText() == trueString) return true;
    return false;
}

Clypsalot::Property::BooleanType BooleanComboBox::booleanValue()
{
    return operator bool();
}

std::any BooleanComboBox::any()
{
    if (currentIndex() == -1) return nullptr;
    return booleanValue();
}

void BooleanComboBox::any(const std::any& value)
{
    if (value.type() == typeid(nullptr)) return;

    if (Clypsalot::anyToBool(value)) setCurrentText(trueString);
    else setCurrentText(falseString);
}

RegexValidatingLineEdit::RegexValidatingLineEdit(const QRegularExpression& regexp, QWidget* parent) :
    QLineEdit(parent),
    validator(regexp)
{
    setValidator(&validator);

    connect(this, &QLineEdit::textEdited, this, &RegexValidatingLineEdit::changed);
}

IntegerLineEdit::IntegerLineEdit(QWidget* parent) :
    RegexValidatingLineEdit(QRegularExpression("-?\\d*"), parent)
{
    setToolTip("Integer data type");
}

Clypsalot::Property::IntegerType IntegerLineEdit::integerValue()
{
    return text().toInt();
}

std::any IntegerLineEdit::any()
{
    if (text() == "") return nullptr;
    return integerValue();
}

void IntegerLineEdit::any(const std::any& value)
{
    if (value.type() == typeid(nullptr)) return;

    setText(QString::number(Clypsalot::anyToInt(value)));
}

RealLineEdit::RealLineEdit(QWidget* parent) :
    RegexValidatingLineEdit(QRegularExpression("-?\\d*\\.\\d*"), parent)
{
    setToolTip("Real data type");
}

Clypsalot::Property::RealType RealLineEdit::realValue()
{
    return text().toFloat();
}

std::any RealLineEdit::any()
{
    if (text() == "") return nullptr;
    return realValue();
}

void RealLineEdit::any(const std::any& value)
{
    if (value.type() == typeid(nullptr)) return;

    setText(QString::number(Clypsalot::anyToFloat(value)));
}

StringLineEdit::StringLineEdit(QWidget* parent) :
    QLineEdit(parent)
{
    setToolTip("String data type");
}

Clypsalot::Property::StringType StringLineEdit::stringValue()
{
    return text().toStdString();
}

std::any StringLineEdit::any()
{
    static const QRegularExpression whitespace("\\s*");

    if (whitespace.match(text()).hasMatch()) return nullptr;
    return stringValue();
}

void StringLineEdit::any(const std::any& value)
{
    if (value.type() == typeid(nullptr)) return;

    setText(QString::fromStdString(Clypsalot::anyToString(value)));
}

SizeLineEdit::SizeLineEdit(QWidget* parent) :
    RegexValidatingLineEdit(QRegularExpression("\\d*"), parent)
{
    setToolTip("Size data type");
}

Clypsalot::Property::SizeType SizeLineEdit::sizeValue()
{
    return text().toUInt();
}

std::any SizeLineEdit::any()
{
    if (text() == "") return nullptr;
    return sizeValue();
}

void SizeLineEdit::any(const std::any& value)
{
    if (value.type() == typeid(nullptr)) return;

    setText(QString::number(Clypsalot::anyToSize(value)));
}
