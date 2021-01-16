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

#ifndef ARCHIVETREE_H
#define ARCHIVETREE_H

#include <QTreeWidget>

#include "ifiletree.h"

class ArchiveTreeWidget;

// custom tree widget that holds a shared pointer to the file tree entry
// they represent
//
class ArchiveTreeWidgetItem : public QTreeWidgetItem {
public:

  ArchiveTreeWidgetItem(QString dataName);
  ArchiveTreeWidgetItem(std::shared_ptr<MOBase::FileTreeEntry> entry);
  ArchiveTreeWidgetItem(ArchiveTreeWidgetItem* parent, std::shared_ptr<MOBase::FileTreeEntry> entry);
  ArchiveTreeWidgetItem(ArchiveTreeWidget* parent, std::shared_ptr<MOBase::FileTreeEntry> entry);

public:

  // populate this tree widget item if it has not been populated yet
  // or if force is true
  //
  void populate(bool force = false);

  // check if this item has already been populated
  //
  bool isPopulated() const { return m_Populated; }

  // replace the entry corresponding to this item
  //
  void setEntry(std::shared_ptr<MOBase::FileTreeEntry> entry) {
    m_Entry = entry;
  }

  // retrieve the entry corresponding to this item
  //
  std::shared_ptr<MOBase::FileTreeEntry> entry() const {
    return m_Entry;
  }

  // overriden method to avoid propagating dataChanged events
  //
  void setData(int column, int role, const QVariant& value) override;

  ArchiveTreeWidgetItem* parent() const {
    return static_cast<ArchiveTreeWidgetItem*>(QTreeWidgetItem::parent());
  }

  ArchiveTreeWidgetItem* child(int index) const {
    return static_cast<ArchiveTreeWidgetItem*>(QTreeWidgetItem::child(index));
  }

protected:

  std::shared_ptr<MOBase::FileTreeEntry> m_Entry;
  bool m_Populated = false;

  friend class ArchiveTreeWidget;
};


// Qt tree widget used to display the content of an archive in the manual installation
// dialog
class ArchiveTreeWidget : public QTreeWidget
{

  Q_OBJECT

public:

  explicit ArchiveTreeWidget(QWidget* parent = 0);

signals:

  // emitted after a tree widget item is moved from one parent to another
  //
  void itemMoved(ArchiveTreeWidgetItem* source, ArchiveTreeWidgetItem* target);

  // emitted when a tree widget item is checked or unchecked
  //
  // unlike itemChanged() or dataChanged(), this signal is only emitted
  // for the item that was changed, not its parent or child
  //
  // this signal is emitted after the tree has been updated, and after all the
  // itemChanged() or dataChanged() signals
  //
  void treeCheckStateChanged(ArchiveTreeWidgetItem* item);

public slots:

protected:

  // slot that trigger the given item to be populated if it has not already
  // been
  //
  void populateItem(QTreeWidgetItem* item);

  virtual void dragEnterEvent(QDragEnterEvent *event) override;
  virtual void dragMoveEvent(QDragMoveEvent *event) override;
  virtual void dropEvent(QDropEvent *event) override;

private:

  bool testMovePossible(ArchiveTreeWidgetItem* source, ArchiveTreeWidgetItem* target);

  // refresh the given item (after a drop)
  //
  void refreshItem(ArchiveTreeWidgetItem* item);

  // emit the itemCheckStateChanged event
  //
  void emitTreeCheckStateChanged(ArchiveTreeWidgetItem* item);

  // the widget item that emitted the dataChanged event
  ArchiveTreeWidgetItem* m_Emitter = nullptr;

  friend class ArchiveTreeWidgetItem;

};

#endif // ARCHIVETREE_H
