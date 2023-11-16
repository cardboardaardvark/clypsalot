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

#include <QDrag>

#include <clypsalot/catalog.hxx>
#include <clypsalot/module.hxx>

#include "catalog.hxx"
#include "logger.hxx"

using namespace std::placeholders;

CatalogTopLevelItem::CatalogTopLevelItem(QTreeWidget* parent, const QString& title) :
    QTreeWidgetItem(parent, QStringList(title), catalogTopLevelItemType)
{ }

CatalogEntryItem::CatalogEntryItem(QTreeWidgetItem* parent, const QString& title, const int type) :
    QTreeWidgetItem(parent, QStringList(title), type)
{ }

CatalogObjectItem::CatalogObjectItem(QTreeWidgetItem* parent, const Clypsalot::ObjectDescriptor& descriptor) :
    CatalogEntryItem(parent, QString::fromStdString(descriptor.kind), catalogObjectItemType),
    m_descriptor(descriptor)
{ }

CatalogMimeData::CatalogMimeData(const CatalogEntryItem* const entry, const QString& title) :
    m_entry(entry)
{
    setText(title);
    setData(catalogEntryMimeFormat, "");
}

Catalog::Catalog(QWidget* parent) :
    QTreeWidget(parent)
{
    m_catalogObjects = makeTopLevelItem("Objects");
    m_catalogPlugins = makeTopLevelItem("Plugins");

    m_subscriptions.push_back(Clypsalot::objectCatalog().subscribe<Clypsalot::ObjectCatalogEntryAddedEvent>(std::bind(&Catalog::handleEvent, this, _1)));
    connect(&m_objectDescriptorQueue, SIGNAL(ready()), this, SLOT(addObjects()));
}

void Catalog::handleEvent(const Clypsalot::ObjectCatalogEntryAddedEvent& event)
{
    m_objectDescriptorQueue.push(&event.entry);
}

void Catalog::startDrag(Qt::DropActions)
{
    auto item = currentItem();

    if (item->type() == catalogTopLevelItemType) return;

    auto mime = new CatalogMimeData(dynamic_cast<const CatalogEntryItem*>(item), item->text(0));
    auto drag = new QDrag(this);
    drag->setMimeData(mime);
    drag->exec(Qt::CopyAction);
}

QTreeWidgetItem* Catalog::makeTopLevelItem(const QString& title)
{
    auto item = new CatalogTopLevelItem(this, title);
    item->setHidden(true);
    item->setExpanded(true);
    return item;
}

void Catalog::addObjects()
{
    if (m_catalogObjects->isHidden()) m_catalogObjects->setHidden(false);

    for (auto descriptor : m_objectDescriptorQueue.drain())
    {
        new CatalogObjectItem(m_catalogObjects, *descriptor);
    }
}
