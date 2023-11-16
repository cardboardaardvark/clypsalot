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

#include <QMimeData>
#include <QString>
#include <QTreeWidgetItem>

#include <clypsalot/forward.hxx>

#include "util.hxx"

static const int catalogTopLevelItemType = QTreeWidgetItem::UserType;
static const int catalogObjectItemType = catalogTopLevelItemType + 1;
static const QString catalogEntryMimeFormat("application/x-clypsalot-catalog-entry");

struct CatalogTopLevelItem : public QTreeWidgetItem
{
    CatalogTopLevelItem(QTreeWidget* parent, const QString& title);
};

struct CatalogEntryItem : public QTreeWidgetItem
{
    CatalogEntryItem(QTreeWidgetItem* parent, const QString& title, const int type);
};

struct CatalogObjectItem : public CatalogEntryItem
{
    const Clypsalot::ObjectDescriptor& m_descriptor;

    CatalogObjectItem(QTreeWidgetItem* parent, const Clypsalot::ObjectDescriptor& descriptor);
};

class CatalogMimeData : public QMimeData
{
    Q_OBJECT

    public:
    const CatalogEntryItem* const m_entry;

    CatalogMimeData(const CatalogEntryItem* const entry, const QString& title);
};

class Catalog : public QTreeWidget
{
    Q_OBJECT

    std::vector<std::shared_ptr<Clypsalot::Subscription>> m_subscriptions;
    ThreadSafeQueue<const Clypsalot::ObjectDescriptor*> m_objectDescriptorQueue;

    protected:
    QTreeWidgetItem* m_catalogObjects = nullptr;
    QTreeWidgetItem* m_catalogPlugins = nullptr;

    QTreeWidgetItem* makeTopLevelItem(const QString& title);
    void handleEvent(const Clypsalot::ObjectCatalogEntryAddedEvent& event);
    void startDrag(Qt::DropActions);

    protected Q_SLOTS:
    void addObjects();

    public:
    Catalog(QWidget* parent);
};
