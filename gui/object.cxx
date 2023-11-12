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

#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>

#include <clypsalot/macros.hxx>
#include <clypsalot/object.hxx>
#include <clypsalot/port.hxx>
#include <clypsalot/property.hxx>
#include <clypsalot/thread.hxx>

#include "logger.hxx"
#include "mainwindow.hxx"
#include "object.hxx"

using namespace std::placeholders;

static const qreal objectBorderWidth = 4;
static const qreal portBorderWidth = 1;
static const Qt::PenStyle normalLineStyle(Qt::SolidLine);
static const Qt::PenStyle selectedLineStyle(Qt::DotLine);

static const QColor objectActiveColor(Qt::green);
static const QColor objectFaultedColor(Qt::red);
static const QColor objectPausedColor(Qt::blue);
static const QColor objectStoppedColor(Qt::yellow);

ObjectPort::ObjectPort(Object* const in_parentObject, const QString& in_name, QGraphicsItem* in_parent) :
    WorkAreaWidget(in_parent),
    m_parentObject(in_parentObject)
{
    setFlag(ItemSendsGeometryChanges);

    auto layout = new QGraphicsLinearLayout(Qt::Horizontal);
    setLayout(layout);

    m_nameLabel = new WorkAreaLabelWidget(in_name);
    m_nameLabel->setFlag(ItemStacksBehindParent);
    layout->addItem(m_nameLabel);
}

ObjectPort::~ObjectPort()
{
    if (m_connections.size() > 0)
    {
        FATAL_ERROR("ObjectPort had connections during destruction");
    }
}

Object* ObjectPort::parentObject() const
{
    return m_parentObject;
}

QString ObjectPort::name()
{
    return m_nameLabel->text();
}

const QList<PortConnection*>& ObjectPort::connections() const
{
    return m_connections;
}

void ObjectPort::addConnection(PortConnection* in_connection)
{
    m_connections.append(in_connection);
}

void ObjectPort::removeConnection(PortConnection* in_connection)
{
    LOGGER(debug, "Removing connection");

    assert(in_connection != nullptr);

    for (auto i = m_connections.begin(); i != m_connections.end(); )
    {
        if (*i == in_connection)
        {
            LOGGER(debug, "Erasing connection");
            i = m_connections.erase(i);
        }
        else i++;
    }
}

void ObjectPort::updateConnections()
{
    for (auto connection : m_connections) connection->update();
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
        auto color = PortConnection::colorForStates(m_parentObject->state(), input->parentObject()->state());
        workArea->updateConnectionDrag(lineStart, input->mapToScene(input->connectPos()), color, false);
    }
    else workArea->updateConnectionDrag(lineStart, cursorPos, m_parentObject->borderColor(), true);
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
    PortConnection* connection = nullptr;

    try {
        connection = new PortConnection(this, to);
    }
    catch (const std::exception& e)
    {
        LOGGER(error, "Could not create a connection: ", e.what());
        MainWindow::instance()->errorMessage(QString::fromStdString(e.what()));
        return;
    }

    addConnection(connection);
    to->addConnection(connection);
}

QColor PortConnection::colorForStates(const Clypsalot::ObjectState in_fromState, const Clypsalot::ObjectState in_toState)
{
    if (in_fromState == Clypsalot::ObjectState::faulted || in_toState == Clypsalot::ObjectState::faulted)
    {
        return objectFaultedColor;
    }
    else if (in_fromState == Clypsalot::ObjectState::stopped || in_toState == Clypsalot::ObjectState::stopped)
    {
        return objectStoppedColor;
    }
    else if (in_fromState == Clypsalot::ObjectState::paused || in_toState == Clypsalot::ObjectState::paused)
    {
        return objectPausedColor;
    }
    else if (Clypsalot::objectIsActive(in_fromState) && Clypsalot::objectIsActive(in_toState))
    {
       return objectActiveColor;
    }

    LOGGER(warn, "Unexpected object states; from=", in_fromState, " to=", in_toState);
    return MainWindow::instance()->workArea()->scene()->palette().color(QPalette::Active, QPalette::WindowText);
}

PortConnection::PortConnection(ObjectOutput* const from, ObjectInput* const to, QGraphicsObject* parent) :
    WorkAreaWidget(parent),
    m_from(from),
    m_to(to)
{
    m_line = new WorkAreaLineWidget(QLineF(), this);

    setFlag(ItemIsSelectable);
    connect(this, SIGNAL(selectedChanged(bool)), this, SLOT(updateSelected(const bool)));

    // link() could throw an exception
    link();

    update();
}

PortConnection::~PortConnection() noexcept
{
    // unlink() could throw an exception but if that happens there is no obvious means
    // of recovery.
    unlink();
    m_from->removeConnection(this);
    m_to->removeConnection(this);
    MainWindow::instance()->workArea()->m_scene->removeItem(this);
}

QPainterPath PortConnection::shape() const
{
    return m_line->shape();
}

QRectF PortConnection::boundingRect() const
{
    return m_line->boundingRect();
}

void PortConnection::setLineStyle(const Qt::PenStyle in_style)
{
    m_line->setStyle(in_style);
}

int PortConnection::type() const
{
    return static_cast<int>(WorkAreaItemType::connection);
}

ObjectOutput* PortConnection::from() const
{
    return m_from;
}

ObjectInput* PortConnection::to() const
{
    return m_to;
}

void PortConnection::updateSelected(const bool in_selected)
{
    LOGGER(debug, "Connection selected: ", in_selected);

    if (in_selected) setLineStyle(selectedLineStyle);
    else setLineStyle(normalLineStyle);
}

void PortConnection::update()
{
    m_line->setLine
    ({
        mapFromItem(m_from, m_from->connectPos()),
        mapFromItem(m_to, m_to->connectPos()),
    });

    auto fromState = m_from->parentObject()->state();
    auto toState = m_to->parentObject()->state();

    m_line->setColor(colorForStates(fromState, toState));
}

void PortConnection::link()
{
    auto fromObject = m_from->parentObject()->object();
    auto fromName = m_from->name().toStdString();
    auto toObject = m_to->parentObject()->object();
    auto toName = m_to->name().toStdString();

    THREAD_CALL
    ({
         std::scoped_lock fromLock(*fromObject);
         std::scoped_lock toLock(*toObject);
         auto& fromPort = fromObject->output(fromName);
         auto& toPort = toObject->input(toName);

         Clypsalot::linkPorts(fromPort, toPort);
     });
}

void PortConnection::unlink()
{
    auto fromObject = m_from->parentObject()->object();
    auto fromName = m_from->name().toStdString();
    auto toObject = m_to->parentObject()->object();
    auto toName = m_to->name().toStdString();

    THREAD_CALL
    ({
         std::lock_guard fromLock(*fromObject);
         std::lock_guard toLock(*toObject);
         auto& fromPort = fromObject->output(fromName);
         auto& toPort = toObject->input(toName);

         Clypsalot::unlinkPorts(fromPort, toPort);
     });
}

ObjectInfo::ObjectInfo(QGraphicsItem* parent) :
    WorkAreaWidget(parent),
    m_kind(new WorkAreaLabelWidget()),
    m_state(new WorkAreaLabelWidget())
{
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
    setFlag(ItemIsSelectable);

    setAutoFillBackground(true);
    setBorderWidth(objectBorderWidth);

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
    updateSelected(false);
    updateObject();
}

Object::~Object()
{
    LOGGER(debug, "In Object::~Object()");

    for (auto output : m_outputs)
    {
        LOGGER(debug, "Cleaning up output: ", output->name());

        // Need a copy because the iterator will be invalidated
        auto connections = output->connections();

        for (auto connection : connections)
        {
            LOGGER(debug, "Removing connection to ", connection->to()->name());
            delete connection;
        }
    }

    for (auto input : m_inputs)
    {
        LOGGER(debug, "Cleaning up input: ", input->name());

        auto connections = input->connections();

        for (auto connection : connections)
        {
            LOGGER(debug, "Removing connection from ", connection->from()->name());
            delete connection;
        }
    }
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
        m_inputs.push_back(input);
        input->setBorderWidth(portBorderWidth);
        inputsLayout->addItem(input);
    }

    for (auto port : m_object->outputs())
    {
        auto outputName = QString::fromStdString(port->name());
        auto output = new ObjectOutput(this, outputName);
        m_outputs.push_back(output);
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

int Object::type() const
{
    return static_cast<int>(WorkAreaItemType::object);
}

QList<Object*> Object::objectCollisions() const noexcept
{
    QList<Object*> collisions;

    for (auto item : scene()->collidingItems(this))
    {
        if (static_cast<WorkAreaItemType>(item->type()) == WorkAreaItemType::object)
        {
            auto object = dynamic_cast<Object*>(item);
            assert(object != nullptr);
            collisions.push_back(object);
        }
    }

    return collisions;
}

void Object::ensureOnTop() noexcept
{
    for (auto collidedWith : objectCollisions())
    {
        collidedWith->stackBefore(this);
    }
}

const Clypsalot::SharedObject& Object::object()
{
    return m_object;
}

void Object::mousePressEvent(QGraphicsSceneMouseEvent* in_event) noexcept
{
    ensureOnTop();

    WorkAreaWidget::mousePressEvent(in_event);
}

void Object::contextMenuEvent(QGraphicsSceneContextMenuEvent* in_event)
{
    LOGGER(debug, "in context menu event handler");

    MainWindow::instance()->workArea()->scene()->clearSelection();
    setSelected(true);

    QMenu menu;
    QAction startAction("Start");
    QAction pauseAction("Pause");
    QAction stopAction("Stop");
    QAction removeAction("Remove");

    connect(&startAction, SIGNAL(triggered()), this, SLOT(start()));
    connect(&pauseAction, SIGNAL(triggered()), this, SLOT(pause()));
    connect(&stopAction, SIGNAL(triggered()), this, SLOT(stop()));
    connect(&removeAction, SIGNAL(triggered()), this, SLOT(deleteLater()));

    for (auto action : {&startAction, &pauseAction, &stopAction, &removeAction})
    {
        menu.addAction(action);
    }

    menu.exec(in_event->screenPos());

    ungrabMouse();
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

QVariant Object::itemChange(const GraphicsItemChange in_change, const QVariant& in_value)
{
    if (in_change == ItemPositionChange && scene())
    {
        auto position = in_value.toPointF();
        auto sceneRect = scene()->sceneRect();

        if (! sceneRect.contains(position))
        {
            // Keep all sides of the widget in the bounds of the scene
            position.setX(qMin(sceneRect.right(), qMax(position.x(), sceneRect.left())));
            position.setY(qMin(sceneRect.bottom(), qMax(position.y(), sceneRect.top())));
            return position;
        }
    }
    else if (in_change == ItemPositionHasChanged && scene())
    {
        ensureOnTop();
        updatePortConnections();
    }

    return WorkAreaWidget::itemChange(in_change, in_value);
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

    m_state = update->m_state;
    m_info->m_state->setText(QString::fromStdString(Clypsalot::toString(update->m_state)));

    if (update->m_state == Clypsalot::ObjectState::paused) setBorderColor(objectPausedColor);
    else if (Clypsalot::objectIsActive(update->m_state)) setBorderColor(objectActiveColor);
    else if (update->m_state == Clypsalot::ObjectState::stopped) setBorderColor(objectStoppedColor);
    else if (update->m_state == Clypsalot::ObjectState::faulted) setBorderColor(objectFaultedColor);
    else
    {
        LOGGER(warn, "Unexpected object state: ", update->m_state);
        setBorderColor(palette().color(QPalette::Active, QPalette::WindowText));
    }

    for (const auto& [name, value] : update->m_propertyValues)
    {
        auto qName = QString::fromStdString(name);
        if (! m_propertyValues.contains(qName)) continue;
        m_propertyValues[qName]->setText(QString::fromStdString(value));
    }

    updatePortConnections();
}

void Object::updatePortConnections()
{
    for (auto item : childItems())
    {
        auto port = dynamic_cast<ObjectPort*>(item);
        if (port == nullptr) continue;
        port->updateConnections();
    }
}

void Object::updateSelected(const bool in_selected)
{
    if (in_selected) setBorderStyle(selectedLineStyle);
    else setBorderStyle(normalLineStyle);
}

Clypsalot::ObjectState Object::state() const
{
    return m_state;
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
