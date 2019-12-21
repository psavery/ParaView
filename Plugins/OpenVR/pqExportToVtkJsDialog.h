/*=========================================================================

  Program:   ParaView

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   pqExportToVtkJsDialog
 * @brief   PV GUI dialog for exporting OpenVR to VTK.js
 *
 * This class exists as part of the desktop GUI for ParaView and provides
 * options for exporting to VTK.js.
 */

#ifndef pqExportToVtkJsDialog_h
#define pqExportToVtkJsDialog_h

#include <QDialog>

class pqExportToVtkJsDialog : public QDialog
{
  Q_OBJECT

public:
  explicit pqExportToVtkJsDialog(QWidget* parent = 0);
  ~pqExportToVtkJsDialog();

  void saveSettings() const;
  void loadSettings();

  QString saveFileName() const;
  bool embedInGlanceHtml() const;
  QString paraviewGlanceFileName() const;
  bool writeTextures() const;
  bool writeTextureLODs() const;
  QString textureLODsBaseUrl() const;
  size_t textureLODsBaseSize() const;

public slots:
  void accept() override;
  int exec() override;

private slots:
  void selectSaveFile();
  void selectParaViewGlanceFile();

private:
  void setupConnections();

  class pqInternals;
  pqInternals* Internals;
};

#endif // pqExportToVtkJsDialog_h
