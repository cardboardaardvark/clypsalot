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
#include <QGraphicsSceneDragDropEvent>

#include <clypsalot/object.hxx>

#include "createobjectdialog.hxx"
#include "logger.hxx"
#include "mainwindow.hxx"
#include "object.hxx"
#include "workarea.hxx"

WorkAreaWidget::WorkAreaWidget(QGraphicsItem* parent) :
    QGraphicsWidget(parent)
{
    if (scene() == nullptr)
    {
        MainWindow::instance()->workArea()->scene()->addItem(this);
    }

    m_borderColor = palette().color(QPalette::Active, QPalette::WindowText);
}

void WorkAreaWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    paintBorder(painter);
}

void WorkAreaWidget::paintBorder(QPainter* painter)
{
    assert(m_borderWidth >= 0);

    if (m_borderWidth == 0) return;

    auto pen = painter->pen();
    pen.setStyle(m_borderStyle);
    pen.setWidth(m_borderWidth);
    pen.setColor(m_borderColor);
    painter->setPen(pen);

    auto rect = boundingRect();
    rect.setX(m_borderWidth * .5);
    rect.setY(m_borderWidth * .5);
    rect.setWidth(rect.width() - m_borderWidth * .5);
    rect.setHeight(rect.height() - m_borderWidth * .5);
    painter->drawRect(rect);
}

qreal WorkAreaWidget::borderWidth()
{
    return m_borderWidth;
}

void WorkAreaWidget::setBorderWidth(const qreal borderWidth)
{
    m_borderWidth = borderWidth;
    update();
}

QColor WorkAreaWidget::borderColor()
{
    return m_borderColor;
}

void WorkAreaWidget::setBorderColor(const QColor& in_color)
{
    m_borderColor = in_color;

    update();
}

void WorkAreaWidget::setBorderStyle(const Qt::PenStyle in_style)
{
    m_borderStyle = in_style;
    update();
}

QVariant WorkAreaWidget::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == QGraphicsItem::ItemSelectedHasChanged) Q_EMIT selectedChanged(value.toBool());

    return value;
}

WorkAreaLabelWidget::WorkAreaLabelWidget(const QString& text, QGraphicsItem* parent) :
    WorkAreaWidget(parent),
    m_textItem(this)
{
    m_textItem.setBrush(palette().color(QPalette::Active, QPalette::WindowText));

    if (text != "") setText(text);
}

QString WorkAreaLabelWidget::text()
{
    return m_textItem.text();
}

void WorkAreaLabelWidget::setText(const QString& text)
{
    m_textItem.setText(text);

    auto textSize = m_textItem.boundingRect().size();
    setMinimumSize(textSize);
    setMaximumSize(textSize);
}

WorkAreaLineWidget::WorkAreaLineWidget(const QLineF& in_line, QGraphicsItem* in_parent) :
    WorkAreaWidget(in_parent)
{
    m_line = new QGraphicsLineItem(in_line, this);

    auto color = palette().color(QPalette::Active, QPalette::WindowText);
    auto pen = m_line->pen();
    pen.setWidth(4);
    pen.setColor(color);
    m_line->setPen(pen);

    setZValue(1);
    setInvalid(false);
}

QRectF WorkAreaLineWidget::boundingRect() const
{
    LOGGER(trace, "line boundingRect() called");
    return m_line->boundingRect();
}

QPainterPath WorkAreaLineWidget::shape() const
{
    LOGGER(trace, "line shape() called");
    return m_line->shape();
}

void WorkAreaLineWidget::setLine(const QLineF& in_line)
{
    m_line->setLine(in_line);
}

void WorkAreaLineWidget::setStyle(const Qt::PenStyle in_style)
{
    auto pen = m_line->pen();
    pen.setStyle(in_style);
    m_line->setPen(pen);
}

void WorkAreaLineWidget::setInvalid(const bool in_invalid)
{
    auto pen = m_line->pen();

    if (in_invalid) pen.setDashPattern({4, 4});
    else pen.setStyle(Qt::SolidLine);

    m_line->setPen(pen);
}

void WorkAreaLineWidget::setColor(const QColor& in_color)
{
    auto pen = m_line->pen();
    pen.setColor(in_color);
    m_line->setPen(pen);
}

WorkAreaScene::WorkAreaScene(QObject* parent) :
    QGraphicsScene(parent)
{
    const auto backgroundColor = Qt::black;
    auto newPalette = palette();

    setBackgroundBrush(backgroundColor);
    newPalette.setColor(QPalette::Active, QPalette::Window, backgroundColor);

    setPalette(newPalette);
}

void WorkAreaScene::dragEnterEvent(QGraphicsSceneDragDropEvent* event)
{
    if (event->mimeData()->hasFormat(catalogEntryMimeFormat))
    {
        event->setAccepted(true);
    }
}

void WorkAreaScene::dragMoveEvent(QGraphicsSceneDragDropEvent* event)
{
    if (event->mimeData()->hasFormat(catalogEntryMimeFormat))
    {
        event->setAccepted(true);
    }
}

void WorkAreaScene::dropEvent(QGraphicsSceneDragDropEvent* event)
{

    if (event->mimeData()->hasFormat(catalogEntryMimeFormat))
    {
        catalogDropEvent(event);
    }
}

void WorkAreaScene::catalogDropEvent(QGraphicsSceneDragDropEvent* event)
{
    auto mime = qobject_cast<const CatalogMimeData*>(event->mimeData());

    if (mime->m_entry->type() == catalogObjectItemType)
    {
        event->setAccepted(true);
        catalogObjectDrop(dynamic_cast<const CatalogObjectItem*>(mime->m_entry), event->scenePos());
    }
}

void WorkAreaScene::catalogObjectDrop(const CatalogObjectItem* const item, const QPointF& position)
{
    CreateObjectDialog dialog(MainWindow::instance(), item->m_descriptor);

    if (! dialog.exec()) return;

    auto object = dialog.object();
    auto outputs = dialog.outputs();
    auto inputs = dialog.inputs();
    auto config = dialog.config();

    {
        std::scoped_lock lock(*object);
        initObject(object, outputs, inputs, config);
    }

    auto uiObject = new Object(object);
    uiObject->setPos(position);
}

QList<Object*> WorkAreaScene::objects()
{
    QList<Object*> retval;

    for(auto item : items())
    {
        auto object = dynamic_cast<Object*>(item);
        if (object == nullptr) continue;
        retval.append(object);
    }

    return retval;
}

QList<Object*> WorkAreaScene::selectedObjects()
{
    QList<Object*> retval;

    for (auto item : selectedItems())
    {
        if (static_cast<WorkAreaItemType>(item->type()) != WorkAreaItemType::object) continue;

        auto object = dynamic_cast<Object*>(item);
        assert(object != nullptr);
        retval.push_back(object);
    }

    return retval;
}

WorkArea::WorkArea(QWidget *parent) :
    QGraphicsView(parent),
    m_scene(new WorkAreaScene(this))
{
    setScene(m_scene);
}

void WorkArea::startConnectionDrag()
{
    assert(! m_connectionDragLine);

    m_connectionDragLine = std::make_unique<WorkAreaLineWidget>();
}

void WorkArea::updateConnectionDrag(const QPointF& in_start, const QPointF& in_end, const QColor& in_color, const bool in_invalid)
{
    assert(m_connectionDragLine);

    m_connectionDragLine->setLine({in_start, in_end});
    m_connectionDragLine->setInvalid(in_invalid);
    m_connectionDragLine->setColor(in_color);
}

void WorkArea::resetConnectionDrag()
{
    assert(m_connectionDragLine);

    m_connectionDragLine = nullptr;
}

void WorkArea::selectAll()
{
    for (auto item : m_scene->items())
    {
        if (item->flags() & QGraphicsItem::ItemIsSelectable)
        {
            LOGGER(debug, "Selected item");
            item->setSelected(true);
        }
    }
}

void WorkArea::startObjects()
{
    for (auto object : m_scene->selectedObjects())
    {
        object->start();
    }
}

void WorkArea::pauseObjects()
{
    for (auto object : m_scene->selectedObjects())
    {
        object->pause();
    }
}

void WorkArea::stopObjects()
{
    for (auto object : m_scene->selectedObjects())
    {
        object->stop();
    }
}

void WorkArea::removeSelected()
{
    for (auto item : m_scene->selectedItems())
    {
        if (static_cast<WorkAreaItemType>(item->type()) == WorkAreaItemType::connection)
        {
            LOGGER(debug, "Deleting connection");
            delete item;
        }
    }

    for (auto item : m_scene->selectedItems())
    {
        if (static_cast<WorkAreaItemType>(item->type()) == WorkAreaItemType::object)
        {
            LOGGER(debug, "Deleting object");
            delete item;
        }
    }
}
