#ifndef TENSORNORMALDISTRIBUTION_H
#define TENSORNORMALDISTRIBUTION_H

#include "ImageDataSpace.h"
#ifdef TENSORVIS_TEEM_SUPPORT
 #include <teem/nrrd.h>
#endif

/// Estimated tensor normal distribution data.
class TensorNormalDistribution
{
public:
	enum Constants { 
		NumModes = 6 
	};

	TensorNormalDistribution()
	: m_data (0),
	  m_modes(0),
	  m_spect(0)
	{}

	~TensorNormalDistribution()
	{
		delete [] m_data;
		delete [] m_modes;
		delete [] m_spect;
	}

#ifdef TENSORVIS_TEEM_SUPPORT
	/// Load custom 28D nrrd format containing tensor mean and covariance
	bool load( const char* filename );

	/// Load tensor covariance eigenmodes from custom nrrd
	bool load_modes( const char* filename );
	/// Load tensor covariance eigenvalues from custom nrrd
	bool load_spectrum( const char* filename );

	/// Save tensor covariance eigenmodes
	bool save_modes   ( const char* filename );
	/// Save tensor covariance eigenvalues
	bool save_spectrum( const char* filename );
#else
	// If teem is not supported, all io functions are dummies returning false
	bool load( const char* filename )            { return false; }
	bool load_modes( const char* filename )      { return false; }
	bool load_spectrum( const char* filename )   { return false; }
	bool save_modes   ( const char* filename )   { return false; }
	bool save_spectrum( const char* filename )   { return false; }
#endif

	/// Compute tensor covariance eigenmodes and eigenvalues (expensive!)
	void computeModes();

protected:
	///@{ Raw data access
	float* tensorData()  { return m_data; }
	float* tensorModes() { return m_modes; }
	float* tensorSpect() { return m_spect; }
	///@}

#ifdef TENSORVIS_TEEM_SUPPORT
	///@{ NRRD helper function
	bool save_attribute( const char* filename, float* ptr, int dim );
	bool load_attribute( const char* filename, float*& ptr, int dim );
	Nrrd* open_nrrd( const char* filename, const char* me );
	///@}
#endif

	///@{ Data addressing
	enum Offsets {
		OffsetMask       = 0,
		OffsetMean       = 1, 
		OffsetCovariance = 7
	};
	///@}

	ImageDataSpace& space() { return m_space; }

private:
	ImageDataSpace m_space;
	float* m_data;   // Custom 28D format: 1D mask, 6D mean, 21D covariance
	float* m_modes;  // Covariance modes (eigenvectors)
	float* m_spect;  // Covariance spectrum (eigenvalues)
};

#endif // TENSORNORMALDISTRIBUTION_H
