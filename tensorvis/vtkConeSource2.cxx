/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConeSource2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkConeSource2.h"

#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPolyData.h"
#include "vtkTransform.h"
#include "vtkCellArray.h"

#include <math.h>

vtkStandardNewMacro(vtkConeSource2);

//----------------------------------------------------------------------------
// Construct with default resolution 6, height 1.0, radius 0.5, capping on
// and principal axes (0,1,0) and (0,0,1).
vtkConeSource2::vtkConeSource2(int res)
{
  res = (res < 0 ? 0 : res);
  this->Resolution = res;
  this->Height = 1.0;
  this->Radius = 0.5;
  this->Capping = 1;

  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;
  
  this->Direction[0] = 1.0;
  this->Direction[1] = 0.0;
  this->Direction[2] = 0.0;
	
  this->Principal1[0] = 0.0;
  this->Principal1[1] = 1.0;
  this->Principal1[2] = 0.0;

  this->Principal2[0] = 0.0;
  this->Principal2[1] = 0.0;
  this->Principal2[2] = 1.0;
	
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
int vtkConeSource2::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  double angle;
  int numLines, numPolys, numPts;
  double x[3], xbot;
  int i;
  vtkIdType pts[VTK_CELL_SIZE];
  vtkPoints *newPoints; 
  vtkCellArray *newLines=0;
  vtkCellArray *newPolys=0;
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  // for streaming
  int piece;
  int numPieces;
  int maxPieces;
  int start, end;
  int createBottom;
  
  piece = output->GetUpdatePiece();
  if (piece >= this->Resolution && !(piece == 0 && this->Resolution == 0))
    {
    return 1;
    }
  numPieces = output->GetUpdateNumberOfPieces();
	
  // Minimum resolution is 4
  maxPieces = this->Resolution >= 4 ? this->Resolution : 4;
  if (numPieces > maxPieces)
    {
    numPieces = maxPieces;
    }
  if (piece >= maxPieces)
    {
    // Super class should do this for us, 
    // but I put this condition in any way.
    return 1;
    }
  start = maxPieces * piece / numPieces;
  end = (maxPieces * (piece+1) / numPieces) - 1;
  createBottom = (this->Capping && (start == 0));
  
  vtkDebugMacro("ConeSource Executing");
  
  if ( this->Resolution )
    {
    angle = 2.0*3.141592654/this->Resolution;
    }
  else
    {
    angle = 0.0;
    }

  // Set things up; allocate memory
  //
  switch ( this->Resolution )
  {
  // Case 0,1,2 are no longer valid in vtkConeSource2
  default:
    if (createBottom)
      {
      // piece 0 has cap.
      numPts = this->Resolution + 1;
      numPolys = end - start + 2;
      }
    else
      {
      numPts = end - start + 3;
      numPolys = end - start + 2;
      }
    newPolys = vtkCellArray::New();
    newPolys->Allocate(newPolys->EstimateSize(numPolys,this->Resolution));
    break;
  }
  newPoints = vtkPoints::New();
  newPoints->SetDataTypeToFloat(); //used later during transformation
  newPoints->Allocate(numPts);

  
  // Create cone
  // In contrast to original vtkConeSource we directly create the absolute 
  // coordinates considering center and direction.
  //
  // Shorthand for basis vectors of local coordinate system on cap  
  double v1[3], v2[3];
  v1[0] = Principal1[0];  v1[1] = Principal1[1];  v1[2] = Principal1[2];
  v2[0] = Principal2[0];  v2[1] = Principal2[1];  v2[2] = Principal2[2];
  
  // Shorthand term for tip vertex of cone
  double mu[3];
  mu[0] = this->Center[0];
  mu[1] = this->Center[1];
  mu[2] = this->Center[2];  
  
  x[0] = mu[0]; //was: this->Height / 2.0; // zero-centered
  x[1] = mu[1]; //was: 0.0;
  x[2] = mu[2]; //was: 0.0;
  pts[0] = newPoints->InsertNextPoint(x);  

  double dir[3];
  dir[0] = this->Height * this->Direction[0];
  dir[1] = this->Height * this->Direction[1];
  dir[2] = this->Height * this->Direction[2];

  //~ xbot = -this->Height / 2.0;

#define CREATE_CIRCLE_POINT( x, theta, v1, v2, mu )  \
   	    // point on unit circle                          \
		double cost = this->Radius*cos(theta),           \
		       sint = this->Radius*sin(theta);           \
		// circle in v1,2 plane translated by mu         \
		x[0] = cost * v1[0]	 +  sint * v2[0]  +  mu[0];  \
		x[1] = cost * v1[1]	 +  sint * v2[1]  +  mu[1];  \
		x[2] = cost * v1[2]	 +  sint * v2[2]  +  mu[2];	

  switch (this->Resolution) 
  {
  // Case 0,1,2 are no longer valid in vtkConeSource2
  default: // General case: create Resolution triangles and single cap
    // create the bottom.
    if ( createBottom )
      {
      for (i=0; i < this->Resolution; i++) 
        {
		//CREATE_CIRCLE_POINT( x, i*angle, v1,v2, mu )

   	    // point on unit circle
		double cost = this->Radius*cos(i*angle),
		       sint = this->Radius*sin(i*angle);
		// circle in v1,2 plane translated by mu
		x[0] = cost * v1[0]	 +  sint * v2[0]  +  mu[0] + dir[0];
		x[1] = cost * v1[1]	 +  sint * v2[1]  +  mu[1] + dir[1];
		x[2] = cost * v1[2]	 +  sint * v2[2]  +  mu[2] + dir[2];	

		//was:
        //~ x[0] = xbot;
        //~ x[1] = this->Radius * cos (i*angle);
        //~ x[2] = this->Radius * sin (i*angle);
			
        // Reverse the order
        pts[this->Resolution - i - 1] = newPoints->InsertNextPoint(x);
        }
      newPolys->InsertNextCell(this->Resolution,pts);
      }
    
    pts[0] = 0;
    if ( ! createBottom)
      {
      // we need to create the points also
		CREATE_CIRCLE_POINT( x, start*angle, v1,v2, mu )
      //~ x[0] = xbot;
      //~ x[1] = this->Radius * cos (start*angle);
      //~ x[2] = this->Radius * sin (start*angle);
      pts[1] = newPoints->InsertNextPoint(x);
		  
      for (i = start; i <= end; ++i)
        {
		CREATE_CIRCLE_POINT( x, (i+1)*angle, v1,v2, mu )
        //~ x[1] = this->Radius * cos ((i+1)*angle);
        //~ x[2] = this->Radius * sin ((i+1)*angle);
        pts[2] = newPoints->InsertNextPoint(x);
        newPolys->InsertNextCell(3,pts);
        pts[1] = pts[2];
        }
      }
    else
      {
      // bottom and points have already been created.
      for (i=start; i <= end; i++) 
        {
        pts[1] = i+1;
        pts[2] = i+2;
        if (pts[2] > this->Resolution)
          {
          pts[2] = 1;
          }
        newPolys->InsertNextCell(3,pts);
        }
      } // createBottom
    
  } //switch

  // Transformation is no longer required, since we directly compute the
  // absolute coordinates.
#if 0 
  // A non-default origin and/or direction requires transformation
  //
  if ( this->Center[0] != 0.0 || this->Center[1] != 0.0 || 
       this->Center[2] != 0.0 || this->Direction[0] != 1.0 || 
       this->Direction[1] != 0.0 || this->Direction[2] != 0.0 )
    {
    vtkTransform *t = vtkTransform::New();
    t->Translate(this->Center[0], this->Center[1], this->Center[2]);
    double vMag = vtkMath::Norm(this->Direction);
    if ( this->Direction[0] < 0.0 )
      {
      // flip x -> -x to avoid instability
      t->RotateWXYZ(180.0, (this->Direction[0]-vMag)/2.0,
                    this->Direction[1]/2.0, this->Direction[2]/2.0);
      t->RotateWXYZ(180.0, 0, 1, 0);
      }
    else
      {
      t->RotateWXYZ(180.0, (this->Direction[0]+vMag)/2.0,
                    this->Direction[1]/2.0, this->Direction[2]/2.0);
      }
    float *ipts=
      static_cast<vtkFloatArray *>(newPoints->GetData())->GetPointer(0);
    for (i=0; i<numPts; i++, ipts+=3)
      {
      t->TransformPoint(ipts,ipts);
      }
    
    t->Delete();
    }
#endif

  // Update ourselves
  //
  output->SetPoints(newPoints);
  newPoints->Delete();

  if ( newPolys )
    {
    newPolys->Squeeze(); // we may have estimated size; reclaim some space
    output->SetPolys(newPolys);
    newPolys->Delete();
    }
  else
    {
    output->SetLines(newLines);
    newLines->Delete();
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkConeSource2::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
               -1);
  return 1;
}


//----------------------------------------------------------------------------
void vtkConeSource2::SetAngle(double angle)
{
  this->SetRadius( this->Height * tan( vtkMath::RadiansFromDegrees( angle ) ) );
}

//----------------------------------------------------------------------------
double vtkConeSource2::GetAngle()
{
  return  vtkMath::DegreesFromRadians( atan2( this->Radius, this->Height ) );
}

//----------------------------------------------------------------------------
void vtkConeSource2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Height: " << this->Height << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
  os << indent << "Center: (" << this->Center[0] << ", " 
     << this->Center[1] << ", " << this->Center[2] << ")\n";
  os << indent << "Direction: (" << this->Direction[0] << ", " 
     << this->Direction[1] << ", " << this->Direction[2] << ")\n";
}
