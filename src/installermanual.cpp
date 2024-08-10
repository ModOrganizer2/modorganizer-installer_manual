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

#include "installermanual.h"

#include <QDialog>
#include <QtPlugin>

#include <uibase/game_features/igamefeatures.h>
#include <uibase/game_features/moddatachecker.h>
#include <uibase/iinstallationmanager.h>
#include <uibase/iplugingame.h>
#include <uibase/utility.h>

#include "installdialog.h"

using namespace MOBase;

InstallerManual::InstallerManual() : m_MOInfo(nullptr) {}

bool InstallerManual::init(IOrganizer* moInfo)
{
  m_MOInfo = moInfo;
  return true;
}

QString InstallerManual::name() const
{
  return "Manual Installer";
}

QString InstallerManual::localizedName() const
{
  return tr("Manual Installer");
}

QList<Setting> InstallerManual::settings() const
{
  return {};
}

unsigned int InstallerManual::priority() const
{
  return 0;
}

bool InstallerManual::isManualInstaller() const
{
  return true;
}

bool InstallerManual::isArchiveSupported(std::shared_ptr<const MOBase::IFileTree>) const
{
  return true;
}

void InstallerManual::openFile(const FileTreeEntry* entry)
{
  QString tempName = manager()->extractFile(entry->shared_from_this());
  shell::Open(tempName);
}

IPluginInstaller::EInstallResult
InstallerManual::install(GuessedValue<QString>& modName,
                         std::shared_ptr<MOBase::IFileTree>& tree, QString&, int&)
{
  qDebug("offering installation dialog");
  InstallDialog dialog(
      tree, modName, m_MOInfo->gameFeatures()->gameFeature<ModDataChecker>(),
      m_MOInfo->managedGame()->dataDirectory().dirName().toLower(), parentWidget());
  connect(&dialog, &InstallDialog::openFile, this, &InstallerManual::openFile);
  if (dialog.exec() == QDialog::Accepted) {
    modName.update(dialog.getModName(), GUESS_USER);

    // TODO probably more complicated than necessary
    tree = dialog.getModifiedTree();
    return IPluginInstaller::RESULT_SUCCESS;
  } else {
    return IPluginInstaller::RESULT_CANCELED;
  }
}

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
Q_EXPORT_PLUGIN2(installerManual, InstallerManual)
#endif
