#include "VectorToMeshColorFilter.h"

#include <limits>

#include <vtkObjectFactory.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkInformationVector.h>
#include <vtkInformation.h>
#include <vtkDataObject.h>
#include <vtkSmartPointer.h>
#include <vtkPointData.h>
#include <vtkDataSetAttributes.h>
#include <vtkObject.h>
#include <vtkDataSet.h>
#include <vtkFieldData.h>
#include <vtkDataSetAttributes.h>
#include <vtkDoubleArray.h>
#include <vtkScalarsToColors.h>
#include <vtkColorTransferFunction.h>
#include <vtkLookupTable.h>

#include "VectorUtils.h"

vtkStandardNewMacro(VectorToMeshColorFilter);
 
//----------------------------------------------------------------------------
VectorToMeshColorFilter::VectorToMeshColorFilter()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(2); 
}
 
//----------------------------------------------------------------------------
VectorToMeshColorFilter::~VectorToMeshColorFilter()
{ 
}

//----------------------------------------------------------------------------
void VectorToMeshColorFilter
	::SetLookupTable( vtkSmartPointer<vtkScalarsToColors> lut )
{
	m_lookupTable = lut;
}

//----------------------------------------------------------------------------
vtkScalarsToColors* VectorToMeshColorFilter::GetLookupTable()
{
	// If no LUT exists, create a default one
	if( !m_lookupTable.GetPointer() )
	{
		vtkScalarsToColors* lut = NULL;
	#if 1
		// Rainbow like
		vtkSmartPointer<vtkLookupTable> llut;
		llut = vtkSmartPointer<vtkLookupTable>::New();
		llut->SetHueRange( 1./6., 1. );
		llut->SetSaturationRange( 1, 1 );
		llut->SetValueRange( 1, 1 );
		llut->Build();
		//llut->SetTableRange( mmin, mmax );
		lut = llut.GetPointer();
	#else
		// Blue-to-green
		vtkSmartPointer<vtkColorTransferFunction> hlut;
		hlut = vtkSmartPointer<vtkColorTransferFunction>::New();
		hlut->AddRGBPoint( mmax, 0,1,0 );
		hlut->AddRGBPoint( mmax/2, 0.72,1,0.28 );
		hlut->AddRGBPoint( 0.1, 1,1,1 ); 
		hlut->AddRGBPoint( -0.1, 1,1,1 ); 
		hlut->AddRGBPoint( mmin/2, 0.28,0.87,1);
		hlut->AddRGBPoint( mmin, 0,0,1 );	
		hlut->HSVWrapOff();
		hlut->SetColorSpaceToRGB();
		lut = hlut.GetPointer();
	#endif

		m_lookupTable = lut;		
	}

	return m_lookupTable;
}

//----------------------------------------------------------------------------
int VectorToMeshColorFilter::RequestData(vtkInformation *vtkNotUsed(request),
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


	// Temporary buffer for computed orthogonal displacement values
	vtkSmartPointer<vtkDoubleArray> orthogonalDisplacement = 
	                                    vtkSmartPointer<vtkDoubleArray>::New();
	orthogonalDisplacement->SetNumberOfComponents(1);
	orthogonalDisplacement->SetNumberOfTuples(vectorInput->GetNumberOfPoints());

	double min_ = std::numeric_limits<double>::max();
	double max_ = -min_;

	VectorUtils::TangentialPerpendicularDecomposition decomp;
   
	for (int iA=0;iA<vectorInput->GetNumberOfPoints();iA++)
	{
		// Vector from vectorfield at p
		double* vector = vectorInput->GetPointData()->GetVectors()->GetTuple3(iA);
		// Surface normal at p
		double* normal = meshInput  ->GetPointData()->GetNormals()->GetTuple3(iA);

#if 1
		// Split a vector into its perpendicular and tangential component
		decomp.compute( vector, normal );

		// Does v_perp point inside or outside from surface?
		double sign = (decomp.dotProduct >= 0.) ? 1. : -1.;

		// Signed length
		double displacement = sign * decomp.magnitude;

		// Keep book of min/max displacement for color table 
		max_ = (displacement > max_) ? displacement : max_;
		min_ = (displacement < min_) ? displacement : min_;

		// Set scalar data to signed length
		orthogonalDisplacement->InsertTuple1( iA, displacement );
#else
		// Project vector onto normal to yield component orthogonal to surface
		double dotprod=vector[0]*normal[0]+vector[1]*normal[1]+vector[2]*normal[2];
		double v_orth[3];
		v_orth[0]=dotprod*normal[0];
		v_orth[1]=dotprod*normal[1];
		v_orth[2]=dotprod*normal[2];

		// Unsigned length
		double length = sqrt( v_orth[0]*v_orth[0] + v_orth[1]*v_orth[1] + v_orth[2]*v_orth[2] );

		// Does v_orth point inside or outside from surface?
		double sign = (dotprod >= 0.) ? 1. : -1.;

		// Signed length
		double displacement = sign * length;
		orthogonalDisplacement->InsertTuple1( iA, displacement);
	 
		// Find min_ and max_ value for lookup table
		if( displacement > max_ ) max_ = displacement;
		if( displacement < min_ ) min_ = displacement;
#endif
	}

	// Store min/max
	m_min = min_;
	m_max = max_;
	
	// Set lookup table
	vtkScalarsToColors* lut = GetLookupTable();
	//lut->SetRange( min_, max_ );
	//lut->Build();

	// Apply to "meshColors" attribute
	double range = max_;
	for (int iA=0;iA<vectorInput->GetNumberOfPoints();iA++)
	{
		double value = orthogonalDisplacement->GetTuple1(iA);
		double*color = lut->GetColor( value );
		double r,g,b;
		r=color[0]*255;
		g=color[1]*255;
		b=color[2]*255;

		meshInput->GetPointData()->GetArray("meshColors")->SetTuple3(iA,r,g,b);
	}
  
	vectorOutput->ShallowCopy(meshInput);
	return 1;
}
 
//----------------------------------------------------------------------------
void VectorToMeshColorFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
