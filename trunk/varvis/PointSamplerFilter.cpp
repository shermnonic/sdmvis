#include "PointSamplerFilter.h"

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
#include "vtkDataArray.h"
#include "vtkFieldData.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkScalarsToColors.h"
#include "vtkColorTransferFunction.h"
#include "vtkCellArray.h"
#include "vtkVertex.h"
#include "vtkCellData.h"
vtkStandardNewMacro(PointSamplerFilter);
 
PointSamplerFilter::PointSamplerFilter()
{
	this->SetNumberOfInputPorts(1);
	this->SetNumberOfOutputPorts(1);
}
 
PointSamplerFilter::~PointSamplerFilter()
{
}

int PointSamplerFilter::RequestData(vtkInformation *vtkNotUsed(request),
                                             vtkInformationVector **inputVector,
                                             vtkInformationVector *outputVector)
{

	// get the info objects
	vtkInformation *infoMesh  = inputVector[0]->GetInformationObject(0);
	vtkInformation *infoPoints = outputVector->GetInformationObject(0);

	// get the input and ouptut 
	vtkPolyData *meshInput = vtkPolyData::SafeDownCast(
	  infoMesh->Get(vtkDataObject::DATA_OBJECT()));

	vtkPolyData *pointsOutput= vtkPolyData::SafeDownCast(
	  infoPoints->Get(vtkDataObject::DATA_OBJECT()));

  
	std::vector<double> areaVector;
  	vtkSmartPointer<vtkPoints>	    thePoints  = vtkSmartPointer<vtkPoints     >::New();   
	vtkSmartPointer<vtkCellArray>   someVertex = vtkSmartPointer<vtkCellArray  >::New();
	vtkSmartPointer<vtkVertex>      vertex     = vtkSmartPointer<vtkVertex     >::New();
	vtkSmartPointer<vtkDoubleArray> theNormals = vtkSmartPointer<vtkDoubleArray>::New();   
	theNormals->SetNumberOfComponents(3);

	// generate lookup table for cell numbers
	for (int iA=0;iA<meshInput->GetNumberOfCells();iA++)
    {
		double pointX[3];
		double pointY[3]; 
		double pointZ[3]; 
		meshInput->GetCell(iA)->GetPoints()->GetPoint(0,pointX);
		meshInput->GetCell(iA)->GetPoints()->GetPoint(1,pointY);
		meshInput->GetCell(iA)->GetPoints()->GetPoint(2,pointZ);

		double vectorA[3];
		double vectorB[3];

		vectorA[0]=pointY[0]-pointX[0];
		vectorA[1]=pointY[1]-pointX[1];
		vectorA[2]=pointY[2]-pointX[2];

		vectorB[0]=pointZ[0]-pointX[0];
		vectorB[1]=pointZ[1]-pointX[1];
		vectorB[2]=pointZ[2]-pointX[2];

		// calculate the determinate of this two 
		//first crossproduct
		double AxB[3];
		AxB[0]= vectorA[1]*vectorB[2]-vectorA[2]-vectorB[1];
		AxB[1]= vectorA[2]*vectorB[0]-vectorA[0]-vectorB[2];
		AxB[2]= vectorA[0]*vectorB[1]-vectorA[1]-vectorB[0];

		// make a norm of this one 
		double norm=AxB[0]*AxB[0]+AxB[1]*AxB[1]+AxB[2]*AxB[2];
		norm = sqrt(norm);
		
		// we have a area , wahoo 
		//put this in a vector for look up 
		double area= 0.5*norm;
		if (areaVector.empty())
			areaVector.push_back(area);
		else
		{	// sum the area for index 
			areaVector.push_back(area+areaVector.at(iA-1));
		}

	}
	// generate Sampling Points 
	for (int iA=0;iA<numberOfSamplingPoints;iA++)
    {
		// find the triangle we wanna work with
		double totalArea= areaVector.at(areaVector.size()-1);   // last element of this one is total area
		double aRandNum= (double)rand()/RAND_MAX ;				// a value between 0 and 1
		double randomArea= totalArea*aRandNum;
		
		// now we have a areaValue and we need to find the index in the area Vector 
		bool done=false;
		int left=0;
		int right=areaVector.size();
		int mid=0.5*(left+right);
		while(!done)
		{
			if(left==mid)
				done=true;
			if (areaVector.at(mid)<randomArea)
			{
				left=mid;
				mid=0.5*(left+right);
			}
			else
			{
				right=mid;
				mid=0.5*(left+right);
			}
		}

		int index=left;

		// get the points for Barycentric coordinate system
		double pointX[3];
		double pointY[3]; 
		double pointZ[3]; 
		meshInput->GetCell(index)->GetPoints()->GetPoint(0,pointX);
		meshInput->GetCell(index)->GetPoints()->GetPoint(1,pointY);
		meshInput->GetCell(index)->GetPoints()->GetPoint(2,pointZ);
		
		if (useMeshNormals)
		{
			// get the normal vector for this point Ids
			// FIND OUT; is the normal for all points in the 
			// Trinangel the same ? >> looks like it 
			// thats why we just read one point and 
			// save its normal data 
			int pIDx=meshInput->GetCell(index)->GetPointId(0);
			double *normalX = meshInput->GetPointData()->GetNormals()->GetTuple3(pIDx);
			double normX[3];

			for (int iB=0;iB<3;iB++)
				normX[iB]=normalX[iB];
			
			theNormals->InsertTuple3(iA,normX[0],normX[1],normX[2]);
		}
		// generate a random point on the triangele surface
		double x,y,z;
	
		double rand1=(double)rand()/RAND_MAX;
		double rand2=(double)rand()/RAND_MAX;
		double rand3=(double)rand()/RAND_MAX;

		double lambda1= rand1/(rand1+rand2+rand3);
		double lambda2= rand2/(rand1+rand2+rand3);
		double lambda3= rand3/(rand1+rand2+rand3);

		double sum = lambda1+lambda2+lambda3; // just a check value; sum should be 1 

		x= lambda1*pointX[0]+lambda2*pointY[0]+lambda3*pointZ[0];
		y= lambda1*pointX[1]+lambda2*pointY[1]+lambda3*pointZ[1];
		z= lambda1*pointX[2]+lambda2*pointY[2]+lambda3*pointZ[2];

		double randomPoint[3] ;
		randomPoint[0]=x;
		randomPoint[1]=y;
		randomPoint[2]=z;

		// insert random point to polydata Points and create a vertex so we can see the point
		vtkIdType pid[1]; 
		pid[0] = thePoints->InsertNextPoint (randomPoint); 
		someVertex->InsertNextCell ( 1,pid );

	}
	// generate OutPut PolyData 
	pointsOutput->SetPoints(thePoints);  // set the points
	pointsOutput->SetVerts(someVertex);  // set the vertex
	if (useMeshNormals)
		pointsOutput->GetPointData()->SetNormals(theNormals);
    return 1;
}
 
//----------------------------------------------------------------------------
void PointSamplerFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}



