/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkPGenericIOMultiBlockReader.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkPGenericIOMultiBlockReader.h -- Read GenericIO formatted data
//
// .SECTION Description
//  Creates a vtkMultiBlockDataSet from a GenericIO file

#ifndef vtkPGenericIOMultiBlockReader_h
#define vtkPGenericIOMultiBlockReader_h

#include "vtkPVVTKExtensionsCosmoToolsModule.h"
#include "vtkMultiBlockDataSetAlgorithm.h" // parent class

#include <set>

class vtkCallbackCommand;
class vtkDataArraySelection;
class vtkMultiProcessController;
class vtkStringArray;
class vtkIdList;
class vtkUnstructuredGrid;
class vtkInformationDoubleKey;

// GenericIO forward declarations
namespace gio
{
  class GenericIOReader;
}

class VTKPVVTKEXTENSIONSCOSMOTOOLS_EXPORT vtkPGenericIOMultiBlockReader : public vtkMultiBlockDataSetAlgorithm
{
public:

  enum IOType {
    IOTYPEMPI,
    IOTYPEPOSIX
  };

  enum BlockAssignment {
    ROUND_ROBIN,
    RCB
  };


  static vtkPGenericIOMultiBlockReader* New();
  vtkTypeMacro(vtkPGenericIOMultiBlockReader,vtkMultiBlockDataSetAlgorithm)
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Set/Get the variable name to be used as the x-axis for plotting particles.
  vtkSetStringMacro(XAxisVariableName)
  vtkGetStringMacro(XAxisVariableName)

  // Description:
  // Set/Get the variable name to be used as the y-axis for plotting particles.
  vtkSetStringMacro(YAxisVariableName)
  vtkGetStringMacro(YAxisVariableName)

  // Description:
  // Set/Get the variable name to be used as the z-axis for plotting particles.
  vtkSetStringMacro(ZAxisVariableName)
  vtkGetStringMacro(ZAxisVariableName)

  // Description:
  // Specify the name of the cosmology particle binary file to read
  vtkSetStringMacro(FileName)
  vtkGetStringMacro(FileName)

  // Description:
  // Set/Get the underlying IO method the reader will employ, i.e., MPI or POSIX.
  vtkSetMacro(GenericIOType,int)
  vtkGetMacro(GenericIOType,int)

  // Description:
  // Set/Get the underlying block-assignment strategy to use, i.e., ROUND_ROBIN,
  // or RCB.
  vtkSetMacro(BlockAssignment,int)
  vtkGetMacro(BlockAssignment,int)

  // Description:
  // Returns the list of arrays used to select the variables to be used
  // for the x,y and z axis.
  vtkGetObjectMacro(ArrayList,vtkStringArray)

  // Description:
  // Get the data array selection tables used to configure which data
  // arrays are loaded by the reader.
  vtkGetObjectMacro(PointDataArraySelection,vtkDataArraySelection)

  // Description:
  // Set/Get a multiprocess-controller for reading in parallel.
  // By default this parameter is set to NULL by the constructor.
  vtkGetMacro(Controller,vtkMultiProcessController*)
  vtkSetMacro(Controller,vtkMultiProcessController*)

  // Description:
  // Returns the number of arrays in the file, i.e., the number of columns.
  int GetNumberOfPointArrays();

  // Description:
  // Returns the name of the ith array.
  const char* GetPointArrayName(int i);

  // Description:
  // Returns the status of the array corresponding to the given name.
  int GetPointArrayStatus(const char* name);

  // Description:
  // Sets the status of the array corresponding to the given name.
  void SetPointArrayStatus(const char* name, int status);

  // Description:
  // Gets/Sets the variable name for the halo id of the particle.
  // This is used by the requested halo selector to select only the
  // points in the desired halos.
  vtkSetStringMacro(HaloIdVariableName)
  vtkGetStringMacro(HaloIdVariableName)

  // Description:
  // Gets the ith requested halo id.
  // If the number of requested halo ids is
  // greater than 0, only points with those halo ids will be read in.
  // Otherwise all points will be read in.
  vtkIdType GetRequestedHaloId(vtkIdType i);

  // Description:
  // Gets the number of requested halo ids.
  // If the number of requested halo ids is
  // greater than 0, only points with those halo ids will be read in.
  // Otherwise all points will be read in.
  vtkIdType GetNumberOfRequestedHaloIds();

  // Description:
  // Sets the number of requested halo ids.
  // Use SetRequestedHaloId() to se the ids after this is called
  // If the number of requested halo ids is
  // greater than 0, only points with those halo ids will be read in.
  // Otherwise all points will be read in.
  void SetNumberOfRequestedHaloIds(vtkIdType numIds);

  // Description:
  // Adds the given halo id to the list of halo ids to request.
  // If the number of requested halo ids is
  // greater than 0, only points with those halo ids will be read in.
  // Otherwise all points will be read in.
  void AddRequestedHaloId(vtkIdType haloId);

  // Description:
  // Clears the list of requested halo ids.
  // If the number of requested halo ids is
  // greater than 0, only points with those halo ids will be read in.
  // Otherwise all points will be read in.
  void ClearRequestedHaloIds();

  // Description:
  // Sets the ith requested halo id to the given haloId.
  // If the number of requested halo ids is
  // greater than 0, only points with those halo ids will be read in.
  // Otherwise all points will be read in.
  void SetRequestedHaloId(vtkIdType i, vtkIdType haloId);


protected:
  vtkPGenericIOMultiBlockReader();
  ~vtkPGenericIOMultiBlockReader();

  char* XAxisVariableName;
  char* YAxisVariableName;
  char* ZAxisVariableName;
  char* HaloIdVariableName;

  char* FileName;
  int GenericIOType;
  int BlockAssignment;

  bool BuildMetaData;

  vtkMultiProcessController* Controller;

  vtkStringArray* ArrayList;
  vtkDataArraySelection* PointDataArraySelection;
  vtkIdList* HaloList;
  vtkCallbackCommand* SelectionObserver;

  gio::GenericIOReader* Reader;

  gio::GenericIOReader* GetInternalReader();

  bool ReaderParametersChanged();

  void LoadMetaData();

  void LoadRawVariableDataForBlock(const std::string& varName, int blockId);

  void LoadRawDataForBlock(int blockId);

  void GetPointFromRawData(
      int xType, void* xBuffer, int yType, void* yBuffer, int zType,
      void* zBuffer, vtkIdType id, double point[3]);

  void LoadCoordinatesForBlock(vtkUnstructuredGrid* grid,
                               std::set< vtkIdType >& pointsInSelectedHalos,
                               int blockId);

  void LoadDataArraysForBlock(vtkUnstructuredGrid* grid,
                              const std::set<vtkIdType>& pointsInSelectedHalos,
                              int blockId);

  vtkUnstructuredGrid* LoadBlock(int blockId);

  // Descriptions
  // Call-back registered with the SelectionObserver.
  static void SelectionModifiedCallback(
    vtkObject *caller,unsigned long eid,
    void *clientdata,void *calldata );

  // Pipeline methods
  virtual int RequestInformation(
      vtkInformation*,vtkInformationVector**,vtkInformationVector*);
  virtual int RequestData(
      vtkInformation*,vtkInformationVector**,vtkInformationVector*);


private:
  // not implemented
  vtkPGenericIOMultiBlockReader(const vtkPGenericIOMultiBlockReader& other);
  vtkPGenericIOMultiBlockReader& operator=(
      const vtkPGenericIOMultiBlockReader& other);
  // Internal helper class
  class vtkGenericIOMultiBlockMetaData;
  vtkGenericIOMultiBlockMetaData* MetaData;
};

#endif
