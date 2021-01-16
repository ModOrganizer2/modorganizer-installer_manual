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

#include "installdialog.h"
#include "ui_installdialog.h"

#include "report.h"
#include "utility.h"
#include "log.h"

#include <QMenu>
#include <QCompleter>
#include <QInputDialog>
#include <QMetaType>
#include <QMessageBox>

// Implementation details for the ArchiveTree widget:
//
// The ArchiveTreeWidget presents to the user the underlying IFileTree, but in order
// to increase performance, the tree is populated dynamically when required. Populating
// the tree is currently required:
//   1) when a branch of the tree widget is expanded,
//   2) when an item is moved to a tree,
//   3) when a directory is created,
//   4) when a directory is "set as data root".
//
// Case 1 is handled automatically in the setExpanded method of ArchiveTreeWidget. Cases 2
// and 3 could be dealt with differently, but populating the tree before inserting an item
// makes everything else easier (not that populating the widget is different from populating
// the IFileTree which is done automatically). Case 4 is handled manually in setDataRoot.
//
// Another specificity of the implementation is the treeCheckStateChanged() signal emitted
// by the ArchiveTreeWidget. This signal is used to avoid having to connect to the itemChanged()
// signal or overriding the dataChanged() method which are called much more often than those.
// The treeCheckStateChanged() signal is send only for the item that has actually been changed
// by the user. While the interface is automatically updated by Qt, we need to update the
// underlying tree manually. This is done by doing the following things:
//   1) When an item is unchecked:
//      - We detach the corresponding entry from its parent, and recursively detach the empty
//        parents (or the ones that become empty).
//      - If the entry is a directory and the item has been populated, we recursively detach
//        all the child entries for all the child items that have been populated (no need to
//        do it for non-populated items)>
//   2) When an item is checked, we do the same process but we re-attach parents and re-insert
//      children.
//
// Detaching or re-attaching parents is also done when a directory is created (if the directory
// is created in an empty directory, we need to re-attach), or when an item is moved (if the
// directory the item comes from is now empty or if the target directory was empty).

using namespace MOBase;


InstallDialog::InstallDialog(std::shared_ptr<IFileTree> tree, const GuessedValue<QString> &modName, const IPluginGame *gamePlugin, QWidget *parent)
  : TutorableDialog("InstallDialog", parent), ui(new Ui::InstallDialog),
  m_Checker(gamePlugin->feature<ModDataChecker>()), m_DataFolderName(gamePlugin->dataDirectory().dirName().toLower()) {

  ui->setupUi(this);

  for (auto iter = modName.variants().begin(); iter != modName.variants().end(); ++iter) {
    ui->nameCombo->addItem(*iter);
  }

  ui->nameCombo->setCurrentIndex(ui->nameCombo->findText(modName));

  // Retrieve the tree from the UI and create the various root items (see the
  // members declaration for more details):
  m_Tree = findChild<ArchiveTreeWidget*>("treeContent");

  m_TreeRoot = new ArchiveTreeWidgetItem(tree);
  m_ViewRoot = new ArchiveTreeWidgetItem("<" + m_DataFolderName + ">");
  m_DataRoot = nullptr;

  m_Tree->addTopLevelItem(m_ViewRoot);

  // Connect the tree slots:
  connect(m_Tree, &ArchiveTreeWidget::treeCheckStateChanged, this, &InstallDialog::onTreeCheckStateChanged);
  connect(m_Tree, &ArchiveTreeWidget::itemMoved, this, &InstallDialog::onItemMoved);

  // Retrieve the label to display problems:
  m_ProblemLabel = findChild<QLabel*>("problemLabel");

  ui->nameCombo->completer()->setCaseSensitivity(Qt::CaseSensitive);

  setDataRoot(m_TreeRoot);
}

InstallDialog::~InstallDialog()
{
  delete ui;
}


QString InstallDialog::getModName() const
{
  return ui->nameCombo->currentText();
}

/**
 * @brief Retrieve the user-modified directory structure.
 *
 * @return the new tree represented by this dialog, which can be a new
 *     tree or a subtree of the original tree.
 **/
std::shared_ptr<MOBase::IFileTree> InstallDialog::getModifiedTree() const {
  return m_DataRoot->entry()->astree();
}


bool InstallDialog::testForProblem()
{
  if (!m_Checker) {
    return true;
  }
  return m_Checker->dataLooksValid(m_DataRoot->entry()->astree()) == ModDataChecker::CheckReturn::VALID;
}


void InstallDialog::updateProblems()
{
  if (!m_Checker) {
    m_Tree->setStyleSheet("QTreeWidget { border: none; }");
    m_ProblemLabel->setText(tr("Cannot check the content of <%1>.").arg(m_DataFolderName));
    m_ProblemLabel->setToolTip(tr("The plugin for the current game does not provide a way to check the content of <%1>.").arg(m_DataFolderName));
    m_ProblemLabel->setStyleSheet("color: darkYellow;");
  }
  else if (testForProblem()) {
    m_Tree->setStyleSheet("QTreeWidget { border: 1px solid darkGreen; border-radius: 2px; }");
    m_ProblemLabel->setText(tr("The content of <%1> looks valid.").arg(m_DataFolderName));
    m_ProblemLabel->setToolTip(tr("The content of <%1> seems valid for the current game.").arg(m_DataFolderName));
    m_ProblemLabel->setStyleSheet("color: darkGreen;");
  } else {
    m_Tree->setStyleSheet("QTreeWidget { border: 1px solid red; border-radius: 2px; }");
    m_ProblemLabel->setText(tr("The content of <%1> does not look valid.").arg(m_DataFolderName));
    m_ProblemLabel->setToolTip(tr("The content of <%1> is probably not valid for the current game.").arg(m_DataFolderName));
    m_ProblemLabel->setStyleSheet("color: red;");
  }
}


void InstallDialog::setDataRoot(ArchiveTreeWidgetItem* const root)
{
  if (root != m_DataRoot) {
    if (m_DataRoot != nullptr) {
      m_DataRoot->addChildren(m_ViewRoot->takeChildren());
    }

    // Force populate:
    root->populate();

    m_DataRoot = root;
    m_ViewRoot->setEntry(m_DataRoot->entry());
    m_ViewRoot->addChildren(m_DataRoot->takeChildren());
    m_ViewRoot->setExpanded(true);
  }
  updateProblems();
}


void InstallDialog::detachParents(ArchiveTreeWidgetItem* item) {
  auto entry = item->entry();
  auto parent = entry->parent();
  entry->detach();
  while (parent != nullptr && parent->empty()) {
    auto tmp = parent->parent();
    parent->detach();
    parent = tmp;
  }

}

void InstallDialog::attachParents(ArchiveTreeWidgetItem* item) {
  while (item->parent() != nullptr) {
    auto parent = static_cast<ArchiveTreeWidgetItem*>(item->parent());
    auto parentEntry = parent->entry();
    if (parentEntry != nullptr) {
      parentEntry->astree()->insert(item->entry());
    }
    item = parent;
  }
}


void InstallDialog::recursiveInsert(ArchiveTreeWidgetItem* item) {
  if (item->isPopulated()) {
    auto tree = item->entry()->astree();
    for (int i = 0; i < item->childCount(); ++i) {
      auto child = static_cast<ArchiveTreeWidgetItem*>(item->child(i));
      tree->insert(child->entry());
      if (child->entry()->isDir()) {
        recursiveInsert(child);
      }
    }
  }
}

void InstallDialog::recursiveDetach(ArchiveTreeWidgetItem* item) {
  if (item->isPopulated()) {
    for (int i = 0; i < item->childCount(); ++i) {
      auto child = static_cast<ArchiveTreeWidgetItem*>(item->child(i));
      if (child->entry()->isDir()) {
        recursiveDetach(child);
      }
    }
    item->entry()->astree()->clear();
  }
}


void InstallDialog::createDirectoryUnder(ArchiveTreeWidgetItem* item)
{
  // Should never happen if we customize the context menu depending
  // on the item:
  if (!item->entry()->isDir()) {
    reportError(tr("Cannot create directory under a file."));
    return;
  }

  // Retrieve the directory:
  auto fileTree = item->entry()->astree();

  bool ok = false;
  QString result = QInputDialog::getText(this, tr("Enter a directory name"), tr("Name"),
    QLineEdit::Normal, QString(), &ok);
  result = result.trimmed();

  if (ok && !result.isEmpty()) {

    // If a file with this name already exists:
    if (fileTree->exists(result)) {
      reportError(tr("A directory or file with that name already exists."));
      return;
    }

    // Need to expand to populate, and it's better for the user anyway:
    item->setExpanded(true);

    ArchiveTreeWidgetItem* newItem = new ArchiveTreeWidgetItem(
      item, fileTree->addDirectory(result));
    item->addChild(newItem);
    newItem->setCheckState(0, Qt::Checked);
    attachParents(item);
    updateProblems();

    m_Tree->scrollToItem(newItem);
  }
}

void InstallDialog::onItemMoved(ArchiveTreeWidgetItem* source, ArchiveTreeWidgetItem* target) {
  // just insert the source in the target.
  auto tree = target->entry()->astree();

  detachParents(source);

  // check if an entry exists with the same name, we check
  // in the tree widget to find unchecked items
  for (int i = 0; i < target->childCount(); ++i) {
    auto* child = target->child(i);
    if (child->entry()->compare(source->entry()->name()) == 0) {
      // remove existing file and force check existing directory
      if (child->entry()->isFile()) {
        target->removeChild(child);
      }
      else {
        child->setCheckState(0, Qt::Checked);
      }
      break;
    }
  }

  tree->insert(source->entry(), IFileTree::InsertPolicy::MERGE);

  attachParents(target);

  // Update the problem:
  updateProblems();
}

void InstallDialog::onTreeCheckStateChanged(ArchiveTreeWidgetItem* item) {

  auto entry = item->entry();

  // If the entry is a directory, we need to either detach or re-attach all the
  // children. It is not possible to only detach the directory because if the
  // user uncheck a directory and then check a file under it, the other files would
  // still be attached.
  //
  // The two recursive methods only go down to the expanded (based on isPopulated() tree, for
  // two reasons:
  //   1. If a tree item has not been populated, then detaching an entry from its parent will
  //      delete it since there would be no remaining shared pointers.
  //   2. If the tree has not been populated yet, all the entries under it are still attached,
  //      so there is no need to process them differently. Detaching a non-expanded item can
  //      be done by simply detaching the tree, no need to detach all the children.
  if (entry->isDir()) {
    if (item->checkState(0) == Qt::Checked && item->isPopulated()) {
      recursiveInsert(item);
    }
    else if (item->checkState(0) == Qt::Unchecked && item->isPopulated()) {
      recursiveDetach(item);
    }
  }

  // Unchecked: we go up the parent chain removing all trees that are now empty:
  if (item->checkState(0) == Qt::Unchecked) {
    detachParents(item);
  }
  // Otherwize, we need to-reattach the parent:
  else {
    attachParents(item);
  }

  updateProblems();
}


void InstallDialog::on_treeContent_customContextMenuRequested(QPoint pos)
{
  ArchiveTreeWidgetItem* selectedItem = static_cast<ArchiveTreeWidgetItem*>(m_Tree->itemAt(pos));
  if (selectedItem == nullptr) {
    return;
  }

  QMenu menu;

  if (selectedItem != m_ViewRoot && selectedItem->entry()->isDir()) {
    menu.addAction(tr("Set as <%1> directory").arg(m_DataFolderName), [this, selectedItem]() { setDataRoot(selectedItem); });
  }

  if (m_ViewRoot->entry() != m_TreeRoot->entry()) {
    menu.addAction(tr("Unset <%1> directory").arg(m_DataFolderName), [this]() { setDataRoot(m_TreeRoot); });
  }

  // Add a separator if not empty:
  if (!menu.isEmpty()) {
    menu.addSeparator();
  }

  if (selectedItem->entry()->isDir()) {
    menu.addAction(tr("Create directory..."), [this, selectedItem]() { createDirectoryUnder(selectedItem); });
  }
  else {
    menu.addAction(tr("&Open"), [this, selectedItem]() {
      emit openFile(selectedItem->entry().get());
    });
  }
  menu.exec(m_Tree->mapToGlobal(pos));
}


void InstallDialog::on_okButton_clicked()
{
  if (!testForProblem()) {
    if (QMessageBox::question(this, tr("Continue?"),
                              tr("This mod was probably NOT set up correctly, most likely it will NOT work. "
                                 "You should first correct the directory layout using the content-tree."),
                              QMessageBox::Ignore | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Cancel) {
      return;
    }
  }
  this->accept();
}

void InstallDialog::on_cancelButton_clicked()
{
  this->reject();
}
