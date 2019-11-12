/*=========================================================================

  Program:   ParaView
  Module:    vtkPVWebExporter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVWebExporter.h"

#include "vtkObjectFactory.h"

#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
#include <vtkCamera.h>
#include "vtkPython.h"
#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSmartPyObject.h"
#endif

vtkStandardNewMacro(vtkPVWebExporter);
//----------------------------------------------------------------------------
vtkPVWebExporter::vtkPVWebExporter()
{
  this->ParaViewGlanceHTML = nullptr;
}

//----------------------------------------------------------------------------
vtkPVWebExporter::~vtkPVWebExporter()
{
  delete[] this->ParaViewGlanceHTML;
}

//----------------------------------------------------------------------------
void vtkPVWebExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
namespace {
//----------------------------------------------------------------------------
vtkSmartPyObject createViewPointsDict(
  const vtkPVWebExporter::ViewPointsType& viewPoints)
{
  // This function should be called with the Python interpreter
  // already initialized.

  vtkSmartPyObject viewPointsDict = PyDict_New();
  for (const auto& e: viewPoints)
  {
    const auto& name = e.first;
    auto* cameraPtr = e.second;

    vtkSmartPyObject camera = vtkPythonUtil::GetObjectFromPointer(cameraPtr);
    PyDict_SetItemString(viewPointsDict, name.c_str(), camera);
  }

  return std::move(viewPointsDict);
}
}
#endif

//----------------------------------------------------------------------------
void vtkPVWebExporter::Write()
{
  this->Superclass::Write();

#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore

  // ensure Python interpreter is initialized.
  vtkPythonInterpreter::Initialize();
  vtkPythonScopeGilEnsurer gilEnsurer;
  try
  {
    vtkSmartPyObject pvmodule(PyImport_ImportModule("paraview.web.vtkjs_helper"));
    if (!pvmodule || PyErr_Occurred())
    {
      vtkGenericWarningMacro("Failed to import paraview.web.vtkjs_helper module.");
      throw 1;
    }

    PyObject_CallMethod(pvmodule, const_cast<char*>("applyParaViewNaming"),
      const_cast<char*>("(s)"), const_cast<char*>(this->FileName));
    if (PyErr_Occurred())
    {
      vtkGenericWarningMacro("Failed to rename datasets using ParaView proxy name");
      throw 1;
    }

    // Add additional viewpoints to the file, if there are any
    if (!this->ViewPoints.empty())
    {
      vtkSmartPyObject viewPointsDict = createViewPointsDict(this->ViewPoints);

      PyObject_CallMethod(pvmodule, const_cast<char*>("addViewPoints"),
        const_cast<char*>("(sO)"), const_cast<char*>(this->FileName),
        viewPointsDict.GetPointer());
      if (PyErr_Occurred())
      {
        vtkGenericWarningMacro("Failed to add viewpoints to vtkjs file");
        throw 1;
      }
    }

    vtkSmartPyObject module(PyImport_ImportModule("vtk.web.vtkjs_helper"));
    if (!module || PyErr_Occurred())
    {
      vtkGenericWarningMacro("Failed to import vtk.web.vtkjs_helper module.");
      throw 1;
    }

    PyObject_CallMethod(module, const_cast<char*>("convertDirectoryToZipFile"),
      const_cast<char*>("(s)"), const_cast<char*>(this->FileName));
    if (PyErr_Occurred())
    {
      vtkGenericWarningMacro("Failed to bundle vtkjs file");
      throw 1;
    }

    if (this->ParaViewGlanceHTML)
    {
      PyObject_CallMethod(module, const_cast<char*>("addDataToViewer"), const_cast<char*>("(ss)"),
        const_cast<char*>(this->FileName), const_cast<char*>(this->ParaViewGlanceHTML));
    }
    if (PyErr_Occurred())
    {
      vtkGenericWarningMacro("Failed to bundle vtkjs data file into HTML viewer");
      throw 1;
    }
  }
  catch (int)
  {
    if (PyErr_Occurred())
    {
      PyErr_Print();
      PyErr_Clear();
    }
  }
#endif
}
