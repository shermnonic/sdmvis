#include "VectorToVertexNormalFilter.h"

#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include "vtkDataObject.h"
#include "vtkSmartPointer.h"
#include "vtkPointData.h"
#include "vtkDataSetAttributes.h"
#include "vtkObject.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"

#include <limits>
#include "VectorUtils.h"
 
vtkStandardNewMacro(VectorToVertexNormalFilter);
 
VectorToVertexNormalFilter::VectorToVertexNormalFilter()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(2);
  
}
 
VectorToVertexNormalFilter::~VectorToVertexNormalFilter()
{
 
}

#include <vtkDataSetAttributes.h>
int VectorToVertexNormalFilter::RequestData(vtkInformation *vtkNotUsed(request),
                                             vtkInformationVector **inputVector,
                                             vtkInformationVector *outputVector)
{
	outputVector->SetNumberOfInformationObjects(2);
	// get the info objects
	vtkInformation *vectorInInfo  = inputVector[0]->GetInformationObject(0);
	vtkInformation *meshInInfo    = inputVector[1]->GetInformationObject(0);
	vtkInformation *vectorOutInfo = outputVector->GetInformationObject(0);

	// get the input and ouptut for vectorfield
	vtkPolyData *vectorInput = vtkPolyData::SafeDownCast(
	  vectorInInfo->Get(vtkDataObject::DATA_OBJECT()));

	vtkPolyData *vectorOutput = vtkPolyData::SafeDownCast(
	  vectorOutInfo->Get(vtkDataObject::DATA_OBJECT()));

	// get the input and ouptut for mesh
	vtkPolyData *meshInput = vtkPolyData::SafeDownCast(
	  meshInInfo->Get(vtkDataObject::DATA_OBJECT()));

	double min_ = std::numeric_limits<double>::max();
	double max_ = -min_;

	VectorUtils::TangentialPerpendicularDecomposition decomp;

	for( int iA=0; iA < vectorInput->GetNumberOfPoints(); iA++ )
	{
		double * vector = vectorInput->GetPointData()->GetVectors()->GetTuple3(iA);
		double * normal = meshInput  ->GetPointData()->GetNormals()->GetTuple3(iA);
#if 1
		// Split a vector into its perpendicular and tangential component
		decomp.compute( vector, normal );
		
		// Keep book of min/max magnitude for color table 
		max_ = (decomp.magnitude > max_) ? decomp.magnitude : max_;
		min_ = (decomp.magnitude < min_) ? decomp.magnitude : min_;

		// Set vectorial data to tangential part
		vectorInput->GetPointData()->GetVectors()->SetTuple3( iA,
			decomp.v_tan[0], decomp.v_tan[1], decomp.v_tan[2] );
#else
		// Projection of vector onto surface normal
		double dotprod = vector[0]*normal[0] + vector[1]*normal[1] + vector[2]*normal[2];

		// Vector perpendicular to surface
		double v_perp[3];
		v_perp[0] = dotprod*normal[0];
		v_perp[1] = dotprod*normal[1];
		v_perp[2] = dotprod*normal[2];

		// Vector tangential to surface
		double v_tan[3];
		v_tan[0] = vector[0] - v_perp[0];
		v_tan[1] = vector[1] - v_perp[1];
		v_tan[2] = vector[2] - v_perp[2];

		// Magnitude of perpendicular part
		double mag = sqrt(v_perp[0]*v_perp[0] + v_perp[1]*v_perp[1] + v_perp[2]*v_perp[2]);
		
			// BUG: Formula previously used by Vitalis was:
			//	sqrt(v_orth[0]+v_orth[0]*v_orth[1]+v_orth[1]*v_orth[2]+v_orth[2]);
		// Keep book of min/max magnitude for color table 
		max = (mag > max) ? mag : max;
		min = (mag < min) ? mag : min;

		// Set vectorial data to tangential part
		vectorInput->GetPointData()->GetVectors()->SetTuple3(iA,v_tan[0],v_tan[1],v_tan[2]);
#endif
	}
	m_max = max_;
	m_min = min_;
	vectorOutput->ShallowCopy(vectorInput);
	return 1;
}
 
 
//----------------------------------------------------------------------------
void VectorToVertexNormalFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

