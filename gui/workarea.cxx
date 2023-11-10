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

#include <QGraphicsSceneDragDropEvent>

#include "createobjectdialog.hxx"
#include "logger.hxx"
#include "mainwindow.hxx"
#include "object.hxx"
#include "workarea.hxx"

WorkAreaWidget::WorkAreaWidget(QGraphicsItem* parent) :
    QGraphicsWidget(parent)
{ }

void WorkAreaWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    paintBorder(painter);
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

void WorkAreaWidget::paintBorder(QPainter* painter)
{
    assert(m_borderWidth >= 0);

    if (m_borderWidth == 0) return;

    auto pen = painter->pen();
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(m_borderWidth);
    painter->setPen(pen);

    auto rect = boundingRect();
    rect.setX(m_borderWidth);
    rect.setY(m_borderWidth);
    rect.setWidth(rect.width() - m_borderWidth);
    rect.setHeight(rect.height() - m_borderWidth);
    painter->drawRect(rect);
}

void WorkAreaWidget::updateSelected(const bool selected)
{
    LOGGER(debug, "Selected=", selected);
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

WorkAreaScene::WorkAreaScene(QObject* parent) :
    QGraphicsScene(parent)
{ }

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

    auto& descriptor = item->m_descriptor;
    auto outputs = dialog.outputs();
    auto inputs = dialog.inputs();
    auto config = dialog.config();
    auto object = new Object(makeObject(descriptor, outputs, inputs, config));

    object->setPos(position);
    addItem(object);
}

WorkAreaConnectionLine::WorkAreaConnectionLine(const QLineF& line, QGraphicsItem* parent) :
    QGraphicsLineItem(line, parent)
{
    auto ourPen = pen();
    ourPen.setWidth(2);
    setPen(ourPen);

    setZValue(1);
    setInvalid(false);
}

void WorkAreaConnectionLine::setInvalid(const bool invalid)
{
    auto newPen = pen();

    if (invalid) newPen.setDashPattern({4, 4});
    else newPen.setStyle(Qt::SolidLine);

    setPen(newPen);
}

WorkArea::WorkArea(QWidget *parent) :
    QGraphicsView(parent),
    m_scene(new WorkAreaScene(this))
{
    setScene(m_scene);
}

QList<Object*> WorkArea::objects()
{
    QList<Object*> retval;

    for(auto item : m_scene->items())
    {
        auto object = dynamic_cast<Object*>(item);
        if (object == nullptr) continue;
        retval.append(object);
    }

    return retval;
}

void WorkArea::startConnectionDrag()
{
    LOGGER(trace, "Starting connection drag");

    assert(m_connectionDragLine == nullptr);

    m_connectionDragLine = new WorkAreaConnectionLine();
    m_scene->addItem(m_connectionDragLine);
}

void WorkArea::updateConnectionDrag(const QPointF& start, const QPointF& end, const bool invalid)
{
    LOGGER(trace, "Updating connection drag");

    assert(m_connectionDragLine != nullptr);

    m_connectionDragLine->setLine({start, end});
    m_connectionDragLine->setInvalid(invalid);
}

void WorkArea::resetConnectionDrag()
{
    LOGGER(trace, "Reseting connection drag");

    assert(m_connectionDragLine != nullptr);

    m_scene->removeItem(m_connectionDragLine);
    delete m_connectionDragLine;
    m_connectionDragLine = nullptr;
}

void WorkArea::startObjects()
{
    for (auto object : objects())
    {
        object->start();
    }
}

void WorkArea::pauseObjects()
{
    for (auto object : objects())
    {
        object->pause();
    }
}

void WorkArea::stopObjects()
{
    for (auto object : objects())
    {
        object->stop();
    }
}
