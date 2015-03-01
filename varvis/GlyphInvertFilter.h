#ifndef __GlyphInvertFilter_h
#define __GlyphInvertFilter_h
 
#include "vtkPolyDataAlgorithm.h"
 
class GlyphInvertFilter : public vtkPolyDataAlgorithm 
{
public:
  vtkTypeMacro(GlyphInvertFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
   static GlyphInvertFilter *New();
 
protected:
  GlyphInvertFilter();
  ~GlyphInvertFilter();
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  GlyphInvertFilter(const GlyphInvertFilter&);  // Not implemented.
  void operator=(const GlyphInvertFilter&);  // Not implemented.
};
 
#endif


