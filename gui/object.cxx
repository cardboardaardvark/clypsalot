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

#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QWindow>

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

ObjectPort::ObjectPort(Object* const in_parentObject, const Clypsalot::Port& in_port, QGraphicsItem* in_parent) :
    WorkAreaWidget(in_parent),
    m_parentObject(in_parentObject)
{
    auto layout = new QGraphicsLinearLayout(Qt::Horizontal);
    setLayout(layout);

    m_nameLabel = new WorkAreaLabelWidget(QString::fromStdString(in_port.name()));
    m_nameLabel->setFlag(ItemStacksBehindParent);
    layout->addItem(m_nameLabel);
}

ObjectPort::~ObjectPort()
{
    LOGGER(debug, "ObjectPort::~ObjectPort()");

    // Need a copy because connectionDestroyed() will wind up invalidating iterators when it is
    // invoked because of the delete.
    auto copy = connections();

    for (auto connection : copy)
    {
        LOGGER(debug, "Deleting connection");
        delete connection;
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

const QList<QPointer<PortConnection>>& ObjectPort::connections() const
{
    return m_connections;
}

void ObjectPort::addConnection(PortConnection* in_connection)
{
    connect(in_connection, SIGNAL(destroyed(QObject*)), this, SLOT(connectionDestroyed(QObject*)));
    m_connections.append(in_connection);
}

void ObjectPort::connectionDestroyed(QObject* in_connection)
{
    LOGGER(debug, "ObjectPort::connectionDestroyed(QObject*)");
    removeConnection(static_cast<PortConnection*>(in_connection));
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
    for (auto connection : m_connections)
    {
        if (! connection) continue;
        connection->update();
    }
}

ObjectInput::ObjectInput(Object* const in_parentObject, Clypsalot::InputPort& in_port, QGraphicsItem* in_parent) :
    ObjectPort(in_parentObject, in_port, in_parent),
    m_inputPort(in_port)
{ }

int ObjectInput::type() const
{
    return static_cast<int>(WorkAreaItemType::inputPort);
}

Clypsalot::InputPort& ObjectInput::port() const
{
    return m_inputPort;
}

QPointF ObjectInput::connectPos() const
{
    auto mySize = size();
    return {0, mySize.height() / 2};
}

ObjectOutput::ObjectOutput(Object* const in_parentObject, Clypsalot::OutputPort& in_port, QGraphicsItem* in_parent) :
    ObjectPort(in_parentObject, in_port, in_parent),
    m_outputPort(in_port)
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
    auto item = workAreaItemAt(WorkAreaItemType::inputPort, cursorPos);

    if (item == nullptr)
    {
        workArea->updateConnectionDrag(lineStart, cursorPos, m_parentObject->borderColor(), true);
        return;
    }

    auto input = dynamic_cast<ObjectInput*>(item);
    assert(input != nullptr);
    auto color = colorForStates({m_parentObject->state(), input->parentObject()->state()});
    workArea->updateConnectionDrag(lineStart, input->mapToScene(input->connectPos()), color, false);
}

void ObjectOutput::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    MainWindow::instance()->workArea()->resetConnectionDrag();

    auto scenePos = event->scenePos();
    auto item = workAreaItemAt(WorkAreaItemType::inputPort, scenePos);

    if (item == nullptr) return;

    auto input = dynamic_cast<ObjectInput*>(item);
    assert(input != nullptr);
    createConnection(input);
}

Clypsalot::OutputPort& ObjectOutput::port() const
{
    return m_outputPort;
}

QPointF ObjectOutput::connectPos() const
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
    MainWindow::instance()->workArea()->scene()->removeItem(this);
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

    m_line->setColor(colorForStates({fromState, toState}));
}

void PortConnection::link()
{
    THREAD_CALL
    ({
         std::scoped_lock fromLock(*m_from->parentObject()->object());
         std::scoped_lock toLock(*m_to->parentObject()->object());
         Clypsalot::linkPorts(m_from->port(), m_to->port());
    });
}

void PortConnection::unlink()
{
    THREAD_CALL
    ({
         std::scoped_lock fromLock(*m_from->parentObject()->object());
         std::scoped_lock toLock(*m_to->parentObject()->object());
         Clypsalot::unlinkPorts(m_from->port(), m_to->port());
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

    connect(this, SIGNAL(checkObject()), this, SLOT(scheduleUpdateObject()), Qt::QueuedConnection);
    connect(this, SIGNAL(selectedChanged(bool)), this, SLOT(updateSelected(bool)));

    initObject(inputsLayout, outputsLayout, propertiesLayout);
    updateSelected(false);
    updateObject();
}

Object::~Object()
{
    LOGGER(debug, "In Object::~Object()");

    // The ports have to be removed before m_object is destroyed otherwise the Clypsalot::SharedObject
    // deleter will unlink the ports then when the destructor for ObjectPort runs it'll try to
    // unlink the ports again.
    for (auto port : m_outputs)
    {
        delete port;
    }

    for (auto port : m_inputs)
    {
        delete port;
    }
}

void Object::initObject(QGraphicsLinearLayout* inputsLayout, QGraphicsLinearLayout* outputsLayout, QGraphicsGridLayout* propertiesLayout)
{
    std::scoped_lock lock(*m_object);

    m_subscriptions.push_back(m_object->subscribe<Clypsalot::ObjectStateChangedEvent>(std::bind(&Object::handleEvent, this, _1)));
    m_info->m_kind->setText(QString::fromStdString(m_object->kind()));

    for (auto port : m_object->inputs())
    {
        auto input = new ObjectInput(this, *port);
        m_inputs.push_back(input);
        input->setBorderWidth(portBorderWidth);
        inputsLayout->addItem(input);
    }

    for (auto port : m_object->outputs())
    {
        auto output = new ObjectOutput(this, *port);
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

QVariant Object::itemChange(const GraphicsItemChange in_change, const QVariant& in_value)
{
    if (in_change == ItemPositionChange)
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
    else if (in_change == ItemPositionHasChanged)
    {
        ensureOnTop();
        updatePortConnections();
    }

    return WorkAreaWidget::itemChange(in_change, in_value);
}

// This is called from inside the Clypsalot thread queue not the UI thread.
void Object::handleEvent(const Clypsalot::ObjectStateChangedEvent&)
{
    if (! m_needsUpdate.load(std::memory_order_relaxed))
    {
        m_needsUpdate.store(true, std::memory_order_relaxed);
        Q_EMIT checkObject();
    }
}

void Object::scheduleUpdateObject()
{
    LOGGER(debug, "Requesting update from work area");
    MainWindow::instance()->workArea()->scheduleUpdateObjects();
}

void Object::updateObject()
{
    struct ObjectUpdate updateData;

    m_needsUpdate.store(false, std::memory_order_relaxed);

    THREAD_CALL
    ({
         std::lock_guard lock(*m_object);
         const auto& properties = m_object->properties();
         auto& propertyValues = updateData.propertyValues;

         // It would be better to allocate memory outside of the thread queue but
         // getting the number of properties requires the object be locked.
         propertyValues.reserve(properties.size());
         updateData.state = m_object->state();

         for (const auto& [name, property] : properties)
         {
             if (property.defined()) propertyValues.emplace_back(name, property.variant());
         }
    });

    for (const auto& [name, value] : updateData.propertyValues)
    {
        auto qName = QString::fromStdString(name);
        if (! m_propertyValues.contains(qName)) continue;
        m_propertyValues[qName]->setText(QString::fromStdString(Clypsalot::toString(value)));
    }

    setState(updateData.state);
    updatePortConnections();
}

bool Object::needsUpdate() const
{
    return m_needsUpdate.load(std::memory_order_relaxed);
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

void Object::setState(const Clypsalot::ObjectState in_state) noexcept
{
    m_state = in_state;
    m_info->m_state->setText(QString::fromStdString(Clypsalot::toString(in_state)));
    setBorderColor(colorForStates({in_state}));
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

static bool _hasState(const std::initializer_list<Clypsalot::ObjectState>& in_states, const Clypsalot::ObjectState in_state)
{
    for (auto state : in_states)
    {
        if (state == in_state) return true;
    }

    return false;
}

QColor colorForStates(const std::initializer_list<Clypsalot::ObjectState>& in_states) noexcept
{
    if (_hasState(in_states, Clypsalot::ObjectState::faulted)) return objectFaultedColor;
    if (_hasState(in_states, Clypsalot::ObjectState::stopped)) return objectStoppedColor;
    if (_hasState(in_states, Clypsalot::ObjectState::paused)) return objectPausedColor;

    for (auto state : in_states)
    {
        if (Clypsalot::objectIsActive(state)) return objectActiveColor;
    }

    LOGGER(warn, "Unexpected object states");

    for (auto state : in_states)
    {
        LOGGER(debug, "Unhandled state: ", state);
    }

    return MainWindow::instance()->workArea()->scene()->palette().color(QPalette::Active, QPalette::WindowText);
}
