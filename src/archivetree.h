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

/**
 * @brief Custom tree widget that holds a shared pointer to the file tree entry
 * they represent.
 */
class ArchiveTreeWidgetItem : public QTreeWidgetItem {
public:

  ArchiveTreeWidgetItem(std::nullptr_t = nullptr);
  ArchiveTreeWidgetItem(std::shared_ptr<MOBase::FileTreeEntry> entry);
  ArchiveTreeWidgetItem(ArchiveTreeWidgetItem* parent, std::shared_ptr<MOBase::FileTreeEntry> entry);
  ArchiveTreeWidgetItem(ArchiveTreeWidget* parent, std::shared_ptr<MOBase::FileTreeEntry> entry);

public:

  /**
   * @brief Populate this tree widget item.
   */
  void populate();

  /**
   * @brief Check if this item has been populated already.
   *
   * @return true if this item has been populated, false otherwize.
   */
  bool isPopulated() const {
    return m_Populated;
  }

  /**
   * @brief Replace the entry corresponding to this item.
   *
   * @param entry The new entry.
   */
  void setEntry(std::shared_ptr<MOBase::FileTreeEntry> entry) {
    m_Entry = entry;
  }

  /**
   * @brief Retrieve the entry corresponding to this item.
   *
   * @return the entry corresponding to this item.
   */
  std::shared_ptr<MOBase::FileTreeEntry> entry() const {
    return m_Entry;
  }

  /**
   * @brief Overriden method to avoid propagating dataChanged events.
   *
   */
  void setData(int column, int role, const QVariant& value) override;

protected:

  // The entry of the item:
  std::shared_ptr<MOBase::FileTreeEntry> m_Entry;

  // Populated or not (when expanded):
  bool m_Populated = false;

  friend class ArchiveTreeWidget;
};


/**
 * @brief QT tree widget used to display the content of an archive in the manual installation dialog
 **/
class ArchiveTreeWidget : public QTreeWidget
{

  Q_OBJECT

public:

  /**
   * @brief Default constructor for the UI.
   *
   */
  explicit ArchiveTreeWidget(QWidget *parent = 0);

signals:

  /**
   * @brief Emitted after a tree widget item is moved from one
   * parent to another.
   *
   * @param source The item being moved.
   * @param target The item to which the source is added.
   *
   */
  void itemMoved(ArchiveTreeWidgetItem *source, ArchiveTreeWidgetItem *target);

  /**
   * @brief Emitted when a tree widget item is checked or unchecked.
   *
   * Unlike itemChanged() or dataChanged(), this signal is only emitted
   * for the item that was changed, not its parent or child.
   *
   * This signal is emitted after the tree has been updated, and after all the 
   * itemChanged() or dataChanged() signals.
   *
   * @param item Item that was triggered the change.
   */
  void treeCheckStateChanged(ArchiveTreeWidgetItem* item);

public slots:

protected:

  /**
   * @brief Slot that trigger the given item to be populated if it has not already
   *     been.
   *
   * @param item The item to populate.
   */
  virtual void populateItem(QTreeWidgetItem* item);

  virtual void dragEnterEvent(QDragEnterEvent *event) override;
  virtual void dragMoveEvent(QDragMoveEvent *event) override;
  virtual void dropEvent(QDropEvent *event) override;

private:

  bool testMovePossible(QTreeWidgetItem *source, QTreeWidgetItem *target);

  /**
   * @brief Emit the itemCheckStateChanged event.
   * 
   * @param item Item that was changed.
   */
  void emitTreeCheckStateChanged(ArchiveTreeWidgetItem* item);

  // The widget item that emitted the dataChanged event:
  ArchiveTreeWidgetItem* m_Emitter = nullptr;

  friend class ArchiveTreeWidgetItem;

};

#endif // ARCHIVETREE_H
