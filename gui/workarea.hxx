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
    object,
    connection,
};

class WorkAreaWidget : public QGraphicsWidget
{
    Q_OBJECT

    qreal m_borderWidth = 0;
    QColor m_borderColor;
    Qt::PenStyle m_borderStyle = Qt::SolidLine;

    protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;
    void paintBorder(QPainter* painter);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

    Q_SIGNALS:
    void selectedChanged(bool);

    public:
    explicit WorkAreaWidget(QGraphicsItem* parent = nullptr);
    qreal borderWidth();
    void setBorderWidth(const qreal borderWidth);
    QColor borderColor();
    void setBorderColor(const QColor& in_color);
    void setBorderStyle(const Qt::PenStyle in_style);
};

class WorkAreaLabelWidget : public WorkAreaWidget
{
    Q_OBJECT

    QGraphicsSimpleTextItem m_textItem;

    public:
    WorkAreaLabelWidget(const QString& text = "", QGraphicsItem* parent = nullptr);
    QString text();
    void setText(const QString& text);
};

class WorkAreaLineWidget : public WorkAreaWidget
{
    QGraphicsLineItem* m_line = nullptr;

    public:
    WorkAreaLineWidget(const QLineF& in_line = QLineF(), QGraphicsItem* in_parent = nullptr);
    QPainterPath shape() const override;
    QRectF boundingRect() const override;
    void setLine(const QLineF& in_line);
    void setStyle(const Qt::PenStyle in_style);
    void setInvalid(const bool in_invalid);
    void setColor(const QColor& in_color);
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
    QList<Object*> selectedObjects();
    QList<Object*> objects();
};

class WorkArea : public QGraphicsView
{
    Q_OBJECT

    std::unique_ptr<WorkAreaLineWidget> m_connectionDragLine;
    bool m_updateObjectsScheduled = false;
    QTimer* objectUpdateTimer = nullptr;

    public:
    WorkAreaScene* const m_scene;

    explicit WorkArea(QWidget* parent = nullptr);
    void startConnectionDrag();
    void updateConnectionDrag(const QPointF& in_start, const QPointF& in_end, const QColor& in_color, const bool in_invalid);
    void resetConnectionDrag();

    public Q_SLOTS:
    void scheduleUpdateObjects();
    void updateObjects();
    void selectAll();
    void startObjects();
    void pauseObjects();
    void stopObjects();
    void removeSelected();
};

QGraphicsItem* workAreaItemAt(const WorkAreaItemType in_type, const QPointF& in_scenePos) noexcept;
