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
#include "logging.hxx"
#include "ui_createobjectdialog.h"

static const int propertiesTabIndex = 0;
static const int outputsTabIndex = 1;
static const int inputsTabIndex = 2;
static const int portTableTypeIndex = 0;
static const int portTableNameIndex = 1;

PortEditor::PortEditor(QWidget* parent) :
    QWidget(parent)
{
    mainLayout = new QVBoxLayout(this);
    topLayout = new QHBoxLayout();
    mainLayout->addLayout(topLayout);

    portNameInput = makePortNameInput();
    portTypeInput = makePortTypeInput();
    addPortButton = makePortButton("Add", false);
    removePortButton = makePortButton("Remove", true);
    topLayout->addWidget(portTypeInput);
    topLayout->addWidget(portNameInput);
    topLayout->addWidget(addPortButton);
    topLayout->addWidget(removePortButton);

    portTable = makePortTable();
    mainLayout->addWidget(portTable);

    connect(portNameInput, SIGNAL(textChanged(const QString&)), this, SLOT(portNameInputChanged(const QString&)));
    connect(addPortButton, SIGNAL(clicked()), this, SLOT(addPortClicked()));
    connect(removePortButton, SIGNAL(clicked()), this, SLOT(removePortClicked()));
}

QComboBox* PortEditor::makePortTypeInput()
{
    auto input = new QComboBox();
    input->setToolTip("Port Type");
    return input;
}

void PortEditor::addType(const QString& name)
{
    portTypeInput->addItem(name);
}

std::vector<std::pair<QString, QString>> PortEditor::ports()
{
    std::vector<std::pair<QString, QString>> vector;
    auto numRows = portTable->rowCount();

    vector.reserve(numRows);

    for (int row = 0; row < numRows; row++)
    {
        auto type = portTable->item(row, portTableTypeIndex)->text();
        auto name = portTable->item(row, portTableNameIndex)->text();
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

    addPortButton->setEnabled(addPortButtonEnabled);
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
    auto numRows = portTable->rowCount();

    for(int row = 0; row < numRows; row++)
    {
        if (portTable->item(row, portTableNameIndex)->text() == name) return true;
    }

    return false;
}

void PortEditor::addPortClicked()
{
    addPort(portNameInput->text().trimmed(), portTypeInput->currentText());
    portNameInput->clear();
}

void PortEditor::addPort(const QString& name, const QString& type)
{
    auto row = portTable->rowCount();

    portTable->insertRow(row);
    portTable->setItem(row, portTableTypeIndex, new QTableWidgetItem(type));
    portTable->setItem(row, portTableNameIndex, new QTableWidgetItem(name));
}

void PortEditor::removePortClicked()
{
    auto selectedRow = portTable->currentRow();

    if (selectedRow == -1) return;

    portTable->removeRow(selectedRow);
}

CreateObjectDialog::CreateObjectDialog(QWidget* parent, const Clypsalot::ObjectDescriptor& descriptor) :
    QDialog(parent),
    ui(new Ui::CreateObjectDialog),
    descriptor(descriptor),
    object(descriptor.make())
{
    LOGGER(trace, "init CreateObjectDialog for ", descriptor.kind);

    setWindowModality(Qt::WindowModal);

    ui->setupUi(this);
    ui->objectKindLabel->setText(QString::fromStdString(descriptor.kind));
    ui->createButton->setEnabled(false);

    connect(ui->objectProperties, &ObjectPropertiesEditor::ready, this, &CreateObjectDialog::editorReady);
    initObject();
    initTabs();

    if (! dialogNeeded())
    {
        LOGGER(debug, "Object does not need CreateObjectDialog");
        setVisible(false);
        // Calling accept() during construction does not seem to work
        QMetaObject::invokeMethod(this, "accept", Qt::QueuedConnection);
    }

    ui->objectProperties->checkReady();
}

CreateObjectDialog::~CreateObjectDialog()
{
    delete ui;
}

bool CreateObjectDialog::dialogNeeded()
{
    auto tabs = ui->tabs;
    auto numTabs = tabs->count();

    for (int tab = 0; tab < numTabs; tab++)
    {
        if (tabs->isTabEnabled(tab)) return true;
    }

    return false;
}

void CreateObjectDialog::initObject()
{
    auto editor = ui->objectProperties;
    std::scoped_lock objectLock(*object);

    outputTypes = object->addOutputTypes();
    inputTypes = object->addInputTypes();

    for (const auto& [name, property] : object->properties())
    {
        if (! property.configurable) continue;

        LOGGER(trace, "Adding property for editing: ", name);
        editor->addProperty(QString::fromStdString(name), property.type, property.required, property.anyValue());
    }
}

void CreateObjectDialog::initTabs()
{
    if (ui->objectProperties->numFields() == 0) ui->tabs->setTabEnabled(propertiesTabIndex, false);

    if (outputTypes.size() == 0)
    {
        ui->tabs->setTabEnabled(outputsTabIndex, false);
    }
    else
    {
        for (const auto& type : outputTypes)
        {
            ui->outputs->addType(QString::fromStdString(type));
        }
    }

    if (inputTypes.size() == 0)
    {
        ui->tabs->setTabEnabled(inputsTabIndex, false);
    }
    else
    {
        for (const auto& type : outputTypes)
        {
            ui->inputs->addType(QString::fromStdString(type));
        }
    }
}

void CreateObjectDialog::editorReady(const bool ready)
{
    ui->createButton->setEnabled(ready);
}

Clypsalot::ObjectConfig CreateObjectDialog::config()
{
    return ui->objectProperties->config();
}

std::vector<std::pair<QString, QString>> CreateObjectDialog::outputs()
{
    return ui->outputs->ports();
}

std::vector<std::pair<QString, QString>> CreateObjectDialog::inputs()
{
    return ui->inputs->ports();
}
