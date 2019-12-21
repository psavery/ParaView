/*=========================================================================

  Program:   ParaView

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "pqExportToVtkJsDialog.h"
#include "ui_pqExportToVtkJsDialog.h"

#include "pqApplicationCore.h"
#include "pqSettings.h"

#include <QFileDialog>

class pqExportToVtkJsDialog::pqInternals : public Ui::pqExportToVtkJsDialog
{
};

pqExportToVtkJsDialog::pqExportToVtkJsDialog(QWidget* parent)
  : QDialog(parent)
  , Internals(new pqInternals)
{
  this->Internals->setupUi(this);

  this->setWindowTitle("Export to VTK.js");

  this->setupConnections();
}

pqExportToVtkJsDialog::~pqExportToVtkJsDialog()
{
  delete this->Internals;
}

void pqExportToVtkJsDialog::setupConnections()
{
  connect(this->Internals->selectSaveFileButton, &QPushButton::pressed, this,
    &pqExportToVtkJsDialog::selectSaveFile);
  connect(this->Internals->selectParaViewGlanceFileButton, &QPushButton::pressed, this,
    &pqExportToVtkJsDialog::selectParaViewGlanceFile);
}

void pqExportToVtkJsDialog::accept()
{
  // Might be good to also validate here that the paraview glance html
  // is a real file.
  this->saveSettings();
  QDialog::accept();
}

int pqExportToVtkJsDialog::exec()
{
  this->loadSettings();
  return QDialog::exec();
}

void pqExportToVtkJsDialog::saveSettings() const
{
  auto* settings = pqApplicationCore::instance()->settings();
  settings->beginGroup("OpenVR");
  settings->beginGroup("ExportToVtkJsDialog");

  settings->setValue("saveFileName", this->saveFileName());
  settings->setValue("embedInGlanceHtml", this->embedInGlanceHtml());
  settings->setValue("paraviewGlanceFileName", this->paraviewGlanceFileName());
  settings->setValue("writeTextures", this->writeTextures());
  settings->setValue("writeTextureLODs", this->writeTextureLODs());
  settings->setValue("textureLODsBaseUrl", this->textureLODsBaseUrl());
  settings->setValue("textureLODsBaseSize", static_cast<qulonglong>(this->textureLODsBaseSize()));

  settings->endGroup();
  settings->endGroup();
}

void pqExportToVtkJsDialog::loadSettings()
{
  auto* settings = pqApplicationCore::instance()->settings();
  settings->beginGroup("OpenVR");
  settings->beginGroup("ExportToVtkJsDialog");

  this->Internals->saveFileName->setText(
    settings->value("saveFileName", "file_name.vtkjs").toString());
  this->Internals->embedInGlanceHtml->setChecked(
    settings->value("embedInGlanceHtml", "false").toBool());
  this->Internals->paraviewGlanceFileName->setText(
    settings->value("paraviewGlanceFileName", "ParaViewGlance.html").toString());
  this->Internals->writeTextures->setChecked(settings->value("writeTextures", "false").toBool());
  this->Internals->writeTextureLODs->setChecked(
    settings->value("writeTextureLODs", "false").toBool());
  this->Internals->textureLODsBaseUrl->setText(
    settings->value("textureLODsBaseUrl", "").toString());
  this->Internals->textureLODsBaseSize->setValue(
    settings->value("textureLODsBaseSize", "100000").toULongLong() / 1000);

  settings->endGroup();
  settings->endGroup();
}

QString pqExportToVtkJsDialog::saveFileName() const
{
  return this->Internals->saveFileName->text();
}

bool pqExportToVtkJsDialog::embedInGlanceHtml() const
{
  return this->Internals->embedInGlanceHtml->isChecked();
}

QString pqExportToVtkJsDialog::paraviewGlanceFileName() const
{
  return this->Internals->paraviewGlanceFileName->text();
}

bool pqExportToVtkJsDialog::writeTextures() const
{
  return this->Internals->writeTextures->isChecked();
}

bool pqExportToVtkJsDialog::writeTextureLODs() const
{
  return this->Internals->writeTextureLODs->isChecked();
}

QString pqExportToVtkJsDialog::textureLODsBaseUrl() const
{
  return this->Internals->textureLODsBaseUrl->text();
}

size_t pqExportToVtkJsDialog::textureLODsBaseSize() const
{
  return static_cast<size_t>(this->Internals->textureLODsBaseSize->value()) * 1000;
}

void pqExportToVtkJsDialog::selectSaveFile()
{
  auto file = QFileDialog::getSaveFileName(
    this, "Select Save File", this->saveFileName(), "VTK.js files (*.vtkjs)");

  if (file.isEmpty())
  {
    return;
  }

  if (!file.endsWith(".vtkjs"))
  {
    file += ".vtkjs";
  }

  this->Internals->saveFileName->setText(file);
}

void pqExportToVtkJsDialog::selectParaViewGlanceFile()
{
  auto file = QFileDialog::getOpenFileName(this, "Select ParaView Glance File",
    this->paraviewGlanceFileName(), "ParaView Glance files (*.html)");

  if (file.isEmpty())
  {
    return;
  }

  this->Internals->paraviewGlanceFileName->setText(file);
}
