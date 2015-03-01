#ifndef __PointSamplerFilter_h
#define __PointSamplerFilter_h
 
#include "vtkPolyDataAlgorithm.h"
#include "vtkScalarsToColors.h"
class PointSamplerFilter : public vtkPolyDataAlgorithm 
{
public:
  vtkTypeMacro(PointSamplerFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
 
  static PointSamplerFilter *New();
 
  void setNumberOfSamplingPoints(int num){numberOfSamplingPoints=num;}
  void saveMeshNormas(bool yes){useMeshNormals=yes;}
protected:
  PointSamplerFilter();
  ~PointSamplerFilter();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  PointSamplerFilter(const PointSamplerFilter&);  // Not implemented.
  void operator=(const PointSamplerFilter&);  // Not implemented.

  bool useMeshNormals;
  int numberOfSamplingPoints;
};
 
#endif


