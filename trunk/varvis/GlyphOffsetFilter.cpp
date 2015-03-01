#include "GlyphOffsetFilter.h"

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
#include "vtkPoints.h" 
#include <vtkDataSetAttributes.h>

vtkStandardNewMacro(GlyphOffsetFilter);
 
GlyphOffsetFilter::GlyphOffsetFilter()
{
	this->SetNumberOfInputPorts(1);
	this->SetNumberOfOutputPorts(1);
}
 
GlyphOffsetFilter::~GlyphOffsetFilter()
{
}

int GlyphOffsetFilter::RequestData(vtkInformation *vtkNotUsed(request),
                                             vtkInformationVector **inputVector,
                                             vtkInformationVector *outputVector)
{
	outputVector->SetNumberOfInformationObjects(1);
	// get the info objects
	vtkInformation *meshInfo  = inputVector[0]->GetInformationObject(0);
	vtkInformation *meshOutInfo = outputVector->GetInformationObject(0);

	// get the input and ouptut for vectorfield
	vtkPolyData *meshInput = vtkPolyData::SafeDownCast(
	meshInfo->Get(vtkDataObject::DATA_OBJECT()));

	vtkPolyData *meshOutput = vtkPolyData::SafeDownCast(
	meshOutInfo->Get(vtkDataObject::DATA_OBJECT()));


	// okay, we need the mesh normals for orientation
	// then the vector data it self 
	// and we generate the offset on Normal data
	// save it and make output 

	
	vtkSmartPointer<vtkPoints> offsetPoints = vtkSmartPointer<vtkPoints>::New();
	for (int iA=0;iA<meshInput->GetNumberOfPoints();iA++)
	{
		double * normal = meshInput->GetPointData()->GetNormals()->GetTuple3(iA);

		// put it all together 
		double v_para[3];
		meshInput->GetPoint(iA,v_para);
		v_para[0]=v_para[0]+m_delta*normal[0];
		v_para[1]=v_para[1]+m_delta*normal[1];
		v_para[2]=v_para[2]+m_delta*normal[2];
		offsetPoints->InsertPoint(iA,v_para[0],v_para[1],v_para[2]);
	}
	meshOutput->ShallowCopy(meshInput);
	meshOutput->SetPoints(offsetPoints);
	return 1;
}
 
 
//----------------------------------------------------------------------------
void GlyphOffsetFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}



