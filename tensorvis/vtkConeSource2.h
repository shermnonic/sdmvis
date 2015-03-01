/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConeSource2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkConeSource2 - generate polygonal cone 
// .SECTION Description
// vtkConeSource2 creates a cone centered at a specified point and pointing in
// a specified direction. (By default, the center is the origin and the
// direction is the x-axis.) In contrast to vtkConeSource no different 
// representations for different resolutions are created. The minimum resolution
// is four faces.
// It also is possible to control
// whether the bottom of the cone is capped with a (resolution-sided)
// polygon, and to specify the height and radius of the cone.

#ifndef __vtkConeSource2_h
#define __vtkConeSource2_h

#include "vtkPolyDataAlgorithm.h"

#include "vtkCell.h" // Needed for VTK_CELL_SIZE

class /*VTK_GRAPHICS_EXPORT*/ vtkConeSource2 : public vtkPolyDataAlgorithm 
{
public:
  vtkTypeMacro(vtkConeSource2,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with default resolution 6, height 1.0, radius 0.5, and
  // capping on. The cone is centered at the origin and points down
  // the x-axis.
  static vtkConeSource2 *New();

  // Description:
  // Set the height of the cone. This is the height along the cone in
  // its specified direction.
  vtkSetClampMacro(Height,double,0.0,VTK_DOUBLE_MAX)
  vtkGetMacro(Height,double);

  // Description:
  // Set the base radius of the cone.
  vtkSetClampMacro(Radius,double,0.0,VTK_DOUBLE_MAX)
  vtkGetMacro(Radius,double);

  // Description:
  // Set the number of facets used to represent the cone.
  vtkSetClampMacro(Resolution,int,0,VTK_CELL_SIZE)
  vtkGetMacro(Resolution,int);

  // Description:
  // Set the center of the cone. It is located at the middle of the axis of
  // the cone. Warning: this is not the center of the base of the cone!
  // The default is 0,0,0.
  vtkSetVector3Macro(Center,double);
  vtkGetVectorMacro(Center,double,3);

  // Description:
  // Set the orientation vector of the cone. The vector does not have
  // to be normalized. The direction goes from the center of the base toward
  // the apex. The default is (1,0,0).
  vtkSetVector3Macro(Direction,double);
  vtkGetVectorMacro(Direction,double,3);
  
  // Description:
  // Set the first principal axis for the cap.
  vtkSetVector3Macro(Principal1,double);
  vtkGetVectorMacro(Principal1,double,3);  
  
  // Description:
  // Set the second principal axis for the cap.
  vtkSetVector3Macro(Principal2,double);
  vtkGetVectorMacro(Principal2,double,3);    

  // Description:
  // Set the angle of the cone. This is the angle between the axis of the cone
  // and a generatrix. Warning: this is not the aperture! The aperture is
  // twice this angle.
  // As a side effect, the angle plus height sets the base radius of the cone.
  // Angle is expressed in degrees.
  void SetAngle (double angle);
  double GetAngle ();

  // Description:
  // Turn on/off whether to cap the base of the cone with a polygon.
  vtkSetMacro(Capping,int);
  vtkGetMacro(Capping,int);
  vtkBooleanMacro(Capping,int);

protected:
  vtkConeSource2(int res=6);
  ~vtkConeSource2() {}

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  double Height;
  double Radius;
  int Resolution;
  int Capping;
  double Center[3];
  double Direction[3];

  double Principal1[3];
  double Principal2[3];
  
private:
  vtkConeSource2(const vtkConeSource2&);  // Not implemented.
  void operator=(const vtkConeSource2&);  // Not implemented.
};

#endif


