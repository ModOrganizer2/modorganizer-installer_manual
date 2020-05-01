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

#include <ifiletree.h>
#include <log.h>

/**
 * This is the constructor for the top-level item which is a fake item:
 */
ArchiveTreeWidgetItem::ArchiveTreeWidgetItem(std::nullptr_t)
  : QTreeWidgetItem(QStringList("<data>")), m_Entry(nullptr) { 
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

/**
 *
 */
void ArchiveTreeWidgetItem::setData(int column, int role, const QVariant& value) 
{
  ArchiveTreeWidget* tree = static_cast<ArchiveTreeWidget*>(treeWidget());
  if (tree != nullptr && tree->m_Emitter == nullptr) {
    tree->m_Emitter = this;
  }
  QTreeWidgetItem::setData(column, role, value);
  if (tree != nullptr && tree->m_Emitter == this) {
    tree->m_Emitter = nullptr;
    tree->emitTreeCheckStateChanged(this);
  }
}

/**
 *
 */
void ArchiveTreeWidgetItem::populate() {

  // Only populates once:
  if (isPopulated()) {
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

/**
 *
 */
ArchiveTreeWidget::ArchiveTreeWidget(QWidget *parent) : QTreeWidget(parent) { 
  connect(this, &ArchiveTreeWidget::itemExpanded, this, &ArchiveTreeWidget::populateItem);
}

/**
 *
 */
void ArchiveTreeWidget::populateItem(QTreeWidgetItem* item) {
  static_cast<ArchiveTreeWidgetItem*>(item)->populate();
}

/**
 *
 */
void ArchiveTreeWidget::emitTreeCheckStateChanged(ArchiveTreeWidgetItem* item) 
{
  emit treeCheckStateChanged(item);
}

/**
 *
 */
bool ArchiveTreeWidget::testMovePossible(QTreeWidgetItem *source, QTreeWidgetItem *target)
{
  if ((target == nullptr) ||
      (source == nullptr)) {
    return false;
  }

  if ((source == target) ||
      (source->parent() == target)) {
    return false;
  }

  return true;
}

/**
 *
 */
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

/**
 *
 */
void ArchiveTreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
  if (!testMovePossible(this->currentItem(), itemAt(event->pos()))) {
    event->ignore();
  } else {
    QTreeWidget::dragMoveEvent(event);
  }
}

/**
 *
 */
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

/**
 *
 */
void ArchiveTreeWidget::dropEvent(QDropEvent *event)
{
  event->ignore();

  // Target widget (might not be a directory):
  QTreeWidgetItem *target = itemAt(event->pos());

  // Index at which the item should be insert (-1 = at the end):
  int insertIndex = -1;

  // If the ItemNeverHasChildren flag is enabled, the target is a file:
  if (target->flags().testFlag(Qt::ItemNeverHasChildren)) {

    if (target->parent() == nullptr) {
      // This should really not happen, how should a file get to the top level?
      return;
    }

    insertIndex = target->parent()->indexOfChild(target);
    target = target->parent();
  }

  // Populate target if required:
  static_cast<ArchiveTreeWidgetItem*>(target)->populate();

  QList<QTreeWidgetItem*> sourceItems = this->selectedItems();

  // Process all the selected items:
  for (QTreeWidgetItem *source : sourceItems) {

    // Do not allow element to be dropped into one of its
    // own child:
    if (isAncestor(source, target)) {
      event->accept();
      return;
    }

    if ((source->parent() != nullptr) &&
        testMovePossible(source, target)) {
      source->parent()->removeChild(source);

      if (insertIndex == -1) {
        target->addChild(source);
      }
      else {
        MOBase::log::debug("Insert child {} at index {}.", source->text(0), insertIndex);
        target->insertChild(insertIndex, source);
        insertIndex += 1;
      }

      emit itemMoved(
        static_cast<ArchiveTreeWidgetItem*>(source),
        static_cast<ArchiveTreeWidgetItem*>(target));
    }
  }

}
