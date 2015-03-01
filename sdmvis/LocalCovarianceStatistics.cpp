#include "LocalCovarianceStatistics.h"
#include "SDMTensorDataProvider.h"
#include "TensorVisBase.h"

#include <vtkMetaImageWriter.h>
#include <vtkMetaImageReader.h>
#ifndef VTKPTR
#define VTKPTR vtkSmartPointer
#endif

#include <iostream>
#include <string>
#include <limits>
#include <cstring> // memset
#include <cassert>

// Compile number of moment measures (alternative to .GetNumMomentMeasures())
#define NUMMOMENTMEASURES \
	(TensorDataStatistics::NumMeasures * TensorDataStatistics::NumMoments)


//============================================================================
//	StatisticsIntegrator
//============================================================================
void StatisticsIntegrator
  ::setTensorData( SDMTensorDataProvider* tdata )
{
	tstat().setTensorData( tdata );
	m_tdata = tdata;
}

void StatisticsIntegrator
  ::setTensorDataStatistics( /*const*/ TensorDataStatistics& tstat )
{
	m_tstat = tstat;
}

const TensorDataStatistics& StatisticsIntegrator
  ::getTensorDataStatistics() const 
{
	return m_tstat; 
}

//============================================================================
//	StatisticsTensorPassThru
//============================================================================
void StatisticsTensorPassThru
  ::integrate_pre()
{
	std::cout << "StatisticsTensorPassThru::integrate_pre()\n";
}

void StatisticsTensorPassThru
  ::integrate_inner_loop( TensorDataProvider* tdata, int ijk[3] )
{
	static float  tensorCacheFloat[9];
	static double tensorCache[9];

	// Just pass thru tensor at ijk
	double pt[3];
	m_space.getPoint( ijk, pt );
	tdata->getTensor( pt[0], pt[1], pt[2], tensorCacheFloat );
	
	for( int i=0; i < 9; i++ )
		tensorCache[i] = tensorCacheFloat[i];
	
	m_passThruField.setTensor( ijk[0], ijk[1], ijk[2], tensorCache );
}

void StatisticsTensorPassThru
  ::integrate_post()
{
	std::cout << "StatisticsTensorPassThru::integrate_post()\n";	
}

void StatisticsTensorPassThru
  ::setup( ImageDataSpace& space )
{
	m_passThruField.allocateData( space );
	m_space = space;
}

void StatisticsTensorPassThru
  ::save( const char* basepath ) const
{
	using namespace std;
	string 
		filename_mxvol = basepath + string("tensor-passthru.mxvol"),
		filename_tf    = basepath + string("tensor-passthru.tensorfield");
	m_passThruField.save( filename_mxvol.c_str() );
	m_passThruField.save( filename_tf   .c_str() );
}

bool StatisticsTensorPassThru
  ::load( const char* basepath ) 
{
	using namespace std;
	
	string filename = basepath + string("tensor-passthru.mxvol");
	return m_passThruField.load( filename.c_str() );
}



//============================================================================
//	StatisticsTensorIntegrator
//============================================================================
void StatisticsTensorIntegrator
  ::integrate_pre()
{
	std::cout << "StatisticsTensorIntegrator::integrate_pre()\n";
}

void StatisticsTensorIntegrator
  ::integrate_inner_loop( TensorDataProvider* tdata, int ijk[3] )
{
	static double tensorCache[9];

	// Update moments statistics
	tstat().setTensorData( tdata );
	tstat().computeAverageTensor();
	
	tstat().getAverageTensor( tensorCache );
	m_integralField.setTensor( ijk[0], ijk[1], ijk[2], tensorCache );
}

void StatisticsTensorIntegrator
  ::integrate_post()
{
	std::cout << "StatisticsTensorIntegrator::integrate_post()\n";	
}

void StatisticsTensorIntegrator
  ::setup( ImageDataSpace& space )
{
	m_integralField.allocateData( space );
	m_space = space;
}

void StatisticsTensorIntegrator
  ::save( const char* basepath ) const
{
	using namespace std;
	string 
		filename_mxvol = basepath + string("tensor-integral.mxvol"),
		filename_tf    = basepath + string("tensor-integral.tensorfield");
	m_integralField.save( filename_mxvol.c_str() );
	m_integralField.save( filename_tf   .c_str() );
}

bool StatisticsTensorIntegrator
  ::load( const char* basepath ) 
{
	using namespace std;
	
	string filename = basepath + string("tensor-integral.mxvol");
	return m_integralField.load( filename.c_str() );
}



//============================================================================
//	StatisticsMomentImagesIntegrator
//============================================================================
StatisticsMomentImagesIntegrator
  ::StatisticsMomentImagesIntegrator()
    : m_min( NUMMOMENTMEASURES, 0. ),
	  m_max( NUMMOMENTMEASURES, 0. )
{
	// Create image instances (does *not* allocate image memory)
	m_momentImages.resize( NUMMOMENTMEASURES );
	for( unsigned i=0; i < m_momentImages.size(); i++ )
	{
		m_momentImages[i] = vtkSmartPointer<vtkImageData>::New();
	}
}

void StatisticsMomentImagesIntegrator
  ::integrate_pre()
{
	// Clear images
	ImageCollection::iterator it = m_momentImages.begin();
	for( ; it != m_momentImages.end(); it++ )
	{
		vtkImageData* img = *it;
		unsigned n = img->GetNumberOfPoints();
		memset( img->GetScalarPointer(), 0, sizeof(float)*n );
	}	

	// Reset min/max moment values
	m_min = std::vector<double>( tstat().getNumMomentMeasures(), 
				+std::numeric_limits<double>::max() ),
	m_max = std::vector<double>( tstat().getNumMomentMeasures(), 
				-std::numeric_limits<double>::max() );	
}

void StatisticsMomentImagesIntegrator
  ::integrate_post()
{
}

void StatisticsMomentImagesIntegrator
  ::integrate_inner_loop( TensorDataProvider* tdata, int ijk[3] )
{
	// Update moments statistics
	tstat().setTensorData( tdata );
	tstat().compute();
	
	// Write moments into scalar images
	std::vector<double> moments = tstat().getMomentMeasures();
	assert( moments.size() == m_momentImages.size() );
	for( unsigned i=0; i < moments.size(); i++ )
	{
		vtkImageData* img = m_momentImages[i];
		float* pixel = (float*)img->GetScalarPointer( ijk );
		
		// Variance mode, no adjustment
		*pixel = (float)moments[i];

		// Update min/max
		m_min[i] = (moments[i] < m_min[i]) ? moments[i] : m_min[i];
		m_max[i] = (moments[i] > m_max[i]) ? moments[i] : m_max[i];
	}	
}

void StatisticsMomentImagesIntegrator
  ::setup( ImageDataSpace& space )
{
	// Get image properties
	int    dims[3];
	double spacing[3];
	double origin[3];
	space.getDimensions( dims );
	space.getSpacing( spacing );
	space.getOrigin( origin );

	// Compute extent on-the-fly
	int extent[6] = { 0, 0, 0, 0, 0, 0 };
	extent[1] = dims[0] - 1;
	extent[3] = dims[1] - 1;
	extent[5] = dims[2] - 1;

	// Allocate image memory
	ImageCollection::iterator it = m_momentImages.begin();
	for( ; it != m_momentImages.end(); it++ )	
	{
		vtkImageData* img = it->GetPointer(); // == (*it)

		img->Initialize();

		img->SetExtent ( extent );
		img->SetSpacing( spacing );
		img->SetOrigin( origin );

		img->SetScalarTypeToFloat(); // ( VTK_FLOAT );
		img->SetNumberOfScalarComponents( 1 );
		img->AllocateScalars();	
		
		img->Update();
	}
}

void StatisticsMomentImagesIntegrator
  ::save( const char* basepath ) const
{
	using namespace std;

	VTKPTR<vtkMetaImageWriter> writer = VTKPTR<vtkMetaImageWriter>::New();

	// Workaround: Local copy of tstat to allow access to const functions
	TensorDataStatistics local_tstat = getTensorDataStatistics();

	ImageCollection::const_iterator it = m_momentImages.begin();
	for( unsigned idx=0; it != m_momentImages.end(); it++, idx++ )	
	{		
		string name = local_tstat.getMomentMeasureName(idx);
		string filename = string(basepath) + name + ".mhd";
		string raw_filename = name + ".raw";		

		// Write image
		vtkImageData* img = it->GetPointer(); // == (*it)
		writer->SetFileName( filename.c_str() );
		writer->SetFileDimensionality( 3 ); // required?
		writer->SetCompression( false );
		writer->SetInput( img );		
		writer->Write(); // call this instead of Update()
	}
}

bool StatisticsMomentImagesIntegrator
  ::load( const char* basepath )
{
	VTKPTR<vtkMetaImageReader> reader = VTKPTR<vtkMetaImageReader>::New();

	ImageCollection::const_iterator it = m_momentImages.begin();
	for( unsigned idx=0; it != m_momentImages.end(); it++, idx++ )	
	{
		vtkImageData* img = it->GetPointer(); // == (*it)
		
		std::string filename = std::string(basepath) 
		                     + tstat().getMomentMeasureName(idx) + ".mhd";

		// Read image
		reader->SetFileName( filename.c_str() );
		reader->Update();

		// Check if image was loaded correctly
		int dims[3];
		reader->GetOutput()->GetDimensions( dims );
		if( dims[0]+dims[1]+dims[2] <= 0 )
		{
			std::cerr << "Warning: Failed to load " << filename << "!\n";
			//return false; <- no reason to get upset, just skip this image
		}
		else
		{
			// Set image data
			img->DeepCopy( reader->GetOutput() );
		}
	}

	// Update min/max ranges
	computeScalarRangeFromImages();

	return true;
}

void StatisticsMomentImagesIntegrator
  ::computeScalarRangeFromImages()
{
	// Store min/max moment values
	std::vector<double> 
		min_( tstat().getNumMomentMeasures(), +std::numeric_limits<double>::max() ),
		max_( tstat().getNumMomentMeasures(), -std::numeric_limits<double>::max() );

	ImageCollection::iterator it = m_momentImages.begin();
	unsigned idx = 0;
	for( ; it != m_momentImages.end(); it++, idx++ )
	{
		float* ptr = (float*) (*it)->GetScalarPointer();
		unsigned numVoxels = (*it)->GetNumberOfPoints();
		double zeroEps = 0.0;
		for( unsigned i=0; i < numVoxels; i++, ptr++ )
		{
			// Only consider non-zero entries
			if( *ptr > zeroEps )
			{
				// Update min/max
				min_[idx] = (*ptr < min_[idx]) ? *ptr : min_[idx];
				max_[idx] = (*ptr > max_[idx]) ? *ptr : max_[idx];
			}
		}
	}

	// Set members
	m_min = min_;
	m_max = max_;
}

void StatisticsMomentImagesIntegrator
  ::getScalarRange( int modeIdx, double (&range)[2] ) const
{
	range[0] = m_min[modeIdx];
	range[1] = m_max[modeIdx];
}
	
void StatisticsMomentImagesIntegrator
  ::setTensorVis( TensorVisBase* tvis )
{
	tstat().setTensorVisBase( tvis );
}


//============================================================================
//	LocalCovarianceStatistics
//============================================================================
LocalCovarianceStatistics
  ::LocalCovarianceStatistics()
  : m_tdata(NULL),
    m_curIntegrator(NULL)
{
	m_curIntegrator = &m_integratorMomentImages;
}

void LocalCovarianceStatistics
  ::setTensorData( SDMTensorDataProvider* tdata )
{
	m_tdata = tdata;
	m_integratorMomentImages.setTensorData( tdata );
}
	
void LocalCovarianceStatistics
  ::setTensorVis( TensorVisBase* tvis )
{
	m_integratorMomentImages.setTensorVis( tvis );
}

void LocalCovarianceStatistics
  ::setTensorDataStatistics( /*const*/ TensorDataStatistics& tstat )
{	
	m_integratorMomentImages.setTensorDataStatistics( tstat );
}

void LocalCovarianceStatistics
  ::compute()
{
	if( !m_tdata ) return;
	assert( m_curIntegrator );
	
	// Get image space over which we iterate below
	int dims[3];	
	m_space.getDimensions( dims );
	
	// Generate sampling points
	m_tdata->invalidate(); // WORKAROUND
	m_tdata->generateGridPoints( m_stepsize[0], m_stepsize[1], m_stepsize[2] );		


	m_curIntegrator->integrate_pre();


	// Iterate over valid sample points
	unsigned numPoints = m_tdata->getNumValidPoints();
	vtkPoints* points = m_tdata->getPoints();
	for( unsigned idx=0; idx < numPoints; idx++ )
	{
		if( idx%(numPoints/50) == 0 )
			std::cout << "Processing tensor field statistics " << (int)((idx/(float)(numPoints-1))*100) << "% \n";

		// Sample point is in physical coordinates
		double point[3];
		points->GetPoint( idx, point );

		// Trust our image space conversion to get pixel address
		int ijk[3];
		m_space.getIJK( point, ijk );

		// SDMTensorDataProvider works with normalized coordinates
		double normalized_point[3];
		m_space.getNormalized( point, normalized_point );

		// Update tensor data		
		m_tdata->setReferencePoint( normalized_point );
		m_tdata->updateTensorData();
				
		// Update statistics --> This is now responsibility of the integrator!!
		//m_tstat.setTensorData( m_tdata );
		//m_tstat.compute();


		m_curIntegrator->integrate_inner_loop( m_tdata, ijk );

	}

	m_curIntegrator->integrate_post();
}

void LocalCovarianceStatistics
  ::setSamplingResolution( int stepX, int stepY, int stepZ )
{
	assert( m_tdata );
	
	m_stepsize[0] = stepX;
	m_stepsize[1] = stepY;
	m_stepsize[2] = stepZ;
	
	// Get image size
	ImageDataSpace space = m_tdata->getImageDataSpace();
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
	m_space = ImageDataSpace( dims, spacing, origin );


	// Setup integrators
	m_integratorMomentImages.setup( m_space );
	m_integratorTensor.setup      ( m_space );	
}


// Forwards to integrator

void LocalCovarianceStatistics
  ::save( const char* basepath ) const
{	
	m_curIntegrator->save( basepath );
}

bool LocalCovarianceStatistics
  ::load( const char* basepath )
{
	return m_curIntegrator->load( basepath );
}

void LocalCovarianceStatistics
  ::getScalarRange( int modeIdx, double (&range)[2] ) const
{
	return m_integratorMomentImages.getScalarRange( modeIdx, range );
}