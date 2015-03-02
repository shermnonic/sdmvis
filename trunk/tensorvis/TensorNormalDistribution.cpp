#include "TensorNormalDistribution.h"
#include <iostream>
#include <cstdio>

#include "rednum.h"
namespace {
	typedef boost::numeric::ublas::matrix< float > Matrix;
	typedef boost::numeric::ublas::vector< float > Vector;
};

#ifdef TENSORVIS_TEEM_SUPPORT

bool TensorNormalDistribution::save_attribute( const char* filename, float* ptr, int dim )
{
	char me[] = "TensorNormalDistribution::save_attribute()";
	Nrrd* nout = nrrdNew();

	// Save 4D data, 1D attribute + 3D volume dimensions
	int dims[3];
	m_space.getDimensions( dims );

	size_t sizes[4];
	sizes[0] = dim;
	sizes[1] = dims[0];
	sizes[2] = dims[1];
	sizes[3] = dims[2];

#if 1
	if( nrrdWrap_nva( nout, m_modes, nrrdTypeFloat, 4, sizes ) )
#else
	if( nrrdWrap_va( nout, m_modes, nrrdTypeFloat, 4, 
		             (unsigned int)dim, (unsigned int)dims[0], (unsigned int)dims[1], (unsigned int)dims[2] ) )
#endif
	{
		char* err = biffGetDone(NRRD);
		fprintf( stderr, "%s: trouble wrapping \"%s\":\n%s", me, filename, err );
		free( err );
		return false;
	}

	if( nrrdSave( filename, nout, NULL ) )
	{
		char* err = biffGetDone(NRRD);
		fprintf( stderr, "%s: trouble writing \"%s\":\n%s", me, filename, err );
		free( err );
		return false;	
	}

	nrrdNix( nout );
	return true;
}

bool TensorNormalDistribution::save_modes( const char* filename )
{
	return save_attribute( filename, m_modes, NumModes*6 );
}

bool TensorNormalDistribution::save_spectrum( const char* filename )
{
	return save_attribute( filename, m_spect, NumModes );
}

Nrrd* TensorNormalDistribution::open_nrrd( const char* filename, const char* me )
{
	// Code adapted from the nrrd documentation at:
	// http://teem.sourceforge.net/nrrd/lib.html (retrieved on 2013-10-04)
	
	Nrrd* nin = nrrdNew();

	if( nrrdLoad( nin, filename, NULL ) )
	{
		char* err = biffGetDone(NRRD);
		fprintf( stderr, "%s: trouble reading \"%s\":\n%s", me, filename, err );
		free( err );
		return NULL;
	}
	
	// say something about the array
	printf("%s: \"%s\" is a %d-dimensional nrrd of type %d (%s)\n", 
		 me, filename, nin->dim, nin->type,
		 airEnumStr(nrrdType, nin->type));
	printf("%s: the array contains %d elements, each %d bytes in size\n",
		 me, (int)nrrdElementNumber(nin), (int)nrrdElementSize(nin));

	return nin;
}

bool TensorNormalDistribution::load( const char* filename )
{
	using namespace std;

	char me[] = "TensorNormalDistribution::load()";
	Nrrd* nin = open_nrrd( filename, me );
	if( !nin )
		return false;

	// Hardcoded handling of Thomas 28D statistics tensor.
	// We simply extract the symmetric mean tensor stored in floats 2 to 8.

	// Sanity check
	if( nin->axis[0].size!=28 )
	{
		fprintf( stderr, "%s: file type mismatch \"%s\"", me, filename );
		nrrdNuke(nin);
		return false;
	}

	// Establish ImageDataSpace
	int dims[3];
	dims[0] = (int)nin->axis[1].size;
	dims[1] = (int)nin->axis[2].size;
	dims[2] = (int)nin->axis[3].size;

	double spacing[3] = { 1, 1, 1 };

	double origin[3];
	origin[0] = nin->spaceOrigin[0];
	origin[1] = nin->spaceOrigin[1];
	origin[2] = nin->spaceOrigin[2];

	ImageDataSpace space( dims, spacing, origin );

	// Sanity check	size
	if( (unsigned)(nrrdElementNumber(nin)/28) != space.getNumberOfPoints() )
	{
		fprintf( stderr, "%s: tensor field size mismatch \"%s\"", me, filename );
		nrrdNuke(nin);
		return false;
	}
	
	// Sanity check type
	if( nin->type != nrrdTypeFloat )
	{
		fprintf( stderr, "%s: tensor field type mismatch \"%s\"", me, filename );
		nrrdNuke(nin);
		return false;
	}

	// Store data pointer
	if( m_data ) free( m_data );
	m_data = (float*)(nin->data);	
	
	// Blow away only the Nrrd struct but *not* the memory at nin->data
	// ( nrrdNix() frees the struct but not the data,
	//   nrrdEmpty() frees the data but not the struct )
	nrrdNix(nin);

	// Set members	
	m_space = space; // was: setImageDataSpace( space );

	return true;	
}

bool TensorNormalDistribution::load_modes( const char* filename )
{
	return load_attribute( filename, m_modes, NumModes*6 );
}

bool TensorNormalDistribution::load_spectrum( const char* filename )
{
	return load_attribute( filename, m_spect, NumModes );
}

bool TensorNormalDistribution::load_attribute( const char* filename, float* &ptr, int dim )
{
	char me[] = "TensorNormalDistribution::load_attribute()";
	Nrrd* nin = open_nrrd( filename, me );
	if( !nin )
		return false;

	// Sanity check size - size must match data space of main nrrd file
	int dims[3];
	m_space.getDimensions( dims );
	if( nin->axis[0].size != dim        ||
		nin->axis[1].size != dims[0]    ||
		nin->axis[2].size != dims[1]    ||
		nin->axis[3].size != dims[2] )
	{
		fprintf( stderr, "%s: file type mismatch \"%s\"", me, filename );
		nrrdNuke(nin);
		return false;
	}
	
	// Sanity check type
	if( nin->type != nrrdTypeFloat )
	{
		fprintf( stderr, "%s: tensor field type mismatch \"%s\"", me, filename );
		nrrdNuke(nin);
		return false;
	}
	
	// Store data pointer
	if( ptr ) delete [] ptr;  // FIXME: Do we have to use free() instead?
	ptr = (float*)(nin->data);

	// Blow away only the Nrrd struct but *not* the memory at nin->data
	nrrdNix(nin);

	return true;
}

#endif // TENSORVIS_TEEM_SUPPORT


void TensorNormalDistribution::computeModes()
{
	// Sanity
	if( !m_data )
		return;
	
	// Free memory
	if( m_modes ) delete [] m_modes; m_modes = 0;
	if( m_spect ) delete [] m_spect; m_spect = 0;
	
	// Allocate memory
	int N = m_space.getNumberOfPoints(); // was: getImageDataSpace().get...
	m_modes = new float[ 6 * NumModes * N ];
	m_spect = new float[     NumModes * N ];

	// Covariance weight (note the factor 2.0 in the lower right block)
	Matrix W(6,6);
	float sqrt2 = (float)sqrt(2.), one=1.f, two=2.f;
	W(0,0)=one; W(0,1)=one; W(0,2)=one; W(0,3)=sqrt2; W(0,4)=sqrt2; W(0,5)=sqrt2;
	W(1,0)=one; W(1,1)=one; W(1,2)=one; W(1,3)=sqrt2; W(1,4)=sqrt2; W(1,5)=sqrt2;
	W(2,0)=one; W(2,1)=one; W(2,2)=one; W(2,3)=sqrt2; W(2,4)=sqrt2; W(2,5)=sqrt2;
	W(3,0)=sqrt2; W(3,1)=sqrt2; W(3,2)=sqrt2; W(3,3)=two; W(3,4)=two; W(3,5)=two;
	W(4,0)=sqrt2; W(4,1)=sqrt2; W(4,2)=sqrt2; W(4,3)=two; W(4,4)=two; W(4,5)=two;
	W(5,0)=sqrt2; W(5,1)=sqrt2; W(5,2)=sqrt2; W(5,3)=two; W(5,4)=two; W(5,5)=two;
	
	// A very expensive loop ;-)
	static Matrix Sigma(6,6);
	static Matrix U, V; // eigenvectors
	static Vector eigenvals;
#if 0
	for( int n=0; n < 1000; n++ ) // for debugging purposes operate on 100 entries only
#else
	for( int n=0; n < N; n++ )
#endif
	{
		//std::cout << "Computing covariance mode " << n+1 << " / " << N <<"\r";
		if( n%100 == 0 )
			printf("Computing covariance modes %3.0f%%\r",(n/(float)(N-1))*100.f);
		
		int addr = n * 28 + OffsetCovariance;
		
		// Convert symmetric repr. to full 6x6 matrix
		int ofs = 0;
		for( int i=0; i < 6; i++ )
			for( int j=i; j < 6; j++, ofs++ )
			{
				Sigma(i,j) = Sigma(j,i) = m_data[ addr + ofs ];
				// Apply covariance weights
				Sigma(i,j) = W(i,j) * Sigma(i,j);
				Sigma(j,i) = W(j,i) * Sigma(j,i);
			}
		
		// FIXME: Exploit symmetry and avoid SVD computation
		rednum::compute_svd<Matrix,Vector,float>( Sigma, U, eigenvals, V );

		// Eigen-tensor weight
		double one_over_sqrt2 = 1. / sqrt(2.);
		Vector wv(6);
		wv(0) = (Vector::value_type)1.;
		wv(1) = (Vector::value_type)1.;
		wv(2) = (Vector::value_type)1.;
		wv(3) = (Vector::value_type)one_over_sqrt2;
		wv(4) = (Vector::value_type)one_over_sqrt2;
		wv(5) = (Vector::value_type)one_over_sqrt2;
		
		// Copy result
		ofs = 0;
		int mode_addr = n * 6 * NumModes;
		for( int j=0; j < NumModes; j++ )
			for( int i=0; i < 6; i++, ofs++ )
			 m_modes[mode_addr + ofs] = U(i,j) * wv(i);
				// was: ... * ((i>=3) ? one_over_sqrt2 : 1.);
			
		int spect_addr = n * NumModes;
		for( int i=0; i < NumModes; i++ )
			m_spect[spect_addr + i] = eigenvals[i];
	}
	std::cout << std::endl;	
}
