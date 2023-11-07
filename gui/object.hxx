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

#include <QGraphicsGridLayout>
#include <QGraphicsLinearLayout>
#include <QGraphicsObject>

#include <clypsalot/forward.hxx>

#include "forward.hxx"
#include "workarea.hxx"

class ObjectPort : public WorkAreaWidget
{
    Q_OBJECT

    WorkAreaLabelWidget* nameLabel = nullptr;
    QList<PortConnection*> connections;

    public:
    Object* const parentObject;

    ObjectPort(Object* const parentObject, const QString& name, QGraphicsItem* parent = nullptr);
    const QString name();
    virtual QPointF connectPos() = 0;
    void addConnection(PortConnection* connection);
    void updateConnectionPositions();
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

class PortConnection : QGraphicsWidget
{
    Q_OBJECT

    ObjectOutput* const from;
    ObjectInput* const to;
    WorkAreaConnectionLine line;

    public:
    PortConnection(ObjectOutput* const from, ObjectInput* const to, QGraphicsObject* parent = nullptr);
    void updatePosition();
};

class ObjectInfo : public WorkAreaWidget
{
    Q_OBJECT

    public:
    WorkAreaLabelWidget* const kind;
    WorkAreaLabelWidget* const state;

    explicit ObjectInfo(QGraphicsItem* parent = nullptr);
};

class Object : public WorkAreaWidget
{
    Q_OBJECT

    ObjectInfo* info;
    std::map<QString, WorkAreaLabelWidget*> m_propertyValues;
    std::vector<std::shared_ptr<Clypsalot::Subscription>> subscriptions;

    protected:
    void handleEvent(const Clypsalot::ObjectStateChangedEvent& event);
    void initObject(QGraphicsLinearLayout* inputsLayout, QGraphicsLinearLayout* outputsLayout, QGraphicsGridLayout* propertiesLayout);
    QVariant itemChange(const GraphicsItemChange change, const QVariant& value) override;
    void updatePortConnectionPositions();

    Q_SIGNALS:
    void stateChanged(Clypsalot::ObjectState state);
    void propertyValues(const QList<std::pair<QString, QString>>& values);

    protected Q_SLOTS:
    void updateState(const Clypsalot::ObjectState state);
    void updateProperties(const QList<std::pair<QString, QString>>& values);

    public:
    const Clypsalot::SharedObject object;

    Object(const Clypsalot::SharedObject& object, QGraphicsItem* parent = nullptr);

    public Q_SLOTS:
    void start();
    void pause();
    void stop();
};
