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

#ifndef INSTALLDIALOG_H
#define INSTALLDIALOG_H

#include "archivetree.h"
#include "tutorabledialog.h"
#include <guessedvalue.h>
#include <ifiletree.h>

#include <iplugingame.h>
#include <moddatachecker.h>

#include <QDialog>
#include <QUuid>
#include <QTreeWidgetItem>
#include <QProgressDialog>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


namespace Ui {
    class InstallDialog;
}


/**
 * a dialog presented to manually define how a mod is to be installed. It provides
 * a tree view of the file contents that can modified directly
 **/
class InstallDialog : public MOBase::TutorableDialog
{
    Q_OBJECT

public:
  /**
   * @brief Create a new install dialog for the given tree. The tree
   * is "own" by the dialog, i.e., any change made by the user is immediately
   * reflected to the given tree, except for the changes to the root.
   *
   * @param tree Tree structure describing the original archive structure.
   * @param modName Name of the mod. The name can be modified through the dialog.
   * @param gamePlugin The current game plugin.
   * @param parent Parent widget.
   **/
  explicit InstallDialog(std::shared_ptr<MOBase::IFileTree> tree, const MOBase::GuessedValue<QString> &modName, const MOBase::IPluginGame* gamePlugin, QWidget *parent = 0);
  ~InstallDialog();

  /**
   * @brief retrieve the (modified) mod name
   *
   * @return updated mod name
   **/
  QString getModName() const;

  /**
   * @brief Retrieve the user-modified directory structure.
   *
   * @return the new tree represented by this dialog, which can be a new
   *     tree or a subtree of the original tree.
   **/
  std::shared_ptr<MOBase::IFileTree> getModifiedTree() const;

signals:

  /**
   * @brief Signal emitted when user request the file corresponding
   *     to the given entry to be opened.
   *
   * @param entry Entry corresponding to the file to open.
   */
  void openFile(const MOBase::FileTreeEntry *entry);

private:

  bool testForProblem();
  void updateProblems();

  /**
   * @brief Detach the entry of this item from its parent, and recursively detach
   *     all of its parent if they become empty.
   *
   * @param item The item to detach.
   */
  void detachParents(ArchiveTreeWidgetItem* item);

  /**
   * @brief Re-attach the entry of this item to its parent, and recursively attach
   *    all of its parent if they were empty (and thus detached).
   *
   * @param item The item to attach.
   */
  void attachParents(ArchiveTreeWidgetItem* item);

  /**
   * @brief Recursively re-insert all the entries below the given item in their
   *     corresponding parents. This method does not recurse in items that have not 
   *     been populated yet.
   *
   * @param item The top-level item to start.
   */
  void recursiveInsert(ArchiveTreeWidgetItem* item);

  /**
   * @brief Recursively detach all the entries below the given item from their
   *     corresponding parents. This method does not recurse in items that have not 
   *     been populated yet.
   *
   * @param item The top-level item to start.
   */
  void recursiveDetach(ArchiveTreeWidgetItem* item);

  /**
   * @brief Set the data root widget.
   */
  void setDataRoot(ArchiveTreeWidgetItem* const root);

  /**
   * @brief Create a directory under the given tree item, asking
   *     the user for a name.
   *
   * @param treeItem Parent item of the directory.
   */
  void createDirectoryUnder(ArchiveTreeWidgetItem *treeItem);

private slots:

  // The two slots to connect to the tree:
  void onItemMoved(ArchiveTreeWidgetItem* source, ArchiveTreeWidgetItem* target);
  void onTreeCheckStateChanged(ArchiveTreeWidgetItem* item);

  // Automatic slots that are directly bound to the UI:
  void on_treeContent_customContextMenuRequested(QPoint pos);
  void on_cancelButton_clicked();
  void on_okButton_clicked();

private:
  Ui::InstallDialog *ui;

  const ModDataChecker* m_Checker;

  // Name of the "data" directory:
  QString m_DataFolderName;

  ArchiveTreeWidget *m_Tree;
  QLabel *m_ProblemLabel;

  // IMPORTANT: If you intend to work on this and understand this, read the detailed
  // explanation at the beginning of the installdialog.cpp file.

  // The tree root is the initial root that will never change (should be const
  // but cannot be since the parent tree cannot be consstructed in the member
  // initializer list):
  ArchiveTreeWidgetItem *m_TreeRoot;

  // The data root is the real widget of the current data. This widget
  // is not the real root that is added to the tree.
  ArchiveTreeWidgetItem *m_DataRoot;

  // This is the actual tree in the widget  (should be const but cannot be since 
  // the parent tree cannot be consstructed in the member initializer list):
  ArchiveTreeWidgetItem *m_ViewRoot;

};

#endif // INSTALLDIALOG_H
