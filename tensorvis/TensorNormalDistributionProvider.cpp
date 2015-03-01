#include "TensorNormalDistributionProvider.h"
#include <iostream>
#include <string>
#include <cstdio>
#include <cstring> 
#include <cassert>

//-----------------------------------------------------------------------------
//	Helper functions
//-----------------------------------------------------------------------------

/// Convert 6D symmetric tensor representation to full 3x3 matrix
inline
void unroll( float (&symmetric6)[6], float (&tensor3x3)[9] )
{
	// [ 0 1 2 ]      [ 0 1 2 ]
	// [   3 4 ] <->  [ 3 4 5 ]
	// [     5 ]      [ 6 7 8 ]	
	tensor3x3[0]                = symmetric6[0];
	tensor3x3[1] = tensor3x3[3] = symmetric6[1];
	tensor3x3[2] = tensor3x3[6] = symmetric6[2];
	tensor3x3[4]                = symmetric6[3];
	tensor3x3[5] = tensor3x3[7] = symmetric6[4];
	tensor3x3[8]                = symmetric6[5];
}

//-----------------------------------------------------------------------------
//	C'tor / D'tor
//-----------------------------------------------------------------------------
TensorNormalDistributionProvider::TensorNormalDistributionProvider()
	: m_threshold(.5f)
{
	m_coeffs[0] = 0.f;
	m_coeffs[1] = 0.f;
	m_coeffs[2] = 0.f;
	m_coeffs[3] = 0.f;
	m_coeffs[4] = 0.f;
	m_coeffs[5] = 0.f;
}

TensorNormalDistributionProvider::~TensorNormalDistributionProvider()
{
}

//-----------------------------------------------------------------------------
//	address()
//-----------------------------------------------------------------------------
inline
int TensorNormalDistributionProvider::
  address( double x, double y, double z, int ofs, int stride )
{
	assert( tensorData() );	
	return getImageDataSpace().getPointIndex( x,y,z )*stride + ofs;
}

inline
int TensorNormalDistributionProvider::
  address( int i, int j, int k, int ofs, int stride )
{
	assert( tensorData() );	
	return getImageDataSpace().getPointIndex( i,j,k )*stride + ofs;
}

//-----------------------------------------------------------------------------
//	Implementation of TensorDataProvider
//-----------------------------------------------------------------------------
void TensorNormalDistributionProvider::
  getTensor( double x, double y, double z, float (&tensor3x3)[9] )
{
	// Get array indices from coordinates
	static int ijk[3];
	getImageDataSpace().getIJK( x,y,z, ijk );	

	// Get mean tensor
	static float mu[6];	
	int ofs = address( ijk[0],ijk[1],ijk[2], OffsetMean );	
	memcpy( (void*)mu, (void*)(&tensorData()[ofs]), 6*sizeof(float) );

	// Linear combination of modes
	for( int mode=0; mode < TensorNormalDistribution::NumModes; mode++ )
	{
		if( m_coeffs[mode] != 0.f )
		{
			// Get mode
			int stride = TensorNormalDistribution::NumModes * 6;
			int ofs_mode = address( ijk[0],ijk[1],ijk[2], mode*6, stride );
			
			static float pc[6];
			memcpy( (void*)pc, (void*)(&tensorModes()[ofs_mode]), 6*sizeof(float) );

			// Add to mu
			for( int i=0; i < 6; i++ )
				mu[i] += m_coeffs[mode] * pc[i];
		}
	}
	
	// Convert 6D symmetric repr. to full 9D matrix
	unroll( mu, tensor3x3 );
}

void TensorNormalDistributionProvider::
  getVector( double x, double y, double z,
	                        float (&vec)[3] )
{
	vec[0] = vec[1] = vec[2] = 0.f;
	return;
}

bool TensorNormalDistributionProvider::
  isValidSamplePoint( double x, double y, double z )
{	
	float* data = tensorData();
	int ofs = address(x,y,z,OffsetMask);
	return data[ofs] > m_threshold;
}

//-----------------------------------------------------------------------------
//	setup()
//-----------------------------------------------------------------------------
bool TensorNormalDistributionProvider::
  setup( const char* filename )
{
	using namespace std;

	bool ok = TensorNormalDistribution::load( filename );

	if( ok )
		setImageDataSpace( TensorNormalDistribution::space() );

	// Try to load additional modes and spectrum attributes
	string filename_s = string(filename);
	size_t sep = filename_s.find_last_of('.');
	if( sep != string::npos )
	{
		string prefix = filename_s.substr( 0, sep ),
		       ext    = filename_s.substr( sep );
		
		// Guess filenames
		string fname_modes = prefix + "-modes" + ext,
			   fname_spect = prefix + "-spect" + ext;

		bool success = true;
		success &= TensorNormalDistribution::load_modes   ( fname_modes.c_str() );
		success &= TensorNormalDistribution::load_spectrum( fname_spect.c_str() );

		if( success )
			cout << "Successfully loaded modes and spectrum attributes!" << endl;
	}

	return ok;
}

//-----------------------------------------------------------------------------
//	setCoeffs()
//-----------------------------------------------------------------------------
void TensorNormalDistributionProvider::
  setCoeffs( float (&coeffs)[6] )
{
	for( int i=0; i < 6; i++ )
		m_coeffs[i] = coeffs[i];
}