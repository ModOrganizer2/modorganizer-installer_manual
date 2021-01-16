/*
Copyright (C) 2012 Sebastian Herbord. All rights reserved.

This file is part of Mod Organizer.

Mod Organizer is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Mod Organizer is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Mod Organizer.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "archivetree.h"

#include <QDragMoveEvent>
#include <QDebug>
#include <QMessageBox>

#include <ifiletree.h>
#include <log.h>

/**
 * This is the constructor for the top-level item which is a fake item:
 */
ArchiveTreeWidgetItem::ArchiveTreeWidgetItem(QString dataName)
  : QTreeWidgetItem(QStringList(dataName)), m_Entry(nullptr) {
  setFlags(flags() & ~Qt::ItemIsUserCheckable);
  setExpanded(true);
  m_Populated = true;
}

ArchiveTreeWidgetItem::ArchiveTreeWidgetItem(std::shared_ptr<MOBase::FileTreeEntry> entry)
  : QTreeWidgetItem(QStringList(entry->name())), m_Entry(entry)
{
  if (entry->isDir()) {
    setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    setFlags(flags() | Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate);
  }
  else {
    setFlags(flags() | Qt::ItemIsUserCheckable | Qt::ItemNeverHasChildren);
  }
  setCheckState(0, Qt::Checked);
  setToolTip(0, entry->path());
}

ArchiveTreeWidgetItem::ArchiveTreeWidgetItem(ArchiveTreeWidgetItem* parent, std::shared_ptr<MOBase::FileTreeEntry> entry)
  : ArchiveTreeWidgetItem(entry)
{
  parent->addChild(this);
}

ArchiveTreeWidgetItem::ArchiveTreeWidgetItem(ArchiveTreeWidget* parent, std::shared_ptr<MOBase::FileTreeEntry> entry)
  : ArchiveTreeWidgetItem(entry)
{
  parent->addTopLevelItem(this);
}

void ArchiveTreeWidgetItem::setData(int column, int role, const QVariant& value)
{
  ArchiveTreeWidget* tree = static_cast<ArchiveTreeWidget*>(treeWidget());
  if (tree != nullptr && tree->m_Emitter == nullptr) {
    tree->m_Emitter = this;
  }
  QTreeWidgetItem::setData(column, role, value);
  if (tree != nullptr && tree->m_Emitter == this) {
    tree->m_Emitter = nullptr;
    if (role == Qt::CheckStateRole) {
      tree->emitTreeCheckStateChanged(this);
    }
  }
}

void ArchiveTreeWidgetItem::populate(bool force) {

  // Only populates once:
  if (isPopulated() && !force) {
    return;
  }

  // Should never happen:
  if (entry()->isFile()) {
    return;
  }

  // We go in reverse of the tree because we want to insert the original
  // entries at the beginning (the item can only contains children if a
  // directory has been created under it or if entries has been moved under
  // it):
  for (auto &entry: *entry()->astree()) {
    auto newItem = new ArchiveTreeWidgetItem(entry);
    newItem->setCheckState(0, flags().testFlag(Qt::ItemIsUserCheckable) ? checkState(0) : Qt::Checked);
    QTreeWidgetItem::addChild(newItem);
  }

  // If the item is unchecked, we need to clear it because it has not been cleared
  // before:
  if (checkState(0) == Qt::Unchecked) {
    entry()->astree()->clear();
  }

  m_Populated = true;
}

ArchiveTreeWidget::ArchiveTreeWidget(QWidget *parent) : QTreeWidget(parent)
{
  setAutoExpandDelay(1000);
  setDragDropOverwriteMode(true);
  connect(this, &ArchiveTreeWidget::itemExpanded, this, &ArchiveTreeWidget::populateItem);
}

void ArchiveTreeWidget::populateItem(QTreeWidgetItem* item)
{
  static_cast<ArchiveTreeWidgetItem*>(item)->populate();
}

void ArchiveTreeWidget::emitTreeCheckStateChanged(ArchiveTreeWidgetItem* item)
{
  emit treeCheckStateChanged(item);
}

bool ArchiveTreeWidget::testMovePossible(ArchiveTreeWidgetItem* source, ArchiveTreeWidgetItem* target)
{
  if (target == nullptr || source == nullptr) {
    return false;
  }

  if (target->flags().testFlag(Qt::ItemNeverHasChildren)) {
    return false;
  }

  if (source == target || source->parent() == target) {
    return false;
  }

  return true;
}

void ArchiveTreeWidget::dragEnterEvent(QDragEnterEvent *event)
{
  QTreeWidgetItem *source = this->currentItem();
  if ((source == nullptr) || (source->parent() == nullptr)) {
    // can't change top level
    event->ignore();
    return;
  } else {
    QTreeWidget::dragEnterEvent(event);
  }

}

void ArchiveTreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
  if (!testMovePossible(
    static_cast<ArchiveTreeWidgetItem*>(currentItem()),
    static_cast<ArchiveTreeWidgetItem*>(itemAt(event->pos())))) {
    event->ignore();
  } else {
    QTreeWidget::dragMoveEvent(event);
  }
}

static bool isAncestor(const QTreeWidgetItem *ancestor, const QTreeWidgetItem *item)
{
  QTreeWidgetItem *iter = item->parent();
  while (iter != nullptr) {
    if (iter == ancestor) {
      return true;
    }
    iter = iter->parent();
  }
  return false;
}

void ArchiveTreeWidget::refreshItem(ArchiveTreeWidgetItem* item)
{
  if (!item->isPopulated() || item->flags().testFlag(Qt::ItemNeverHasChildren)) {
    return;
  }

  // at this point, all child items are checked for we only remember the ones
  // that were expanded to re-expand them
  std::map<QString, bool, MOBase::FileNameComparator> expanded;
  while (item->childCount() > 0) {
    auto* child = item->child(0);
    expanded[child->entry()->name()] = child->isExpanded();
    item->removeChild(child);
  }

  item->populate(true);

  for (int i = 0; i < item->childCount(); ++i) {
    auto* child = item->child(i);
    if (expanded[child->entry()->name()]) {
      child->setExpanded(true);
    }
  }
}

void ArchiveTreeWidget::dropEvent(QDropEvent *event)
{
  event->ignore();

  // target widget (should be a directory)
  auto *target =  static_cast<ArchiveTreeWidgetItem*>(itemAt(event->pos()));

  // this should not really happen because it is prevent by dragMoveEvent
  if (target->flags().testFlag(Qt::ItemNeverHasChildren)) {

    // this should really not happen, how should a file get to the top level?
    if (target->parent() == nullptr) {
      return;
    }

    target = target->parent();
  }

  // populate target if required
  target->populate();

  auto sourceItems = this->selectedItems();

  // check the selected items - we do not want to move only
  // some items so we check everything first and then move
  for (auto* source : sourceItems) {

    auto* aSource = static_cast<ArchiveTreeWidgetItem*>(source);

    // do not allow element to be dropped into one of its
    // own child
    if (isAncestor(source, target)) {
      event->accept();
      QMessageBox::warning(parentWidget(),
        tr("Cannot drop"),
        tr("Cannot drop '%1' into one of its subfolder.").arg(aSource->entry()->name()));
      return;
    }

    auto sourceEntry = aSource->entry();
    auto targetEntry = target->entry()->astree()->find(sourceEntry->name());
    if (targetEntry && targetEntry->fileType() != sourceEntry->fileType()) {
      event->accept();
      QMessageBox::warning(parentWidget(),
        tr("Cannot drop"),
        targetEntry->isFile() ?
        tr("A file '%1' already exists in folder '%2'.").arg(sourceEntry->name()).arg(target->entry()->name())
        : tr("A folder '%1' already exists in folder '%2'.").arg(sourceEntry->name()).arg(target->entry()->name()));
      return;
    }

  }

  for (auto* source : sourceItems) {

    auto* aSource = static_cast<ArchiveTreeWidgetItem*>(source);

    // this only check dropping an item on itself or dropping an item in
    // its parent so it is ok, it just does not do anything
    if (source->parent() == nullptr || !testMovePossible(aSource, target)) {
      continue;
    }

    // force expand item that are going to be merged
    for (int i = 0; i < target->childCount(); ++i) {
      auto* child = target->child(i);
      if (child->entry()->compare(aSource->entry()->name()) == 0
        && !child->flags().testFlag(Qt::ItemNeverHasChildren)) {
        child->setExpanded(true);
      }
    }

    // remove the source from its parent
    source->parent()->removeChild(source);

    // actually perform the move on the underlying tree model
    emit itemMoved(aSource, target);
  }

  // refresh the target item - this assumes that itemMoved is called synchronously
  // and perform the FileTree changes
  refreshItem(target);

}
