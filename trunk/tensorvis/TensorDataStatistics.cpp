#include "TensorDataStatistics.h"
#include "ImageDataSpace.h"
#include <cassert>

TensorDataStatistics
  ::TensorDataStatistics()
  : m_tdata( NULL ),
	m_tvis ( NULL ),
	m_moment( NumMoments*NumMeasures, 0. )
{	
#ifdef TENSORDATASTATISTICS_SUPPORT_MOMENT_IMAGES	
	// Create image instances (does *not* allocate image memory)
	m_momentImages.resize( NumMeasures * NumMoments );
	for( unsigned i=0; i < m_momentImages.size(); i++ )
	{
		m_momentImages[i] = vtkSmartPointer<vtkImageData>::New();
	}
#endif	
}

void TensorDataStatistics
  ::setTensorData( TensorDataProvider* tdata )
{
	m_tdata = tdata;
}
	
void TensorDataStatistics
  ::setTensorVisBase( TensorVisBase* tvis )
{
	m_tvis = tvis;
}

void TensorDataStatistics
  ::getAverageTensor( double (&tensor)[9] ) const
{
	for( unsigned i=0; i < 9; i++ )
		tensor[i] = m_avgTensor[i];
}

void TensorDataStatistics
  ::computeAverageTensor()
{
	if( !m_tdata ) return;

	// Zero average tensor
	memset( (void*)m_avgTensor, 0, 9*sizeof(double) );

	// Point and tensor datasets have identical indices, iterate over points.
	vtkPoints* points = m_tdata->getPoints();
	unsigned numPoints = m_tdata->getNumValidPoints();

	for( unsigned id=0; id < numPoints; id++ )
	{		
		// Get tensor data
		double* T = m_tdata->getTensors()->GetTuple9( id );

		// Accumulate
		for( unsigned i=0; i < 9; i++ )
			m_avgTensor[i] += T[i];
	}

	// Average
	for( unsigned i=0; i < 9; i++ )
		m_avgTensor[i] /= numPoints;
}

void TensorDataStatistics
  ::compute()
{
	if( !m_tdata ) return;	
	
	// Initialize all moments before accumulation
	for( unsigned i=0; i < m_moment.size(); i++ )
		m_moment[i] = 0.;
	
	// Point and tensor datasets have identical indices, iterate over points.
	vtkPoints* points = m_tdata->getPoints();
	unsigned numPoints = m_tdata->getNumValidPoints();
	
	// First moments
	for( unsigned id=0; id < numPoints; id++ )
	{		
		// Get tensor data
		TensorSpectrum ts = getSpectrum( m_tdata->getTensors()->GetTuple9( id ) );
		
		// Accumulate average		
		m_moment[ getIndex( Average, MaxEigenvalue ) ] += ts.maxEigenvalue();
		m_moment[ getIndex( Average, MeanDiffusivity)] += ts.meanDiffusivity();
		m_moment[ getIndex( Average, FrobeniusNorm)  ] += ts.frobeniusNorm();
		m_moment[ getIndex( Average, FractionalAnisotropy)] 
		                                          += ts.fractionalAnisotropy();
	}
	
	// Average
	m_moment[ getIndex( Average, MaxEigenvalue )      ] /= numPoints;
	m_moment[ getIndex( Average, MeanDiffusivity)     ] /= numPoints;
	m_moment[ getIndex( Average, FrobeniusNorm)       ] /= numPoints;
	m_moment[ getIndex( Average, FractionalAnisotropy)] /= numPoints;	
	
	// Second moments
	for( unsigned id=0; id < numPoints; id++ )
	{		
		// Get tensor data
		TensorSpectrum ts = getSpectrum( m_tdata->getTensors()->GetTuple9( id ) );
		
		// Accumulate 2nd moment
		
		double me = ts.maxEigenvalue();
		double md = ts.meanDiffusivity();
		double fa = ts.fractionalAnisotropy();
		double fn = ts.frobeniusNorm();
		
		double avg_me = m_moment[ getIndex( Average, MaxEigenvalue )      ];
		double avg_md = m_moment[ getIndex( Average, MeanDiffusivity)     ];
		double avg_fa = m_moment[ getIndex( Average, FractionalAnisotropy)];
		double avg_fn = m_moment[ getIndex( Average, FrobeniusNorm)       ];
		
		double me2 = (me - avg_me) * (me - avg_me);		
		double md2 = (md - avg_md) * (md - avg_md);
		double fa2 = (fa - avg_fa) * (fa - avg_fa);
		double fn2 = (fn - avg_fn) * (fn - avg_fn);
		
		m_moment[ getIndex( Variance, MaxEigenvalue )      ] += me2;
		m_moment[ getIndex( Variance, MeanDiffusivity)     ] += md2;
		m_moment[ getIndex( Variance, FractionalAnisotropy)] += fa2;
		m_moment[ getIndex( Variance, FrobeniusNorm)       ] += fn2;
	}
	
	// Average
	m_moment[ getIndex( Variance, MaxEigenvalue )      ] /= numPoints;
	m_moment[ getIndex( Variance, MeanDiffusivity)     ] /= numPoints;
	m_moment[ getIndex( Variance, FractionalAnisotropy)] /= numPoints;
	m_moment[ getIndex( Variance, FrobeniusNorm)       ] /= numPoints;
}	

int TensorDataStatistics
  ::getIndex( int moment, int measure ) const
{
	assert( 0 <= moment ); 	assert( moment < NumMoments );
	assert( 0 <= measure);  assert( measure< NumMeasures );
	return measure*NumMoments + moment;
}

std::string TensorDataStatistics
  ::getMomentMeasureName( unsigned index ) const
{
	std::string name;
	switch( index )
	{
	case 0: name = "max.eigenvalue-average";        break;
	case 1: name = "max.eigenvalue-variance";       break;
	case 2: name = "mean-diffusivity-average";      break;
	case 3: name = "mean-diffusivity-variance";     break;
	case 4: name = "fractional-anisotropy-average"; break;
	case 5: name = "fractional-anisotropy-variance";break;
	case 6: name = "frobeniusnorm-average";         break;
	case 7: name = "frobeniusnorm-variance";        break;
	default:
		name = "unkown";
	}

	return name;
}

double TensorDataStatistics
  ::getMoment( int moment, int measure ) const
{
	return m_moment[ getIndex(moment,measure) ];
}

TensorSpectrum TensorDataStatistics
  ::getSpectrum( double* tensor )
{
	// Do we have to compute eigendecomposition?
	// If a TensorVisBase is associated, use its setting.
	bool extractEigenvalues = m_tvis?m_tvis->getExtractEigenvalues() : true;	
	
	TensorSpectrum ts;
	if( extractEigenvalues )			
		// We have tensor data, compute eigenvectors
		ts.compute( tensor );
	else
		// We have scaled eigenvector basis given
		ts.set( tensor );

	return ts;
}

#ifdef TENSORDATASTATISTICS_SUPPORT_MOMENT_IMAGES
float* TensorDataStatistics
  ::getScalarPointer( vtkImageData* img, const double (&point)[3] )
{
	// Trust our own image space conversion
	ImageDataSpace space = m_tdata->getImageDataSpace();	
	int ijk[3];
	space.getIJK( point, ijk );
	
	return (float*)(img->GetScalarPointer( ijk[0], ijk[1], ijk[2] ));
}

void TensorDataStatistics
  ::compute_internal( const double (&point)[3], /*const*/ TensorSpectrum& ts )
{
	
	vtkImageData* img = m_momentImages[0];
	
	getScalarPointer( img, point, ts );
	
	/*
	enum Measures { MaximumEigenvalue, MeanDiffusivity, FractionalAnisotropy, 
		            NumMeasures };
	enum Moments  { Average, Variance, 
		            NumMoments };	
	*/
	
	
	double 
}

void TensorDataStatistics
  ::updateTensorData()
{
	assert( m_tdata );

	// Generate tensor data
	m_tdata->generateGridPoints( m_stepsize[0], m_stepsize[1], m_stepsize[2] );	
	m_tdata->updateTensorData();
}

void TensorDataStatistics
  ::setSamplingResolution( int stepX, int stepY, int stepZ )
{
	assert( m_tdata );
	
	m_stepsize[0] = stepX;
	m_stepsize[1] = stepY;
	m_stepsize[2] = stepZ;
	
	// Get image size
	ImageDataSpace space = m_tdata->getImageDataSpace();
	int dims[3];
	space->getDimensions( dims );
	
	// Image extent after resampling with given stepsize
	int extent[6] = { 0, 0, 0, 0, 0, 0 };
	extent[1] = dims[0] / stepX;
	extent[3] = dims[1] / stepY;
	extent[5] = dims[2] / stepZ;
	
	// Calculate new image spacing
	double spacing[3];
	space->getSpacing( spacing );
	spacing[0] *= (double)stepX;
	spacing[1] *= (double)stepY;
	spacing[2] *= (double)stepZ;
	
	// Allocate image memory
	ImageCollection::iterator img = m_momentImages.begin();
	for( ; img != m_momentsImages.end(); img++ )	
	{
		img->SetExtent ( extent );
		img->SetSpacing( spacing );
		img->AllocateScalars( VTK_FLOAT, 1 );
	}
}
#endif
