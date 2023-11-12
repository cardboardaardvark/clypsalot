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
    QVBoxLayout* m_mainLayout = nullptr;
    QHBoxLayout* m_topLayout = nullptr;
    QLineEdit* m_portNameInput = nullptr;
    QComboBox* m_portTypeInput = nullptr;
    QPushButton* m_addPortButton = nullptr;
    QPushButton* m_removePortButton = nullptr;
    QTableWidget* m_portTable = nullptr;

    QLineEdit* makePortNameInput();
    QComboBox* makePortTypeInput();
    QPushButton* makePortButton(const QString& in_text, const bool in_enabled);
    QPushButton* makeRemovePortButton();
    QTableWidget* makePortTable();
    bool tableHasName(const QString& in_name);
    void addPort(const QString& in_name, const QString& in_type);

    protected Q_SLOTS:
    void portNameInputChanged(const QString& in_currentText);
    void addPortClicked();
    void removePortClicked();

    public:
    explicit PortEditor(QWidget* in_parent = nullptr);
    void addType(const QString& in_name);
    std::vector<std::pair<QString, QString>> ports();
};

class CreateObjectDialog : public QDialog
{
    Q_OBJECT

    Ui::CreateObjectDialog* m_ui;
    Clypsalot::SharedObject m_object;
    std::vector<std::string> m_outputTypes;
    std::vector<std::string> m_inputTypes;

    void initObject();
    void initTabs();
    bool dialogNeeded();

    protected Q_SLOTS:
    void editorReady(bool);

    public:
    const Clypsalot::ObjectDescriptor& m_descriptor;

    CreateObjectDialog(QWidget* in_parent, const Clypsalot::ObjectDescriptor& in_descriptor);
    ~CreateObjectDialog();
    Clypsalot::SharedObject object();
    Clypsalot::ObjectConfig config();
    std::vector<std::pair<QString, QString>> outputs();
    std::vector<std::pair<QString, QString>> inputs();
};
