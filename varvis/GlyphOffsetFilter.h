#ifndef __GlyphOffsetFilter_h
#define __GlyphOffsetFilter_h
 
#include "vtkPolyDataAlgorithm.h"
 
class GlyphOffsetFilter : public vtkPolyDataAlgorithm 
{
public:
  vtkTypeMacro(GlyphOffsetFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  void SetOffset(double delta){m_delta=delta;}
  static GlyphOffsetFilter *New();
 
protected:
  GlyphOffsetFilter();
  ~GlyphOffsetFilter();
 
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  GlyphOffsetFilter(const GlyphOffsetFilter&);  // Not implemented.
  void operator=(const GlyphOffsetFilter&);  // Not implemented.
  double m_delta;
};
 
#endif


