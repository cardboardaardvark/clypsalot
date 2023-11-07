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

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsWidget>
#include <QLineF>

#include <clypsalot/forward.hxx>

#include "catalog.hxx"
#include "forward.hxx"

enum class WorkAreaItemType : int
{
    inputPort = QGraphicsItem::UserType,
};

class WorkAreaWidget : public QGraphicsWidget
{
    Q_OBJECT

    qreal m_borderWidth = 0;

    protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;
    void paintBorder(QPainter* painter);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

    protected Q_SLOTS:
    void updateSelected(const bool selected);

    Q_SIGNALS:
    void selectedChanged(bool);

    public:
    explicit WorkAreaWidget(QGraphicsItem* parent = nullptr);
    qreal borderWidth();
    void setBorderWidth(const qreal borderWidth);
};

class WorkAreaLabelWidget : public WorkAreaWidget
{
    Q_OBJECT

    QGraphicsSimpleTextItem textItem;

    public:
    WorkAreaLabelWidget(const QString& text = "", QGraphicsItem* parent = nullptr);
    QString text();
    void setText(const QString& text);
};

class WorkAreaScene : public QGraphicsScene
{
    Q_OBJECT

    protected:
    void dragEnterEvent(QGraphicsSceneDragDropEvent* event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent* event) override;
    void dropEvent(QGraphicsSceneDragDropEvent* event) override;
    void catalogDropEvent(QGraphicsSceneDragDropEvent* event);
    void catalogObjectDrop(const CatalogObjectItem* const item, const QPointF& position);

    public:
    WorkAreaScene(QObject* parent);
};

class WorkAreaConnectionLine : public QGraphicsLineItem
{
    public:
    WorkAreaConnectionLine(const QLineF& line = QLineF(), QGraphicsItem* parent = nullptr);
    void setInvalid(const bool invalid);
};

class WorkArea : public QGraphicsView
{
    Q_OBJECT

    WorkAreaConnectionLine* connectionDragLine = nullptr;

    public:
    WorkAreaScene* const scene;

    explicit WorkArea(QWidget* parent = nullptr);
    QList<Object*> objects();
    void startConnectionDrag();
    void updateConnectionDrag(const QPointF& start, const QPointF& end, const bool invalid);
    void resetConnectionDrag();

    public Q_SLOTS:
    void startObjects();
    void pauseObjects();
    void stopObjects();
};
