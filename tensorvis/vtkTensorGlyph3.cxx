/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTensorGlyph3.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTensorGlyph3.h"

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkTensorGlyph3);

vtkTensorGlyph3::vtkTensorGlyph3()
{
	VectorDisplacement = 0; 
	VectorDisplacementFactor = 1.0;

	EigenvalueOffset = 0;
	EigenvalueOffsetValue = 0.1;

	EigenvalueSqrtScaling = 0;

	this->SetInputArrayToProcess(2, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS,
                            vtkDataSetAttributes::VECTORS);
}

//----------------------------------------------------------------------------
vtkTensorGlyph3::~vtkTensorGlyph3()
{
}

//----------------------------------------------------------------------------
// Don't rely on the implementation of this->Superclass::RequestData as that
// has, at point of writing, some known issues. See issues 1 and 2 at
// http://public.kitware.com/Bug/view.php?id=12179
int vtkTensorGlyph3::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *source = vtkPolyData::SafeDownCast(
    sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataArray *inTensors;
  double tensor[9];
  vtkDataArray *inScalars;
  vtkIdType numPts, numSourcePts, numSourceCells, inPtId, i;
  int j;
  vtkPoints *sourcePts;
  vtkDataArray *sourceNormals;
  vtkCellArray *sourceCells, *cells;
  vtkPoints *newPts;
  vtkFloatArray *newScalars=NULL;
  vtkFloatArray *newNormals=NULL;
  double x[3], s;
  vtkTransform *trans;
  vtkCell *cell;
  vtkIdList *cellPts;
  int npts;
  vtkIdType *pts;
  vtkIdType ptIncr, cellId;
  vtkIdType subIncr;
  int numDirs, dir, eigen_dir, symmetric_dir;
  vtkMatrix4x4 *matrix;
  double *m[3], w[3], *v[3];
  double m0[3], m1[3], m2[3];
  double v0[3], v1[3], v2[3];
  double xv[3], yv[3], zv[3];
  double maxScale;
  vtkPointData *pd, *outPD;

  numDirs = (this->ThreeGlyphs?3:1)*(this->Symmetric+1);

  pts = new vtkIdType[source->GetMaxCellSize()];
  trans = vtkTransform::New();
  matrix = vtkMatrix4x4::New();

  // set up working matrices
  m[0] = m0; m[1] = m1; m[2] = m2;
  v[0] = v0; v[1] = v1; v[2] = v2;

  vtkDebugMacro(<<"Generating tensor glyphs");

  pd = input->GetPointData();
  outPD = output->GetPointData();
  inTensors = this->GetInputArrayToProcess(0, inputVector);
  inScalars = this->GetInputArrayToProcess(1, inputVector);
  vtkDataArray *inVectors;
  inVectors = this->GetInputArrayToProcess(2, inputVector);
  numPts = input->GetNumberOfPoints();

  if ( !inTensors || numPts < 1 )
    {
    vtkErrorMacro(<<"No data to glyph!");
    return 1;
    }

  //[MH-08-2013]---ADDON-BEGIN----

  //
  // Special case of superquadric tensor glyphs
  //
  // We assume that the topology does not change. The particular superquadric
  // is adjusted later according to the eigenvalues computed in the main loop.
  bool doSuperquadrics = false;
  vtkSuperquadricSource* sqSource = NULL;
  if( m_superquadricSource.GetPointer() )
  {
	  doSuperquadrics = true;

	  sqSource = vtkSuperquadricSource::New();
	  sqSource->SetPhiResolution  ( m_superquadricSource->GetPhiResolution() );
	  sqSource->SetThetaResolution( m_superquadricSource->GetThetaResolution() );
  }
  //[MH-08-2013]---ADDON-END----

  //
  // Allocate storage for output PolyData
  //  
  sourcePts = source->GetPoints();
  numSourcePts = sourcePts->GetNumberOfPoints();
  numSourceCells = source->GetNumberOfCells();

  newPts = vtkPoints::New();
  newPts->Allocate(numDirs*numPts*numSourcePts);

  // Setting up for calls to PolyData::InsertNextCell()
  if ( (sourceCells=source->GetVerts())->GetNumberOfCells() > 0 )
    {
    cells = vtkCellArray::New();
    cells->Allocate(numDirs*numPts*sourceCells->GetSize());
    output->SetVerts(cells);
    cells->Delete();
    }
  if ( (sourceCells=this->GetSource()->GetLines())->GetNumberOfCells() > 0 )
    {
    cells = vtkCellArray::New();
    cells->Allocate(numDirs*numPts*sourceCells->GetSize());
    output->SetLines(cells);
    cells->Delete();
    }
  if ( (sourceCells=this->GetSource()->GetPolys())->GetNumberOfCells() > 0 )
    {
    cells = vtkCellArray::New();
    cells->Allocate(numDirs*numPts*sourceCells->GetSize());
    output->SetPolys(cells);
    cells->Delete();
    }
  if ( (sourceCells=this->GetSource()->GetStrips())->GetNumberOfCells() > 0 )
    {
    cells = vtkCellArray::New();
    cells->Allocate(numDirs*numPts*sourceCells->GetSize());
    output->SetStrips(cells);
    cells->Delete();
    }

  // only copy scalar data through
  pd = this->GetSource()->GetPointData();
  // generate scalars if eigenvalues are chosen or if scalars exist.
  if (this->ColorGlyphs &&
      ((this->ColorMode == COLOR_BY_EIGENVALUES) ||
	   (this->ColorMode == COLOR_BY_FRACTIONAL_ANISOTROPY) ||
       (inScalars && (this->ColorMode == COLOR_BY_SCALARS)) ) )
    {
    newScalars = vtkFloatArray::New();
    newScalars->Allocate(numDirs*numPts*numSourcePts);
    if (this->ColorMode == COLOR_BY_EIGENVALUES)
      {
      newScalars->SetName("MaxEigenvalue");
      }
    else if (this->ColorMode == COLOR_BY_FRACTIONAL_ANISOTROPY)
      {
      newScalars->SetName("FractionalAnisotropy");
      }
    else
      {
      newScalars->SetName(inScalars->GetName());
      }
    }
  else
    {
    outPD->CopyAllOff();
    outPD->CopyScalarsOn();
    outPD->CopyAllocate(pd,numDirs*numPts*numSourcePts);
    }
  if ( (sourceNormals = pd->GetNormals()) )
    {
    newNormals = vtkFloatArray::New();
    newNormals->SetNumberOfComponents(3);
    newNormals->SetName("Normals");
    newNormals->Allocate(numDirs*3*numPts*numSourcePts);
    }
  //
  // First copy all topology (transformation independent)
  //
  for (inPtId=0; inPtId < numPts; inPtId++)
    {
    ptIncr = numDirs * inPtId * numSourcePts;
    for (cellId=0; cellId < numSourceCells; cellId++)
      {
      cell = this->GetSource()->GetCell(cellId);
      cellPts = cell->GetPointIds();
      npts = cellPts->GetNumberOfIds();
      for (dir=0; dir < numDirs; dir++)
        {
        // This variable may be removed, but that
        // will not improve readability
        subIncr = ptIncr + dir*numSourcePts;
        for (i=0; i < npts; i++)
          {
          pts[i] = cellPts->GetId(i) + subIncr;
          }
        output->InsertNextCell(cell->GetCellType(),npts,pts);
        }
      }
    }
  //
  // Traverse all Input points, transforming glyph at Source points
  //
  trans->PreMultiply();

  for (inPtId=0; inPtId < numPts; inPtId++)
    {
    ptIncr = numDirs * inPtId * numSourcePts;

    // Translation is postponed

    inTensors->GetTuple(inPtId, tensor);

    // compute orientation vectors and scale factors from tensor
    if ( this->ExtractEigenvalues ) // extract appropriate eigenfunctions
      {
      for (j=0; j<3; j++)
        {
        for (i=0; i<3; i++)
          {
          m[i][j] = tensor[i+3*j];
          }
        }
      vtkMath::Jacobi(m, w, v);

      //copy eigenvectors
      xv[0] = v[0][0]; xv[1] = v[1][0]; xv[2] = v[2][0];
      yv[0] = v[0][1]; yv[1] = v[1][1]; yv[2] = v[2][1];
      zv[0] = v[0][2]; zv[1] = v[1][2]; zv[2] = v[2][2];
      }
    else //use tensor columns as eigenvectors
      {
      for (i=0; i<3; i++)
        {
        xv[i] = tensor[i];
        yv[i] = tensor[i+3];
        zv[i] = tensor[i+6];
        }
      w[0] = vtkMath::Normalize(xv);
      w[1] = vtkMath::Normalize(yv);
      w[2] = vtkMath::Normalize(zv);
      }

//[MH-08-2013]---ADDON-BEGIN----
	// Do the superquadric glyps!
	if( doSuperquadrics )
	{
	  // Compute barycentric coordinates from eigenvalues c_(linear/planar/sperical)
      double cl =    (w[0]-w[1]) / (w[0]+w[1]+w[2]),
		     cp = 2.*(w[1]-w[2]) / (w[0]+w[1]+w[2]);

	  // Modulate superquadric parameters according to cl and cp
	  double alpha, beta;
	  if( cl >= cp )
	  {
		  // Linear case
		  alpha = pow(1. - cp, SuperquadricGamma);
		  beta  = pow(1. - cl, SuperquadricGamma);
		  // TODO: Use parameterization along x
	  }
	  else
	  {
		  // Planar or spherical case
		  alpha = pow(1. - cl, SuperquadricGamma);
		  beta  = pow(1. - cp, SuperquadricGamma);
		  // TODO: Use parameterization along x
	  }

	  // Re-compute superquadric PolyData
      sqSource->SetPhiRoundness  ( alpha );
      sqSource->SetThetaRoundness( beta );
	  sqSource->Update();

	  // Exchange source points
	  if( sourcePts->GetNumberOfPoints() == sqSource->GetOutput()->GetPoints()->GetNumberOfPoints() )
	  {
		sourcePts->DeepCopy( sqSource->GetOutput()->GetPoints() );		

		if( sqSource->GetOutput()->GetPointData()->GetNormals() )
		{
		  sourceNormals->DeepCopy( sqSource->GetOutput()->GetPointData()->GetNormals() );
		}
		else
		  std::cout << "Warning: Normals mismatch in superquadric glyph!\n";
	  }
	  else
	  {
		std::cout << "Warning: Mismatch in superquadric glyph point count!\n";
	  }
	  //sourcePts = m_superquadricSource->GetOutput()->GetPoints();
		// sourcePts = source->GetPoints();
	}
//[MH-08-2013]---ADDON-END----

//[MH-03-2013]---ADDON-BEGIN----
	// Preserve original scale factors for the case that we want to write
	// them out to scalar data, e.g. for color-coding this leads to a correctly
	// scaled lookup table without the arbitrary scaling factor (which should
	// only increase visibility of the glyphs in a particular visualization)
	double w_original[3];
	w_original[0] = w[0];
	w_original[1] = w[1];
	w_original[2] = w[2];
//[MH-03-2013]---ADDON-END----

    // compute scale factors
    w[0] *= this->ScaleFactor;
    w[1] *= this->ScaleFactor;
    w[2] *= this->ScaleFactor;

//[MH-08-2013]---ADDON-BEGIN----
	if( EigenvalueOffset )
	{
	  w[0] += EigenvalueOffsetValue;
	  w[1] += EigenvalueOffsetValue;
	  w[2] += EigenvalueOffsetValue;
	}
	if( EigenvalueSqrtScaling )
	{
		// Scale eigenvalues for geometry
		w[0] = sqrt(w[0]);
		w[1] = sqrt(w[1]);
		w[2] = sqrt(w[2]);

		// Also scale eigenvalues for color coding
		w_original[0] = sqrt(w_original[0]);
		w_original[1] = sqrt(w_original[1]);
		w_original[2] = sqrt(w_original[2]);
	}
//[MH-08-2013]---ADDON-END----


    if ( this->ClampScaling )
      {
      for (maxScale=0.0, i=0; i<3; i++)
        {
        if ( maxScale < fabs(w[i]) )
          {
          maxScale = fabs(w[i]);
          }
        }
      if ( maxScale > this->MaxScaleFactor )
        {
        maxScale = this->MaxScaleFactor / maxScale;
        for (i=0; i<3; i++)
          {
          w[i] *= maxScale; //preserve overall shape of glyph
          }
        }
      }

    // normalization is postponed

    // make sure scale is okay (non-zero) and scale data
    for (maxScale=0.0, i=0; i<3; i++)
      {
      if ( w[i] > maxScale )
        {
        maxScale = w[i];
        }
      }
    if ( maxScale == 0.0 )
      {
      maxScale = 1.0;
      }
    for (i=0; i<3; i++)
      {
      if ( w[i] == 0.0 )
        {
        w[i] = maxScale * 1.0e-06;
        }
      }

    // Now do the real work for each "direction"

    for (dir=0; dir < numDirs; dir++)
      {
      eigen_dir = dir%(this->ThreeGlyphs?3:1);
      symmetric_dir = dir/(this->ThreeGlyphs?3:1);

      // Remove previous scales ...
      trans->Identity();

	  ///*==*
	  //double mu[3];
	  //pd->GetVectors()->GetTuple( inPtId, mu );

      // translate Source to Input point
      input->GetPoint(inPtId, x);
      trans->Translate(x[0], x[1], x[2]);

//[MH-11-2012]---ADDON-BEGIN----
	  // translate by additional vector data given
	  if( VectorDisplacement && inVectors  )
	  {
		double* tv = inVectors->GetTuple3(inPtId);
		double tvscale = VectorDisplacementFactor; //1e7;
		trans->Translate( tvscale*tv[0], tvscale*tv[1], tvscale*tv[2] );
	  }
//[MH-11-2012]---ADDON-END----

      // normalized eigenvectors rotate object for eigen direction 0
      matrix->Element[0][0] = xv[0];
      matrix->Element[0][1] = yv[0];
      matrix->Element[0][2] = zv[0];
      matrix->Element[1][0] = xv[1];
      matrix->Element[1][1] = yv[1];
      matrix->Element[1][2] = zv[1];
      matrix->Element[2][0] = xv[2];
      matrix->Element[2][1] = yv[2];
      matrix->Element[2][2] = zv[2];
      trans->Concatenate(matrix);

      if (eigen_dir == 1)
        {
        trans->RotateZ(90.0);
        }

      if (eigen_dir == 2)
        {
        trans->RotateY(-90.0);
        }

      if (this->ThreeGlyphs)
        {
        trans->Scale(w[eigen_dir], this->ScaleFactor, this->ScaleFactor);
        }
      else
        {
        trans->Scale(w[0], w[1], w[2]);
        }

      // Mirror second set to the symmetric position
      if (symmetric_dir == 1)
        {
        trans->Scale(-1.,1.,1.);
        }

      // if the eigenvalue is negative, shift to reverse direction.
      // The && is there to ensure that we do not change the
      // old behaviour of vtkTensorGlyphs (which only used one dir),
      // in case there is an oriented glyph, e.g. an arrow.
      if (w[eigen_dir] < 0 && numDirs > 1)
        {
        trans->Translate(-this->Length, 0., 0.);
        }

      // multiply points (and normals if available) by resulting
      // matrix
      trans->TransformPoints(sourcePts,newPts);

      // Apply the transformation to a series of points,
      // and append the results to outPts.
      if ( newNormals )
        {
        // a negative determinant means the transform turns the
        // glyph surface inside out, and its surface normals all
        // point inward. The following scale corrects the surface
        // normals to point outward.
        if (trans->GetMatrix()->Determinant() < 0)
          {
          trans->Scale(-1.0,-1.0,-1.0);
          }
        trans->TransformNormals(sourceNormals,newNormals);
        }

        // Copy point data from source
      if ( this->ColorGlyphs && inScalars &&
           (this->ColorMode == COLOR_BY_SCALARS) )
        {
//[MH-03-2013]---CHANGE-BEGIN----
		// Remove user scale factor
        s = inScalars->GetComponent(inPtId, 0) / this->ScaleFactor;
		// was:
		// s = inScalars->GetComponent(inPtId, 0);
//[MH-03-2013]---CHANGE-END------
        for (i=0; i < numSourcePts; i++)
          {
          newScalars->InsertTuple(ptIncr+i, &s);
          }
        }
      else if (this->ColorGlyphs &&
               (this->ColorMode == COLOR_BY_EIGENVALUES) )
        {
        // If ThreeGlyphs is false we use the first (largest)
        // eigenvalue as scalar.
//[MH-03-2013]---CHANGE-BEGIN----
		// Remove user scale factor
		 s = w_original[eigen_dir];
		// was:
		// s = w[eigen_dir];
//[MH-03-2013]---CHANGE-END------
        for (i=0; i < numSourcePts; i++)
          {
          newScalars->InsertTuple(ptIncr+i, &s);
          }
        }
//[MH-07-2013]---CHANGE-BEGIN----
      else if (this->ColorGlyphs &&
               (this->ColorMode == COLOR_BY_FRACTIONAL_ANISOTROPY) )
        {

			double mu = (w_original[0]+w_original[1]+w_original[2]) / 3.;
			double FA = sqrt(3./2.) * 
				sqrt( (w_original[0]-mu)*(w_original[0]-mu) +
					  (w_original[1]-mu)*(w_original[1]-mu) +
					  (w_original[2]-mu)*(w_original[2]-mu)	) 
				/
				sqrt( w_original[0]*w_original[0] +
				      w_original[1]*w_original[1] +
				      w_original[2]*w_original[2] );

			s = FA;

        for (i=0; i < numSourcePts; i++)
          {
          newScalars->InsertTuple(ptIncr+i, &s);
          }
        }
//[MH-07-2013]---CHANGE-END------
      else
        {
        for (i=0; i < numSourcePts; i++)
          {
          outPD->CopyData(pd,i,ptIncr+i);
          }
        }
      ptIncr += numSourcePts;
      }
    }
  vtkDebugMacro(<<"Generated " << numPts <<" tensor glyphs");
  //
  // Update output and release memory
  //
  delete [] pts;

  if( sqSource ) sqSource->Delete();

  output->SetPoints(newPts);
  newPts->Delete();

  if ( newScalars )
    {
    int idx = outPD->AddArray(newScalars);
    outPD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    newScalars->Delete();
    }

  if ( newNormals )
    {
    outPD->SetNormals(newNormals);
    newNormals->Delete();
    }

  output->Squeeze();
  trans->Delete();
  matrix->Delete();

  return 1;
}

//----------------------------------------------------------------------------
// Don't rely on the implementation of this->Superclass::RequestUpdateExtent
// as that has, at point of writing, a known issue. See issue 3 at
// http://public.kitware.com/Bug/view.php?id=12179
int vtkTensorGlyph3::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  if (sourceInfo)
    {
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
                    0);
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
                    1);
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
                    0);
    }

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
              outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
              outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
              outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  return 1;
}

//----------------------------------------------------------------------------
void vtkTensorGlyph3::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
