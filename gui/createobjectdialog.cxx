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

// Drag and drop rows in QTableWidget https://bugreports.qt.io/browse/QTBUG-13873?page=com.atlassian.jira.plugin.system.issuetabpanels%3Aall-tabpanel

#include <QHeaderView>

#include <clypsalot/module.hxx>
#include <clypsalot/object.hxx>

#include "createobjectdialog.hxx"
#include "data.hxx"
#include "logger.hxx"
#include "ui_createobjectdialog.h"

static const int propertiesTabIndex = 0;
static const int outputsTabIndex = 1;
static const int inputsTabIndex = 2;
static const int portTableTypeIndex = 0;
static const int portTableNameIndex = 1;

PortEditor::PortEditor(QWidget* parent) :
    QWidget(parent)
{
    m_mainLayout = new QVBoxLayout(this);
    m_topLayout = new QHBoxLayout();
    m_mainLayout->addLayout(m_topLayout);

    m_portNameInput = makePortNameInput();
    m_portTypeInput = makePortTypeInput();
    m_addPortButton = makePortButton("Add", false);
    m_removePortButton = makePortButton("Remove", true);
    m_topLayout->addWidget(m_portTypeInput);
    m_topLayout->addWidget(m_portNameInput);
    m_topLayout->addWidget(m_addPortButton);
    m_topLayout->addWidget(m_removePortButton);

    m_portTable = makePortTable();
    m_mainLayout->addWidget(m_portTable);

    connect(m_portNameInput, SIGNAL(textChanged(const QString&)), this, SLOT(portNameInputChanged(const QString&)));
    connect(m_addPortButton, SIGNAL(clicked()), this, SLOT(addPortClicked()));
    connect(m_removePortButton, SIGNAL(clicked()), this, SLOT(removePortClicked()));
}

QComboBox* PortEditor::makePortTypeInput()
{
    auto input = new QComboBox();
    input->setToolTip("Port Type");
    return input;
}

void PortEditor::addType(const QString& name)
{
    m_portTypeInput->addItem(name);
}

std::vector<std::pair<QString, QString>> PortEditor::ports()
{
    std::vector<std::pair<QString, QString>> vector;
    auto numRows = m_portTable->rowCount();

    vector.reserve(numRows);

    for (int row = 0; row < numRows; row++)
    {
        auto type = m_portTable->item(row, portTableTypeIndex)->text();
        auto name = m_portTable->item(row, portTableNameIndex)->text();
        vector.emplace_back(type, name);
    }

    return vector;
}

QPushButton* PortEditor::makePortButton(const QString& text, const bool enabled)
{
    auto button = new QPushButton(text);
    button->setEnabled(enabled);
    return button;
}

QLineEdit* PortEditor::makePortNameInput()
{
    auto input = new RegexValidatingLineEdit(QRegularExpression("\\w[\\w\\s]*"));
    input->setToolTip("Port Name");
    return input;
}

void PortEditor::portNameInputChanged(const QString& currentText)
{
    auto text = currentText.trimmed();
    bool addPortButtonEnabled = true;

    if (tableHasName(text)) addPortButtonEnabled = false;
    else if (currentText == "") addPortButtonEnabled = false;

    m_addPortButton->setEnabled(addPortButtonEnabled);
}

QTableWidget* PortEditor::makePortTable()
{
    auto table = new QTableWidget();
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels(QStringList({ "Type", "Name" }));
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setSectionsMovable(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    return table;
}

bool PortEditor::tableHasName(const QString& name)
{
    auto numRows = m_portTable->rowCount();

    for(int row = 0; row < numRows; row++)
    {
        if (m_portTable->item(row, portTableNameIndex)->text() == name) return true;
    }

    return false;
}

void PortEditor::addPortClicked()
{
    addPort(m_portNameInput->text().trimmed(), m_portTypeInput->currentText());
    m_portNameInput->clear();
}

void PortEditor::addPort(const QString& name, const QString& type)
{
    auto row = m_portTable->rowCount();

    m_portTable->insertRow(row);
    m_portTable->setItem(row, portTableTypeIndex, new QTableWidgetItem(type));
    m_portTable->setItem(row, portTableNameIndex, new QTableWidgetItem(name));
}

void PortEditor::removePortClicked()
{
    auto selectedRow = m_portTable->currentRow();

    if (selectedRow == -1) return;

    m_portTable->removeRow(selectedRow);
}

CreateObjectDialog::CreateObjectDialog(QWidget* parent, const Clypsalot::ObjectDescriptor& descriptor) :
    QDialog(parent),
    m_ui(new Ui::CreateObjectDialog),
    m_descriptor(descriptor),
    m_object(descriptor.make())
{
    LOGGER(trace, "init CreateObjectDialog for ", descriptor.kind);

    setWindowModality(Qt::WindowModal);

    m_ui->setupUi(this);
    m_ui->objectKindLabel->setText(QString::fromStdString(descriptor.kind));
    m_ui->createButton->setEnabled(false);

    connect(m_ui->objectProperties, &ObjectPropertiesEditor::ready, this, &CreateObjectDialog::editorReady);
    initObject();
    initTabs();

    if (! dialogNeeded())
    {
        LOGGER(debug, "Object does not need CreateObjectDialog");
        setVisible(false);
        // Calling accept() during construction does not seem to work
        QMetaObject::invokeMethod(this, "accept", Qt::QueuedConnection);
    }

    m_ui->objectProperties->checkReady();
}

CreateObjectDialog::~CreateObjectDialog()
{
    delete m_ui;
}

bool CreateObjectDialog::dialogNeeded()
{
    auto tabs = m_ui->tabs;
    auto numTabs = tabs->count();

    for (int tab = 0; tab < numTabs; tab++)
    {
        if (tabs->isTabEnabled(tab)) return true;
    }

    return false;
}

void CreateObjectDialog::initObject()
{
    auto editor = m_ui->objectProperties;
    std::scoped_lock objectLock(*m_object);

    m_outputTypes = m_object->addOutputTypes();
    m_inputTypes = m_object->addInputTypes();

    for (const auto& [name, property] : m_object->properties())
    {
        if (! property.hasFlag(Clypsalot::Property::Configurable)) continue;

        LOGGER(trace, "Adding property for editing: ", name);
        editor->addProperty(
                    QString::fromStdString(name),
                    property.type(),
                    property.hasFlag(Clypsalot::Property::Required),
                    property.anyValue());
    }
}

void CreateObjectDialog::initTabs()
{
    if (m_ui->objectProperties->numFields() == 0) m_ui->tabs->setTabEnabled(propertiesTabIndex, false);

    if (m_outputTypes.size() == 0)
    {
        m_ui->tabs->setTabEnabled(outputsTabIndex, false);
    }
    else
    {
        for (const auto& type : m_outputTypes)
        {
            m_ui->outputs->addType(QString::fromStdString(type));
        }
    }

    if (m_inputTypes.size() == 0)
    {
        m_ui->tabs->setTabEnabled(inputsTabIndex, false);
    }
    else
    {
        for (const auto& type : m_outputTypes)
        {
            m_ui->inputs->addType(QString::fromStdString(type));
        }
    }
}

void CreateObjectDialog::editorReady(const bool ready)
{
    m_ui->createButton->setEnabled(ready);
}

Clypsalot::ObjectConfig CreateObjectDialog::config()
{
    return m_ui->objectProperties->config();
}

std::vector<std::pair<QString, QString>> CreateObjectDialog::outputs()
{
    return m_ui->outputs->ports();
}

std::vector<std::pair<QString, QString>> CreateObjectDialog::inputs()
{
    return m_ui->inputs->ports();
}
