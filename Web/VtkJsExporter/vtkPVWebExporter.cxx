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
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMUndoStackBuilder.h"
#include "vtkSMViewProxy.h"

#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
#include "vtkCamera.h"
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
  const vtkPVWebExporter::ViewPointsType& viewPoints,
  const vtkPVWebExporter::ViewPointsVisibilitiesType& viewPointsVisibilities)
{
  // This function should be called with the Python interpreter
  // already initialized.

  vtkSmartPyObject res = PyDict_New();
  for (const auto& e: viewPoints)
  {
    const auto& name = e.first;
    auto* cameraPtr = e.second;

    vtkSmartPyObject viewPointDict = PyDict_New();

    vtkSmartPyObject camera = vtkPythonUtil::GetObjectFromPointer(cameraPtr);
    PyDict_SetItemString(viewPointDict, "camera", camera);

    // Try to get the visibilities as well
    auto it = viewPointsVisibilities.find(name);
    if (it != viewPointsVisibilities.end())
    {
      vtkSmartPyObject showList = PyList_New(0);
      vtkSmartPyObject hideList = PyList_New(0);
      for (const auto& e: it->second)
      {
        vtkSmartPyObject proxy = vtkPythonUtil::GetObjectFromPointer(e.first);
        auto& listToUse = e.second ? showList : hideList;
        PyList_Append(listToUse, proxy);
      }
      PyDict_SetItemString(viewPointDict, "show", showList);
      PyDict_SetItemString(viewPointDict, "hide", hideList);
    }

    PyDict_SetItemString(res, name.c_str(), std::move(viewPointDict));
  }
  return std::move(res);
}
}
#endif

namespace {
//----------------------------------------------------------------------------
std::vector<vtkSMRepresentationProxy*> getRepsToShow(
  const vtkPVWebExporter::ViewPointsVisibilitiesType& viewPointsVisibilities)
{
  // Get a vector of unique representation proxies that are currently
  // not visible, but are visible in one or more view points (and thus
  // need to be written out).
  std::vector<vtkSMRepresentationProxy*> ret;

  // Loop through view points
  for (const auto& e: viewPointsVisibilities)
  {
    // Loop through the visibilities of each actor
    for (const auto& v: e.second)
    {
      // Is the actor visible in this view point?
      if (v.second)
      {
        auto* rep = vtkSMRepresentationProxy::SafeDownCast(v.first);
        if (rep)
        {
          bool vis = vtkSMPropertyHelper(rep, "Visibility").GetAsInt();
          if (!vis && std::find(ret.begin(), ret.end(), rep) == ret.end())
          {
            // If it's not currently visible, and not already in our
            // vector, let's add it.
            ret.push_back(rep);
          }
        }
      }
    }
  }

  return ret;
}

//----------------------------------------------------------------------------
bool blockUndoStack(bool block)
{
  // Returns the previous state
  auto* usb = vtkSMProxyManager::GetProxyManager()->GetUndoStackBuilder();
  if (!usb)
  {
    return false;
  }

  bool prev = usb->GetIgnoreAllChanges();
  usb->SetIgnoreAllChanges(block);
  return prev;
}

//----------------------------------------------------------------------------
vtkSMViewProxy* getActiveView()
{
  auto* pxm = vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  auto* viewSM = pxm->GetSelectionModel("ActiveView");
  if (!viewSM)
  {
    vtkGenericWarningMacro("Failed to get ActiveView.");
    return nullptr;
  }
  return vtkSMViewProxy::SafeDownCast(viewSM->GetCurrentProxy());
}

//----------------------------------------------------------------------------
void setVisibilities(const std::vector<vtkSMRepresentationProxy*>& reps,
                     bool visible)
{
  if (reps.empty())
  {
    // Nothing to do...
    return;
  }

  auto* view = getActiveView();
  if (!view)
  {
    return;
  }

  // Don't modify the undo stack
  bool prevBlocked = blockUndoStack(true);

  for (auto* rep: reps)
  {
    vtkSMPropertyHelper(rep, "Visibility").Set(visible);
    rep->UpdateVTKObjects();
  }

  // A render is required to update the actors and mappers
  view->StillRender();

  blockUndoStack(prevBlocked);
}

//----------------------------------------------------------------------------
struct HideRepsOnDestruct
{
  // Just a helper struct to automatically hide the stored
  // representations when this is destroyed...

  ~HideRepsOnDestruct()
  {
    try
    {
      setVisibilities(this->RepsToHide, false);
    }
    catch (...)
    {
    }
  }
  std::vector<vtkSMRepresentationProxy*> RepsToHide;
};
}

//----------------------------------------------------------------------------
void vtkPVWebExporter::Write()
{
  HideRepsOnDestruct repsHider;
  if (!this->ViewPoints.empty() && !this->ViewPointsVisibilities.empty())
  {
    // There may be actors which are currently not visible in the
    // main window, but are visible in a different view point.
    // In order for the data sets of these actors to be written out,
    // we need to make them visible and render, so that the mappers
    // will be up to date. The actors will become invisible again after
    // writing is complete.
    std::vector<vtkSMRepresentationProxy*> reps = getRepsToShow(this->ViewPointsVisibilities);
    setVisibilities(reps, true);
    repsHider.RepsToHide = reps;
  }

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
      vtkSmartPyObject viewPointsDict = createViewPointsDict(
        this->ViewPoints, this->ViewPointsVisibilities);

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
