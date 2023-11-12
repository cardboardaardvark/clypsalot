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

#include "forward.hxx"
#include "workarea.hxx"

class ObjectPort : public WorkAreaWidget
{
    Q_OBJECT

    WorkAreaLabelWidget* m_nameLabel = nullptr;

    protected:
    QList<PortConnection*> m_connections;

    public:
    Object* const m_parentObject;

    ObjectPort(Object* const in_parentObject, const QString& in_name, QGraphicsItem* in_parent = nullptr);
    ~ObjectPort();
    QString name();
    const QList<PortConnection*>& connections() const;
    virtual QPointF connectPos() = 0;
    Object* parentObject() const;
    void addConnection(PortConnection* in_connection);
    void removeConnection(PortConnection* in_connection);
    void updateConnections();
};

class ObjectInput : public ObjectPort
{
    Q_OBJECT

    public:
    ObjectInput(Object* const parentObject, const QString& name, QGraphicsItem* parent = nullptr);
    int type() const override;
    QPointF connectPos() override;
};

class ObjectOutput : public ObjectPort
{
    Q_OBJECT

    protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

    public:
    ObjectOutput(Object* const parentObject, const QString& name, QGraphicsItem* parent = nullptr);
    QPointF connectPos() override;
    void createConnection(ObjectInput* to);
};

class PortConnection : public WorkAreaWidget
{
    Q_OBJECT

    ObjectOutput* const m_from = nullptr;
    ObjectInput* const m_to = nullptr;
    WorkAreaLineWidget* m_line = nullptr;

    void link();
    void unlink();

    protected Q_SLOTS:
    void updateSelected(const bool in_selected);

    public:
    static QColor colorForStates(const Clypsalot::ObjectState in_fromState, const Clypsalot::ObjectState in_toState);
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
    Clypsalot::ObjectState m_state;
    std::vector<std::pair<std::string, std::string>> m_propertyValues;
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
    std::atomic_bool m_checkObjectSignalNeeded = ATOMIC_VAR_INIT(true);

    QList<Object*> objectCollisions() const noexcept;
    void ensureOnTop() noexcept;

    protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* in_event) noexcept override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* in_event) override;
    void handleEvent(const Clypsalot::ObjectStateChangedEvent& event);
    void initObject(QGraphicsLinearLayout* inputsLayout, QGraphicsLinearLayout* outputsLayout, QGraphicsGridLayout* propertiesLayout);
    QVariant itemChange(const GraphicsItemChange change, const QVariant& value) override;
    void updatePortConnections();

    Q_SIGNALS:
    void checkObject();

    protected Q_SLOTS:
    void updateSelected(const bool in_selected);
    void updateObject();

    public:
    Object(const Clypsalot::SharedObject& object, QGraphicsItem* parent = nullptr);
    ~Object();
    int type() const override;
    const Clypsalot::SharedObject& object();
    Clypsalot::ObjectState state() const;

    public Q_SLOTS:
    void start();
    void pause();
    void stop();
};
