/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTensorGlyph3.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTensorGlyph3 - scale and orient glyph(s) according to tensor eigenvalues and eigenvectors
// .SECTION Description
// vtkTensorGlyph3 is a filter that copies a geometric representation
// (specified as polygonal data) to every input point. The geometric
// representation, or glyph, can be scaled and/or rotated according to
// the tensor at the input point. Scaling and rotation is controlled
// by the eigenvalues/eigenvectors of the tensor as follows. For each
// tensor, the eigenvalues (and associated eigenvectors) are sorted to
// determine the major, medium, and minor eigenvalues/eigenvectors.
//
// vtkTensorGlyph3 is a custom adaption of vtkTensorGlyph2.
// 
// vtkTensorGlyph2 differs from vtkTensorGlyph in that it names the data array
// it creates such that ParaView will be able to use them for coloring and
// further processing.

// .SECTION See Also
// vtkTensorGlyph vtkGlyph3D vtkPointLoad vtkHyperStreamline

#ifndef __vtkTensorGlyph3_h
#define __vtkTensorGlyph3_h

#include "vtkTensorGlyph.h"
#include "vtkSmartPointer.h"
#include "vtkSuperquadricSource.h"

// Windows specific stuff------------------------------------------
#if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__)
#  if defined(VTK_BUILD_SHARED_LIBS)
#    define VTK_TENSORGLYPH_EXPORT  __declspec( dllexport )
#  else
#    define VTK_TENSORGLYPH_EXPORT
#  endif
#else
#  define VTK_TENSORGLYPH_EXPORT
#endif

class VTK_TENSORGLYPH_EXPORT vtkTensorGlyph3 : public vtkTensorGlyph
{
public:
  vtkTypeMacro(vtkTensorGlyph3,vtkTensorGlyph);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Construct object with scaling on and scale factor 1.0. Eigenvalues are
  // extracted, glyphs are colored with input scalar data, and logarithmic
  // scaling is turned off.
  static vtkTensorGlyph3 *New();

  // Description:
  // Turn on/off translation by additional vector data
  vtkSetMacro(VectorDisplacement,int);
  vtkGetMacro(VectorDisplacement,int);
  vtkBooleanMacro(VectorDisplacement,int);

  // Description:
  // Specify scale factor for vector displacement.
  vtkSetMacro(VectorDisplacementFactor,double);
  vtkGetMacro(VectorDisplacementFactor,double);

  // Description:
  // Offset eigenvalues by a constant value.
  vtkSetMacro(EigenvalueOffsetValue,double);
  vtkGetMacro(EigenvalueOffsetValue,double);

  // Description:
  // Offset eigenvalues by a constant value.
  vtkSetMacro(EigenvalueOffset,int);
  vtkGetMacro(EigenvalueOffset,int);
  vtkBooleanMacro(EigenvalueOffset,int);

  // Description:
  // Scake eigenvalues by sqrt().
  vtkSetMacro(EigenvalueSqrtScaling,int);
  vtkGetMacro(EigenvalueSqrtScaling,int);
  vtkBooleanMacro(EigenvalueSqrtScaling,int);


  // HACK BELOW: Is it O.K. to simply extend the enum from the base class
  //             and overwrite the get/set functions?

//BTX
  enum
  {
      COLOR_BY_SCALARS,
      COLOR_BY_EIGENVALUES,
	  COLOR_BY_FRACTIONAL_ANISOTROPY
  };
//ETX

  // Description:
  // Set the color mode to be used for the glyphs.  This can be set to
  // use the input scalars (default) or to use the eigenvalues at the
  // point.  If ThreeGlyphs is set and the eigenvalues are chosen for
  // coloring then each glyph is colored by the corresponding
  // eigenvalue and if not set the color corresponding to the largest
  // eigenvalue is chosen.  The recognized values are:
  // COLOR_BY_SCALARS = 0 (default)
  // COLOR_BY_EIGENVALUES = 1
  vtkSetClampMacro(ColorMode, int, COLOR_BY_SCALARS, COLOR_BY_FRACTIONAL_ANISOTROPY);
  vtkGetMacro(ColorMode, int);
  void SetColorModeToScalars()
    {this->SetColorMode(COLOR_BY_SCALARS);};
  void SetColorModeToEigenvalues()
    {this->SetColorMode(COLOR_BY_EIGENVALUES);};  
  void SetColorModeToFractionalAnisotropy()
    {this->SetColorMode(COLOR_BY_FRACTIONAL_ANISOTROPY);};  

  // [Max]
  // Stupid workaround to get access to source algorithm. By default the 
  // PolyDataAlgorithms seems only to have access to the produced DataObject
  // but not the algorithm itself?
  void SetSuperquadricSource( vtkSmartPointer<vtkSuperquadricSource> sqsrc )
  {
	  m_superquadricSource = sqsrc;
  }  
  vtkSuperquadricSource* GetSuperquadricSource() { return m_superquadricSource; }

  // Description:
  // Scake eigenvalues by sqrt().
  vtkSetMacro(SuperquadricGamma,double);
  vtkGetMacro(SuperquadricGamma,double);

protected:
  vtkTensorGlyph3();
  ~vtkTensorGlyph3();

  // Data generation method
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  vtkTensorGlyph3(const vtkTensorGlyph3&);  // Not implemented.
  void operator=(const vtkTensorGlyph3&);  // Not implemented.

  int VectorDisplacement; // Boolean controls displacement by vector data
  double VectorDisplacementFactor;

  int EigenvalueOffset;
  double EigenvalueOffsetValue;

  int EigenvalueSqrtScaling;

  vtkSmartPointer<vtkSuperquadricSource> m_superquadricSource;
  double SuperquadricGamma;
};

#endif
