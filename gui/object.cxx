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

#include <memory>

#include <QGraphicsSceneMouseEvent>

#include <clypsalot/macros.hxx>
#include <clypsalot/object.hxx>
#include <clypsalot/port.hxx>
#include <clypsalot/property.hxx>
#include <clypsalot/thread.hxx>

#include "logger.hxx"
#include "mainwindow.hxx"
#include "object.hxx"

using namespace std::placeholders;

static const qreal infoBorderWidth = 2;
static const qreal portBorderWidth = 1;

ObjectPort::ObjectPort(Object* const parentObject, const QString& name, QGraphicsItem* parent) :
    WorkAreaWidget(parent),
    m_parentObject(parentObject)
{
    setFlag(ItemSendsGeometryChanges);

    auto layout = new QGraphicsLinearLayout(Qt::Horizontal);
    setLayout(layout);

    m_nameLabel = new WorkAreaLabelWidget(name);
    m_nameLabel->setFlag(ItemStacksBehindParent);
    layout->addItem(m_nameLabel);
}

const QString ObjectPort::name()
{
    return m_nameLabel->text();
}

void ObjectPort::addConnection(PortConnection* connection)
{
    m_connections.append(connection);
}

void ObjectPort::updateConnectionPositions()
{
    for (auto connection : m_connections) connection->updatePosition();
}

ObjectInput::ObjectInput(Object* const parentObject, const QString& name, QGraphicsItem* parent) :
    ObjectPort(parentObject, name, parent)
{
    setAcceptDrops(true);
}

int ObjectInput::type() const
{
    return static_cast<int>(WorkAreaItemType::inputPort);
}

QPointF ObjectInput::connectPos()
{
    auto mySize = size();
    return {0, mySize.height() / 2};
}

ObjectOutput::ObjectOutput(Object* const parentObject, const QString& name, QGraphicsItem* parent) :
    ObjectPort(parentObject, name, parent)
{ }

void ObjectOutput::mousePressEvent(QGraphicsSceneMouseEvent *)
{
    MainWindow::instance()->workArea()->startConnectionDrag();
}

void ObjectOutput::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    auto workArea = MainWindow::instance()->workArea();
    auto lineStart = mapToScene(connectPos());
    auto cursorPos = mapToScene(event->pos());
    auto item = workArea->m_scene->itemAt(cursorPos, QTransform());

    if (item && static_cast<WorkAreaItemType>(item->type()) == WorkAreaItemType::inputPort)
    {
        auto input = dynamic_cast<ObjectInput*>(item);
        assert(input != nullptr);
        workArea->updateConnectionDrag(lineStart, input->mapToScene(input->connectPos()), false);
    }
    else workArea->updateConnectionDrag(lineStart, cursorPos, true);
}

void ObjectOutput::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    MainWindow::instance()->workArea()->resetConnectionDrag();

    auto lastPos = event->lastPos();
    auto item = MainWindow::instance()->workArea()->m_scene->itemAt(mapToScene(lastPos), QTransform());

    if (item && static_cast<WorkAreaItemType>(item->type()) == WorkAreaItemType::inputPort)
    {
        auto input = dynamic_cast<ObjectInput*>(item);
        assert(input != nullptr);
        createConnection(input);
    }
}

QPointF ObjectOutput::connectPos()
{
    auto mySize = size();
    return {mySize.width(), mySize.height() / 2};
}

void ObjectOutput::createConnection(ObjectInput* to)
{
    try {
        auto fromObject = m_parentObject->m_object;
        auto toObject = to->m_parentObject->m_object;

        THREAD_CALL
        ({
             std::scoped_lock fromLock(*fromObject);
             std::scoped_lock toLock(*toObject);
             auto& fromPort = fromObject->output(name().toStdString());
             auto& toPort = toObject->input(to->name().toStdString());

             Clypsalot::linkPorts(fromPort, toPort);
         });
    }
    catch (const Clypsalot::Error& e)
    {
        LOGGER(error, "Could not create a connection: ", e.what());
        MainWindow::instance()->errorMessage(QString::fromStdString(e.what()));
        return;
    }

    auto connection = new PortConnection(this, to);
    addConnection(connection);
    to->addConnection(connection);
}

PortConnection::PortConnection(ObjectOutput* const from, ObjectInput* const to, QGraphicsObject* parent) :
    QGraphicsWidget(parent),
    m_from(from),
    m_to(to),
    m_line(QLineF(), this)
{
    updatePosition();
    MainWindow::instance()->workArea()->m_scene->addItem(this);
}

void PortConnection::updatePosition()
{
    m_line.setLine
    ({
        mapFromItem(m_from, m_from->connectPos()),
        mapFromItem(m_to, m_to->connectPos()),
    });
}

ObjectInfo::ObjectInfo(QGraphicsItem* parent) :
    WorkAreaWidget(parent),
    m_kind(new WorkAreaLabelWidget()),
    m_state(new WorkAreaLabelWidget())
{
    setBorderWidth(infoBorderWidth);

    auto layout = new QGraphicsLinearLayout(Qt::Vertical);

    setLayout(layout);

    layout->addItem(m_kind);
    layout->setAlignment(m_kind, Qt::AlignHCenter);

    layout->addItem(m_state);
    layout->setAlignment(m_state, Qt::AlignHCenter);
}

Object::Object(const Clypsalot::SharedObject& object, QGraphicsItem* parent) :
    WorkAreaWidget(parent),
    m_object(object)
{
    setFlag(ItemIsMovable);
    setAutoFillBackground(true);

    auto mainLayout = new QGraphicsLinearLayout(Qt::Horizontal);
    auto inputsLayout = new QGraphicsLinearLayout(Qt::Vertical);
    auto infoLayout = new QGraphicsLinearLayout(Qt::Vertical);
    auto outputsLayout = new QGraphicsLinearLayout(Qt::Vertical);
    auto propertiesLayout = new QGraphicsGridLayout();

    setLayout(mainLayout);

    mainLayout->addItem(inputsLayout);

    m_info = new ObjectInfo();
    infoLayout->addItem(m_info);
    infoLayout->addItem(propertiesLayout);
    mainLayout->addItem(infoLayout);

    mainLayout->addItem(outputsLayout);

    connect(this, SIGNAL(checkObject()), this, SLOT(updateObject()), Qt::QueuedConnection);
    connect(this, SIGNAL(selectedChanged(bool)), this, SLOT(updateSelected(bool)));

    initObject(inputsLayout, outputsLayout, propertiesLayout);
    updateObject();
}

void Object::initObject(QGraphicsLinearLayout* inputsLayout, QGraphicsLinearLayout* outputsLayout, QGraphicsGridLayout* propertiesLayout)
{
    std::scoped_lock lock(*m_object);

    m_subscriptions.push_back(m_object->subscribe<Clypsalot::ObjectStateChangedEvent>(std::bind(&Object::handleEvent, this, _1)));
    m_info->m_kind->setText(QString::fromStdString(m_object->kind()));

    for (auto port : m_object->inputs())
    {
        auto inputName = QString::fromStdString(port->name());
        auto input = new ObjectInput(this, inputName);
        input->setBorderWidth(portBorderWidth);
        inputsLayout->addItem(input);
    }

    for (auto port : m_object->outputs())
    {
        auto outputName = QString::fromStdString(port->name());
        auto output = new ObjectOutput(this, outputName);
        output->setBorderWidth(portBorderWidth);
        outputsLayout->addItem(output);
    }

    uint rowNum = 0;

    for (const auto& [name, property] : m_object->properties())
    {
        auto qName = QString::fromStdString(name);
        auto nameLabel = new WorkAreaLabelWidget(qName);
        auto valueLabel = new WorkAreaLabelWidget();
        propertiesLayout->addItem(nameLabel, rowNum, 0);
        propertiesLayout->addItem(valueLabel, rowNum, 1);
        m_propertyValues[qName] = valueLabel;

        rowNum++;
    }
}

// This is called from inside the Clypsalot thread queue not the UI thread.
void Object::handleEvent(const Clypsalot::ObjectStateChangedEvent&)
{
    if (m_checkObjectSignalNeeded)
    {
        m_checkObjectSignalNeeded = false;
        Q_EMIT checkObject();
    }
}

QVariant Object::itemChange(const GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionChange && scene())
    {
        auto position = value.toPointF();
        auto rectangle = scene()->sceneRect();

        if (! rectangle.contains(position))
        {
            // Keep all sides of the widget in the bounds of the scene
            position.setX(qMin(rectangle.right(), qMax(position.x(), rectangle.left())));
            position.setY(qMin(rectangle.bottom(), qMax(position.y(), rectangle.top())));
            return position;
        }
    }
    else if (change == ItemPositionHasChanged && scene())
    {
        updatePortConnectionPositions();
    }

    return WorkAreaWidget::itemChange(change, value);
}

void Object::updateObject()
{
    m_checkObjectSignalNeeded = true;

    auto update = THREAD_CALL
    ({
         auto retval = std::make_unique<ObjectUpdate>();
         std::lock_guard lock(*m_object);
         const auto& properties = m_object->properties();
         auto& propertyValues = retval->m_propertyValues;

         retval->m_state = m_object->state();
         propertyValues.reserve(properties.size());

         for (const auto& [name, property] : properties)
         {
             if (property.defined()) propertyValues.emplace_back(name, property.valueToString());
         }

         return retval;
     });

     m_info->m_state->setText(QString::fromStdString(Clypsalot::toString(update->m_state)));

    for (const auto& [name, value] : update->m_propertyValues)
    {
        auto qName = QString::fromStdString(name);
        if (! m_propertyValues.contains(qName)) continue;
        m_propertyValues[qName]->setText(QString::fromStdString(value));
    }
}

void Object::updatePortConnectionPositions()
{
    for (auto item : childItems())
    {
        auto port = dynamic_cast<ObjectPort*>(item);
        if (port == nullptr) continue;
        port->updateConnectionPositions();
    }
}

void Object::start()
{
    THREAD_CALL
    ({
         std::scoped_lock lock(*m_object);
         Clypsalot::startObject(m_object);
    });
}

void Object::pause()
{
    THREAD_CALL
    ({
        std::scoped_lock lock(*m_object);
        Clypsalot::pauseObject(m_object);
    });
}

void Object::stop()
{
    THREAD_CALL
    ({
        std::scoped_lock lock(*m_object);
        Clypsalot::stopObject(m_object);
    });
}
