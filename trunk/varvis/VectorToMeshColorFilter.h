#ifndef __VectorToMeshColorFilter_h
#define __VectorToMeshColorFilter_h
 
#include <vtkPolyDataAlgorithm.h>
#include <vtkScalarsToColors.h>
#include <vtkSmartPointer.h>
class VectorToMeshColorFilter : public vtkPolyDataAlgorithm 
{
public:
  vtkTypeMacro(VectorToMeshColorFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
 
  static VectorToMeshColorFilter *New();
  float GetMin() { return m_min; }
  float GetMax() { return m_max; }

  /// Set LUT to be used for mesh coloring, set this before executing algorithm.
  void SetLookupTable( vtkSmartPointer<vtkScalarsToColors> lut );

  /// Return current LUT, creates a default LUT when none is present.
  vtkScalarsToColors* GetLookupTable();
 
protected:
  VectorToMeshColorFilter();
  ~VectorToMeshColorFilter();
 
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  VectorToMeshColorFilter(const VectorToMeshColorFilter&);  // Not implemented.
  void operator=(const VectorToMeshColorFilter&);  // Not implemented.

  float m_min,m_max;

  vtkSmartPointer<vtkScalarsToColors> m_lookupTable;
};
#endif


