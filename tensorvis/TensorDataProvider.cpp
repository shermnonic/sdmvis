#include "TensorDataProvider.h"
#include "TensorData.h"
#include <vtkImageData.h>
#include <cassert>
#include <cstdlib>  // rand(), srand(), RAND_MAX
#include <ctime>    // time()
#include <iostream>
#include <limits>


// Anonymous namespace for helper functions to avoid naming conflicts
namespace {
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
}; // End of anonymous namespace

//-----------------------------------------------------------------------------
// C'tor
//-----------------------------------------------------------------------------
TensorDataProvider::TensorDataProvider()
  : m_numPoints(0),
    m_thresholdImage(NULL),
	m_lastSampling(NoSampling),
	m_sliceDir(0),
	m_slice(0)
{
	m_lastGridStep[0] = m_lastGridStep[1] = m_lastGridStep[2] = -1;
}

//-----------------------------------------------------------------------------
void TensorDataProvider::
  setImageDataSpace( ImageDataSpace space )
{
	m_space = space;
}

//-----------------------------------------------------------------------------
//	updateTensorData()
//-----------------------------------------------------------------------------
void TensorDataProvider::
  updateTensorData()
{
	unsigned numPoints = getNumValidPoints();
	
	reserveTensorData( numPoints );

	//std::cout << "Updating tensor data at " << numPoints << " points\n";

	float  tensor[9];
	float  vector[3];
	double point [3];
	for( unsigned id=0; id < numPoints; id++ )
	{		
		if( (numPoints > 5000) && (id%(numPoints/200) == 0) )
			std::cout << "Updating tensor data " << (int)((id/(float)(numPoints-1))*100) << "% \r";

		m_samplePoints->GetPoint( id, point );
		
		getTensor( point[0],point[1],point[2], tensor );
		getVector( point[0],point[1],point[2], vector );

		m_sampleTensors->SetTuple9( id, tensor[0],tensor[1],tensor[2],
									    tensor[3],tensor[4],tensor[5],
										tensor[6],tensor[7],tensor[8] );
		m_sampleVectors->SetTuple3( id, vector[0],vector[1],vector[2] );
	}
	
	//std::cout << "Updating tensor data completed. \n";
}

//-----------------------------------------------------------------------------
void TensorDataProvider::
  reserveTensorData( unsigned numPoints )
{	
	// We always allocate tensor *and* vector data. Although this produces
	// a slight memory overhead when no vector data are given it simplifies
	// the rest of the implementation because we can assume that vector data
	// is always present.
	m_sampleTensors = VTKPTR<vtkDoubleArray>::New();
	m_sampleVectors = VTKPTR<vtkDoubleArray>::New();
	m_sampleTensors->SetNumberOfComponents( 9 );
	m_sampleVectors->SetNumberOfComponents( 3 );
	m_sampleTensors->SetNumberOfTuples( numPoints );
	m_sampleVectors->SetNumberOfTuples( numPoints );
	
	m_sampleTensors->SetName("DisplacementCovariance");
	m_sampleVectors->SetName("AverageDisplacement");	
}

//-----------------------------------------------------------------------------
void TensorDataProvider::
  reservePoints( unsigned maxNumPoints )
{
	m_samplePoints = VTKPTR<vtkPoints>::New();
	m_samplePoints->SetNumberOfPoints( maxNumPoints );
}

//-----------------------------------------------------------------------------
bool TensorDataProvider::
  insertPoint( int id, int i, int j, int k )
{
	double point[3];
	m_space.getPoint( i,j,k, point );
	return insertPoint( id, point[0],point[1],point[2] );
}

//-----------------------------------------------------------------------------
bool TensorDataProvider::
  insertPoint( int id, double x, double y, double z )
{
	if( isValidSamplePoint( x,y,z ) )
	{
		//m_samplePoints->InsertPoint( id, x,y,z );
		m_samplePoints->SetPoint( id, x,y,z ); // no range checking
		return true;
	}	
	
	// Sample point was rejected
	return false;
}

//-----------------------------------------------------------------------------
//  generateGridPoints()
//-----------------------------------------------------------------------------
void TensorDataProvider::
  generateGridPoints( int gridStepSizeX, int gridStepSizeY, int gridStepSizeZ )
{	
	assert( gridStepSizeX > 0 );
	if( gridStepSizeY <= 0 ) gridStepSizeY = gridStepSizeX;
	if( gridStepSizeZ <= 0 ) gridStepSizeZ = gridStepSizeX;
	
	// Get active (sub)region
	int dim_max[3], dim_min[3];
	m_subregion.get_range( m_space, dim_min, dim_max );

	//// Reserve maximum number of points
	//unsigned maxNumPoints = (dims[0]/gridStepSizeX) * 
	//                        (dims[1]/gridStepSizeY) * 
	//                        (dims[2]/gridStepSizeZ);	
	//reservePoints( maxNumPoints );

	// Count number of valid points
	int numPoints = 0;
	double point[3];
	for( int x=dim_min[0]; x < dim_max[0]; x+=gridStepSizeX )
		for( int y=dim_min[1]; y < dim_max[1]; y+=gridStepSizeY )
			for( int z=dim_min[2]; z < dim_max[2]; z+=gridStepSizeZ )
			{
				m_space.getPoint( x,y,z, point );
				if( isValidSamplePoint( point[0],point[1],point[2] ) )
					numPoints++;
			}

	reservePoints( numPoints );

	// Generate sample points
	int id = 0;
	for( int x=dim_min[0]; x < dim_max[0]; x+=gridStepSizeX )
		for( int y=dim_min[1]; y < dim_max[1]; y+=gridStepSizeY )
			for( int z=dim_min[2]; z < dim_max[2]; z+=gridStepSizeZ )
			{
				if( insertPoint( id, x, y, z ) )
					id++;
			}

	// WORKAROUND
	//m_samplePoints->SetNumberOfPoints( id );
	if( numPoints != id )
		std::cout << "Warning: Incosistent number of sample points!\n";
			
	// Set number of generated sample points
	m_numPoints = id;
	m_lastSampling = GridSampling;
	m_lastGridStep[0] = gridStepSizeX;
	m_lastGridStep[1] = gridStepSizeY;
	m_lastGridStep[2] = gridStepSizeZ;
}

//-----------------------------------------------------------------------------
//  generateRandomPoints()
//-----------------------------------------------------------------------------
void TensorDataProvider::
  generateRandomPoints( int numSamples )
{
	assert( numSamples >= 0 );

	// Reserve points
	reservePoints( numSamples );
	
	frand_seed();

	// Get active (sub)region
	int dim_max[3], dim_min[3];
	m_subregion.get_range( m_space, dim_min, dim_max );
	int dims[3];
	dims[0] = dim_max[0] - dim_min[0];
	dims[1] = dim_max[1] - dim_min[1];
	dims[2] = dim_max[2] - dim_min[2];
	
	// Do on average at most 100 trials per sample
	const size_t maxTrials = 100*numSamples;
	unsigned trials=0;
	int id=0;
	for( ; id < numSamples && trials < maxTrials; trials++ )
	{
		// Random voxel index
		int x = (int)(frand()*(dims[0]-1)) + dim_min[0];
		int y = (int)(frand()*(dims[1]-1)) + dim_min[1];
		int z = (int)(frand()*(dims[2]-1)) + dim_min[2];

		if( insertPoint( id, x, y, z ) )
			id++;
	}

	// WORKAROUND: Explicitly shrink points if not all samples are valid
	if( id < numSamples )
		m_samplePoints->SetNumberOfPoints( id );
	
	// Set number of generated sample points
	m_numPoints = id;
	m_lastSampling = RandomSampling;
}

//-----------------------------------------------------------------------------
//	setPoints()
//-----------------------------------------------------------------------------
void TensorDataProvider::
  setPoints( vtkPoints* pts, bool threshold )
{
	if( !pts ) return;
	unsigned numPoints = pts->GetNumberOfPoints();
	reservePoints( numPoints );

	if( !threshold )
	{
		m_samplePoints->DeepCopy( pts );
		m_numPoints = numPoints;
	}
	else
	{
		unsigned id=0;
		for( unsigned i=0; i < numPoints; i++ )
		{
			double pt[3];
			pts->GetPoint( i, pt );

			if( insertPoint( id, pt[0],pt[1],pt[2] ) )
				id++;
		}

		if( id < numPoints )
			m_samplePoints->SetNumberOfPoints( id );

		m_numPoints = id;
	}
}

//-----------------------------------------------------------------------------
//	setImageMask()
//-----------------------------------------------------------------------------
void TensorDataProvider::
  setImageMask( vtkImageData* img )
{
	int dims[3];
	double spacing[3];
	double origin [3];
	img->GetDimensions( dims );
	img->GetSpacing   ( spacing );
	img->GetOrigin    ( origin );
	m_thresholdSpace = ImageDataSpace( dims, spacing, origin );
	m_thresholdImage = img;
}

//-----------------------------------------------------------------------------
//	setImageThreshold()
//-----------------------------------------------------------------------------
void TensorDataProvider::
  setImageThreshold( double threshLow, double threshHigh )
{
	m_thresholdLow   = threshLow;
	m_thresholdHigh  = threshHigh;
}

//-----------------------------------------------------------------------------
//	isValidSamplePoint()
//-----------------------------------------------------------------------------
bool TensorDataProvider::
  isValidSamplePoint( double x, double y, double z )
{
#if 1
	// Image threshold
	if( m_thresholdImage )
	{
		int ijk[3];
		m_thresholdSpace.getIJK( x,y,z, ijk );
		double intensity 
			= m_thresholdImage->GetScalarComponentAsDouble(ijk[0],ijk[1],ijk[2],0);
		if( (intensity < m_thresholdLow) ) //|| (intensity > m_thresholdHigh) )
			return false;
	}
#endif
#if 0
	// Check for close-to-zero matrix
	float myeps = 20.f*std::numeric_limits<float>::min();
	float T[9];
	getTensor(x,y,z,T);
	if( fabs(T[0])+fabs(T[1])+fabs(T[2])+fabs(T[3])+fabs(T[4])+
		fabs(T[5])+fabs(T[6])+fabs(T[7])+fabs(T[8]) < myeps )
		return false;
#endif
	return true;
}

//-----------------------------------------------------------------------------
//	writeTensorData()
//-----------------------------------------------------------------------------

ImageDataSpace resample( const ImageDataSpace& space, int stepX, int stepY, int stepZ )
{
	int dims[3];
	space.getDimensions( dims );
	
	// Image extent after resampling with given stepsize
	dims[0] /= stepX;
	dims[1] /= stepY;
	dims[2] /= stepZ;
	
	int extent[6] = { 0, 0, 0, 0, 0, 0 };
	extent[1] = dims[0] - 1;
	extent[3] = dims[1] - 1;
	extent[5] = dims[2] - 1;
	
	// Calculate new image spacing
	double spacing[3];
	space.getSpacing( spacing );
	spacing[0] *= (double)stepX;
	spacing[1] *= (double)stepY;
	spacing[2] *= (double)stepZ;

	// Origin left unchanged for now
	double origin[3];
	space.getOrigin( origin );

	// Update our image space calculation
	return ImageDataSpace( dims, spacing, origin );
}

bool TensorDataProvider::
  writeTensorData( const char* filename )
{
	using namespace std;

	ImageDataSpace space = 	getImageDataSpace();
	int dims[3];
	space.getDimensions( dims );

#if 0
	// Enforce grid sampling with stepsize 1
	if( m_lastGridStep[0] != 1 || 
		m_lastGridStep[1] != 1 || 
		m_lastGridStep[2] != 1 ||
		m_lastSampling != GridSampling )
	{
		cout << "Warning: Tensor data not sampled on image space grid, "
			    "thus resampling is required!" << endl;
		generateGridPoints( 1 );
	}
#else
	// Re-use last grid sampling
	if( m_lastSampling != GridSampling )
	{
		cout << "Warning: Tensor data not sampled on image space grid!" << endl;
		return false;
	}
	// WORKAROUND: Adapt image space according to grid sampling.	
	space = resample( space, m_lastGridStep[0], m_lastGridStep[1], m_lastGridStep[2] );
#endif

	cout << "Allocating buffer for tensor data" << endl;
	TensorData tdata;
	tdata.allocateData( space );

	cout << "Copying tensor data" << endl;
	bool hasVectors = (getVectors() != NULL);
	int numPoints = getPoints()->GetNumberOfPoints();
	double defaultVector[3] = {0.,0.,0.}; // Default vector when no vector data present
	for( vtkIdType id=0; id < numPoints; id++ )
	{
		// Progress message
		if( (numPoints > 900) && (id%(numPoints/300) == 0) )
			std::cout << "Preparing tensor data " << (int)((id/(float)(numPoints-1))*100) << "% \r";

		double* tensor = getTensors()->GetTuple9( id );
		double* vector = defaultVector;
		if( hasVectors ) vector = getVectors()->GetTuple3( id );		

	  #if 0
		// We know linearization and write at a sample resolution of 1
		tdata.setTensor( id, tensor );
		tdata.setVector( id, vector );

	  #else
		// Explicitly compute ijk voxel position from physical position
		static double pt[3];
		getPoints() ->GetPoint ( id, pt );

		int ijk[3];
		space.getIJK( pt, ijk );

		tdata.setTensor( ijk[0],ijk[1],ijk[2], tensor );
		tdata.setVector( ijk[0],ijk[1],ijk[2], vector );
	  #endif
	}
	cout << "Preparing tensor data finished" << endl;

	cout << "Writing tensor data to " << filename << endl;
	tdata.save( filename );
	return true;
}

//-----------------------------------------------------------------------------
//	get / setSlicing()
//-----------------------------------------------------------------------------
void TensorDataProvider::
  setSlicing( bool enable )
{
	m_subregion.active = true;
}

//-----------------------------------------------------------------------------
bool TensorDataProvider::
  getSlicing() const
{
	return m_subregion.active;
}

//-----------------------------------------------------------------------------
//	setSliceDirection()
//-----------------------------------------------------------------------------
void TensorDataProvider::
  setSliceDirection( int dim )
{
	setSlice( 0 );
}

//-----------------------------------------------------------------------------
//	getNumSlices()
//-----------------------------------------------------------------------------
int TensorDataProvider::
  getNumSlices( int dim )
{
	// Default to current slice axis
	if( dim<0 || dim>2 ) dim = m_sliceDir;

	// Return slice axis size
	int dims[3];
	m_space.getDimensions( dims );
	return dims[ dim ];
}

//-----------------------------------------------------------------------------
//	setSlice()
//-----------------------------------------------------------------------------
void TensorDataProvider::
  setSlice( int slice )
{
	// Get number of slices (same as getNumSlices())
	int dims[3];
	m_space.getDimensions( dims );	
	int numSlices = dims[ m_sliceDir ];

	// Clamp to valid axis values
	if( slice < 0 ) slice = 0; else
	if( slice >= numSlices ) slice = numSlices-1;

	// Set subregion to slice
	m_subregion.reset( m_space );
	m_subregion.min_[ m_sliceDir ] = slice;
	m_subregion.max_[ m_sliceDir ] = slice+1;

	m_slice = slice;
}
