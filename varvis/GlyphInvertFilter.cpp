#include "GlyphInvertFilter.h"

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
#include "vtkDoubleArray.h"
#include "vtkPolyData.h"
#include <vtkDataSetAttributes.h>
#include "vtkCellArray.h"

vtkStandardNewMacro(GlyphInvertFilter);
 
GlyphInvertFilter::GlyphInvertFilter()
{
	this->SetNumberOfInputPorts(1);
	this->SetNumberOfOutputPorts(1);
}
 
GlyphInvertFilter::~GlyphInvertFilter()
{
}

int GlyphInvertFilter::RequestData(vtkInformation *vtkNotUsed(request),
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

	
	vtkSmartPointer<vtkPoints>      endPoints  = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkDoubleArray> endVectors = vtkSmartPointer<vtkDoubleArray>::New();
	endVectors->SetNumberOfComponents(3);
	vtkSmartPointer<vtkCellArray>	endVertex  = vtkSmartPointer<vtkCellArray>::New();	
	
	// get each point -> set his location to p+v
	//				  -> set vector to - v

	for (int iA=0;iA<meshInput->GetNumberOfPoints();iA++)
	{
		double * vector = meshInput->GetPointData()->GetVectors()->GetTuple3(iA);
		double * point  = meshInput->GetPoint(iA);

		double endpoint[3];
		double normVector[3];
		double vectorLength=vector[0]*vector[0]+
							vector[1]*vector[1]+
							vector[2]*vector[2];

		vectorLength=sqrt(vectorLength);

		normVector[0]=vector[0] / vectorLength;
		normVector[1]=vector[1] / vectorLength;
		normVector[2]=vector[2] / vectorLength;

		endpoint[0]=point[0]+normVector[0];
		endpoint[1]=point[1]+normVector[1];
		endpoint[2]=point[2]+normVector[2];
		
		double endvector[3];
			endvector[0]=-vector[0];
			endvector[1]=-vector[1];
			endvector[2]=-vector[2];
			
		vtkIdType startPid[1];
		startPid[0]= endPoints->InsertNextPoint ( endpoint[0], endpoint[1], endpoint[2] );
		endVertex->InsertNextCell ( 1,startPid);
		endVectors->InsertNextTuple3(endvector[0],endvector[1],endvector[2]);



	}
	meshOutput->ShallowCopy(meshInput);
	meshOutput->SetPoints(endPoints);
	meshOutput->SetVerts(endVertex);
	
	meshOutput->GetPointData()->SetVectors(endVectors);
	return 1;
}
 
 
//----------------------------------------------------------------------------
void GlyphInvertFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}



