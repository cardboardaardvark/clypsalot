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

#include <QComboBox>
#include <QDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include <clypsalot/forward.hxx>

namespace Ui {
    class CreateObjectDialog;
}

class PortEditor : public QWidget
{
    Q_OBJECT

    protected:
    QVBoxLayout* mainLayout;
    QHBoxLayout* topLayout;
    QLineEdit* portNameInput;
    QComboBox* portTypeInput;
    QPushButton* addPortButton;
    QPushButton* removePortButton;
    QTableWidget* portTable;

    QLineEdit* makePortNameInput();
    QComboBox* makePortTypeInput();
    QPushButton* makePortButton(const QString& text, const bool enabled);
    QPushButton* makeRemovePortButton();
    QTableWidget* makePortTable();
    bool tableHasName(const QString& name);
    void addPort(const QString& name, const QString& type);

    protected Q_SLOTS:
    void portNameInputChanged(const QString& currentText);
    void addPortClicked();
    void removePortClicked();

    public:
    explicit PortEditor(QWidget* parent = nullptr);
    void addType(const QString& name);
    std::vector<std::pair<QString, QString>> ports();
};

class CreateObjectDialog : public QDialog
{
    Q_OBJECT

    Ui::CreateObjectDialog *ui;
    std::vector<std::string> outputTypes;
    std::vector<std::string> inputTypes;

    void initObject();
    void initTabs();
    bool dialogNeeded();

    protected Q_SLOTS:
    void editorReady(bool);

    public:
    const Clypsalot::ObjectDescriptor& descriptor;
    const Clypsalot::SharedObject object;

    CreateObjectDialog(QWidget* parent, const Clypsalot::ObjectDescriptor& descriptor);
    ~CreateObjectDialog();
    Clypsalot::ObjectConfig config();
    std::vector<std::pair<QString, QString>> outputs();
    std::vector<std::pair<QString, QString>> inputs();
};
