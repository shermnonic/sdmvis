#include "TensorVis.h"
#include <vtkRenderer.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkSphereSource.h>
#include <vtkCylinderSource.h>
#include <vtkCubeSource.h>
#include <vtkSuperquadricSource.h>
#include <vtkArrowSource.h>
#include <vtkLookupTable.h>
#include <vtkTextProperty.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdlib>  // rand(), srand(), RAND_MAX
#include <time.h>   // time()

//-----------------------------------------------------------------------------
//  C'tor
//-----------------------------------------------------------------------------
TensorVis::TensorVis()
	: m_samplePoints (VTKPTR<vtkPoints>        ::New()),
	  m_sampleTensors(VTKPTR<vtkDoubleArray>   ::New()),
	  m_sampleVectors(VTKPTR<vtkDoubleArray>   ::New())
	  // FIXME: Initialization required? The smart pointers are resetted in 
	  //        updateGlyphs() either way!
{
}

//-----------------------------------------------------------------------------
//  setup()
//-----------------------------------------------------------------------------
bool TensorVis::setup( const char* filename, vtkImageData* refvol )
{
	using namespace std;

	// Sanity check
	if( !refvol )
	{
		cerr << "Error: TensorVis::setup() requires valid vtkImageData instance!\n";
		return false;
	}

	// Assume tensor data has same dimension as reference volume
	int dims[3];
	refvol->GetDimensions( dims );

	// Load tensor data
	if( !m_tensors.load(filename,dims) )
	{
		cerr << "Failed to load tensor data!" << endl;
		return false;
	}

	// Reference volume is furtheron required for sampling the tensor field e.g.
	// based on an intensity value threshold.
	m_refVol = refvol;

	// Perform a random sampling initially because it is robust and fast
	updateGlyphs( GridSampling );

	return true;
}

//-----------------------------------------------------------------------------
//  insertGlyph()
//-----------------------------------------------------------------------------
bool TensorVis::insertGlyph( int id, int x, int y, int z )
{
	// Get associated tensor
	float tensor[9];
	m_tensors.getTensor( x,y,z, tensor );

	// Get associated vector
	float v[3];
	m_tensors.getVector( x,y,z, v );

	// Compute physical point location of voxel
	double point[3];
	int ijk[3]; ijk[0]=x; ijk[1]=y; ijk[2]=z;
	m_refVol->GetPoint( m_refVol->ComputePointId( ijk ), point );

	// Threshold on scalar value of reference volume
	double intensity = m_refVol->GetScalarComponentAsDouble(x,y,z,0);
	if( intensity < getThreshold() )
	{
		return false;
	}

	// Set point, tensor and vector data
	m_samplePoints ->InsertPoint ( id, point );
	m_sampleTensors->InsertTuple9( id, tensor[0],tensor[1],tensor[2],
		                               tensor[3],tensor[4],tensor[5],
		                               tensor[6],tensor[7],tensor[8] );
	m_sampleVectors->InsertTuple3( id, v[0], v[1], v[2] );
	return true;
}

//-----------------------------------------------------------------------------
//  gridSampling()
//-----------------------------------------------------------------------------
int TensorVis::gridSampling()
{
	// Assume tensor data and reference volume have exact same dimensionality
	int dims[3];
	m_tensors.getImageDataSpace().getDimensions( dims );
	
	int id = 0;
	for( int x=0; x < dims[0]; x+=getGridStepSize() )
		for( int y=0; y < dims[1]; y+=getGridStepSize() )
			for( int z=0; z < dims[2]; z+=getGridStepSize() )
			{
				if( insertGlyph( id, x, y, z ) )
					id++;
			}

	return id;
}

//-----------------------------------------------------------------------------
//  randomSampling()
//-----------------------------------------------------------------------------

/// Return floating point random value between 0.0 and 1.0
float frand()
{
	return (float)(rand()%RAND_MAX)/(float)RAND_MAX;
}
/// Reseed random generator for frand()
void frand_seed( unsigned int seed=-1 )
{
	if( seed==-1 )
		seed = time(NULL);
	srand(seed);
}

int TensorVis::randomSampling()
{
	frand_seed();

	// Assume tensor data and reference volume have exact same dimensionality
	int dims[3];
	m_tensors.getImageDataSpace().getDimensions( dims );

	int numSamples = getSampleSize();

	const size_t maxTrials = 100*numSamples;
	size_t trials=0;
	int i=0;
	for( ; i < numSamples && trials < maxTrials; ++i, ++trials )
	{
		// Random voxel index
		int x = (int)(frand()*(dims[0]-1));
		int y = (int)(frand()*(dims[1]-1));
		int z = (int)(frand()*(dims[2]-1));

		if( !insertGlyph( i, x, y, z ) )
		{
			--i;
		}
	}
	return i;
}

//-----------------------------------------------------------------------------
//  updateGlyphs()
//-----------------------------------------------------------------------------
void TensorVis::updateGlyphs( int /*SamplingStrategy*/ strategy )
{	
	// Clear old sampling data (WORKAROUND using ::New() operator)
	m_samplePoints  = VTKPTR<vtkPoints>     ::New();
	m_sampleTensors = VTKPTR<vtkDoubleArray>::New();
	m_sampleVectors = VTKPTR<vtkDoubleArray>::New();
	m_samplePoints ->SetNumberOfPoints( getSampleSize() );
	m_sampleTensors->SetNumberOfTuples( getSampleSize() );
	m_sampleTensors->SetNumberOfComponents( 9 );
	m_sampleVectors->SetNumberOfComponents( 3 );

	m_sampleTensors->SetName("DisplacementCovariance");
	m_sampleVectors->SetName("AverageDisplacement");

	// Sample glyphs
	int n=0;
	switch( strategy )
	{
	default:
		std::cout << "Warning: Unknown TensorVis sampling strategy!\n";
	case GridSampling  : n=gridSampling();   break;
	case RandomSampling: n=randomSampling(); break;
	}
	std::cout << "Inserted " << n << " tensor glyphs.\n";

	// Clear any remaining points
	m_samplePoints ->SetNumberOfPoints( n );
	m_sampleTensors->SetNumberOfTuples( n );
	m_sampleVectors->SetNumberOfTuples( n );

	// Update tensor glyph visualization
	updateGlyphData( m_samplePoints, m_sampleTensors, m_sampleVectors );	
}
