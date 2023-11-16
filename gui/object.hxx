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

#include <atomic>

#include <QGraphicsGridLayout>
#include <QGraphicsLinearLayout>
#include <QGraphicsObject>
#include <QPointer>

#include <clypsalot/forward.hxx>
#include <clypsalot/property.hxx>

#include "forward.hxx"
#include "workarea.hxx"

class ObjectPort : public WorkAreaWidget
{
    Q_OBJECT

    WorkAreaLabelWidget* m_nameLabel = nullptr;

    protected:
    Object* const m_parentObject;
    QList<QPointer<PortConnection>> m_connections;

    ObjectPort(Object* const in_parentObject, const Clypsalot::Port& in_port, QGraphicsItem* in_parent = nullptr);

    protected Q_SLOTS:
    void connectionDestroyed(QObject* in_connection);
    public:
    ~ObjectPort();
    QString name();
    const QList<QPointer<PortConnection>>& connections() const;
    virtual QPointF connectPos() const = 0;
    Object* parentObject() const;
    void addConnection(PortConnection* in_connection);
    void removeConnection(PortConnection* in_connection);
    void updateConnections();
};

class ObjectInput : public ObjectPort
{
    Q_OBJECT

    Clypsalot::InputPort& m_inputPort;

    public:
    ObjectInput(Object* const parentObject, Clypsalot::InputPort& in_port, QGraphicsItem* in_parent = nullptr);
    int type() const override;
    Clypsalot::InputPort& port() const;
    QPointF connectPos() const override;
};

class ObjectOutput : public ObjectPort
{
    Q_OBJECT

    Clypsalot::OutputPort& m_outputPort;

    protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

    public:
    ObjectOutput(Object* const parentObject, Clypsalot::OutputPort& in_port, QGraphicsItem* in_parent = nullptr);
    Clypsalot::OutputPort& port() const;
    QPointF connectPos() const override;
    void createConnection(ObjectInput* in_to);
};

class PortConnection : public WorkAreaWidget
{
    Q_OBJECT

    const QPointer<ObjectOutput> m_from;
    const QPointer<ObjectInput>  m_to;
    WorkAreaLineWidget* m_line = nullptr;

    void link();
    void unlink();

    protected Q_SLOTS:
    void updateSelected(const bool in_selected);

    public:
    PortConnection(ObjectOutput* const from, ObjectInput* const to, QGraphicsObject* parent = nullptr);
    ~PortConnection() noexcept;
    QPainterPath shape() const override;
    QRectF boundingRect() const override;
    void setLineStyle(const Qt::PenStyle in_style);
    int type() const override;
    ObjectOutput* from() const;
    ObjectInput* to() const;
    void update();
};

class ObjectInfo : public WorkAreaWidget
{
    Q_OBJECT

    public:
    WorkAreaLabelWidget* const m_kind;
    WorkAreaLabelWidget* const m_state;

    explicit ObjectInfo(QGraphicsItem* parent = nullptr);
};

struct ObjectUpdate
{
    Clypsalot::ObjectState state;
    std::vector<std::pair<std::string, Clypsalot::Property::Variant>> propertyValues;
};

class Object : public WorkAreaWidget
{
    Q_OBJECT

    const Clypsalot::SharedObject m_object;
    Clypsalot::ObjectState m_state;
    ObjectInfo* m_info;
    std::map<QString, WorkAreaLabelWidget*> m_propertyValues;
    QList<ObjectOutput*> m_outputs;
    QList<ObjectInput*> m_inputs;
    std::vector<std::shared_ptr<Clypsalot::Subscription>> m_subscriptions;
    std::atomic_bool m_needsUpdate = false;

    QList<Object*> objectCollisions() const noexcept;
    void ensureOnTop() noexcept;

    Q_SIGNALS:
    void checkObject();

    protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* in_event) noexcept override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* in_event) override;
    void handleEvent(const Clypsalot::ObjectStateChangedEvent& event);
    void initObject(QGraphicsLinearLayout* inputsLayout, QGraphicsLinearLayout* outputsLayout, QGraphicsGridLayout* propertiesLayout);
    QVariant itemChange(const GraphicsItemChange change, const QVariant& value) override;
    void updatePortConnections();
    void setState(const Clypsalot::ObjectState in_state) noexcept;

    protected Q_SLOTS:
    void updateSelected(const bool in_selected);
    void scheduleUpdateObject();

    public:
    Object(const Clypsalot::SharedObject& object, QGraphicsItem* parent = nullptr);
    ~Object();
    bool needsUpdate() const;
    void updateObject();
    int type() const override;
    const Clypsalot::SharedObject& object();
    Clypsalot::ObjectState state() const;

    public Q_SLOTS:
    void start();
    void pause();
    void stop();
};

QColor colorForStates(const std::initializer_list<Clypsalot::ObjectState>& in_states) noexcept;
