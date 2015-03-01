#ifndef __VectorToVertexNormalFilter_h
#define __VectorToVertexNormalFilter_h
 
#include "vtkPolyDataAlgorithm.h"
 
class VectorToVertexNormalFilter : public vtkPolyDataAlgorithm 
{
public:
  vtkTypeMacro(VectorToVertexNormalFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  double getMin(){return m_min;}
  double getMax(){return m_max;}

  static VectorToVertexNormalFilter *New();
 
protected:
  VectorToVertexNormalFilter();
  ~VectorToVertexNormalFilter();
 
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  VectorToVertexNormalFilter(const VectorToVertexNormalFilter&);  // Not implemented.
  void operator=(const VectorToVertexNormalFilter&);  // Not implemented.
  double m_min;
  double m_max;
};
#endif


