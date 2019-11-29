/*=========================================================================

  Program:   ParaView
  Module:    vtkPVWebExporter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVWebExporter
 * @brief   vtkPVWebExporter is used to produce vtkjs export in the ParaView
 * application.
 *
 * vtkPVWebExporter is used to produce scene export for web sharing.
 */

#ifndef vtkPVWebExporter_h
#define vtkPVWebExporter_h

#include "vtkJSONSceneExporter.h"
#include "vtkPVWebExporterModule.h" // needed for exports

#include <map>
#include <string>

class vtkCamera;

class VTKPVWEBEXPORTER_EXPORT vtkPVWebExporter : public vtkJSONSceneExporter
{
public:
  static vtkPVWebExporter* New();
  vtkTypeMacro(vtkPVWebExporter, vtkJSONSceneExporter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify file name of the ParaViewGlance HTML file to use to embed the data in
   */
  vtkSetStringMacro(ParaViewGlanceHTML);
  vtkGetStringMacro(ParaViewGlanceHTML);
  //@}

  //@{
  /**
   * Specify extra view points to be written out to the file.
   *
   * The key is the name of the view point.
   */
  using ViewPointsType = std::map<std::string, vtkCamera*>;
  vtkSetMacro(ViewPoints, ViewPointsType);
  vtkGetMacro(ViewPoints, ViewPointsType);
  //@}

  //@{
  /**
   * Specify the visibilities for each view point.
   *
   * The "ViewPointsVisibilitiesType" key is the name of the view point.
   * The "VisibilitiesType" key is the representation proxy.
   *
   * "vtkObjectBase*" is used instead of "vtkSMProxy*" so we don't need
   * to include vtkSMProxy.h (and add it as a cmake dependency) in order
   * to use vtkPythonUtil::GetObjectFromPointer() on it.
   */
  using VisibilitiesType = std::map<vtkObjectBase*, bool>;
  using ViewPointsVisibilitiesType = std::map<std::string, VisibilitiesType>;
  vtkSetMacro(ViewPointsVisibilities, ViewPointsVisibilitiesType);
  vtkGetMacro(ViewPointsVisibilities, ViewPointsVisibilitiesType);
  //@}

protected:
  vtkPVWebExporter();
  ~vtkPVWebExporter() override;

  // Decorate method to enable zip bundling
  void Write() override;

private:
  vtkPVWebExporter(const vtkPVWebExporter&) = delete;
  void operator=(const vtkPVWebExporter&) = delete;

  char* ParaViewGlanceHTML;
  ViewPointsType ViewPoints;
  ViewPointsVisibilitiesType ViewPointsVisibilities;
};

#endif
