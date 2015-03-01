#ifndef TENSORNORMALDISTRIBUTIONPROVIDER_H
#define TENSORNORMALDISTRIBUTIONPROVIDER_H

#include "TensorNormalDistribution.h"
#include "TensorDataProvider.h"
#include "TensorData.h"
#include "ImageDataSpace.h"

/// TensorVis support for estimated tensor normal distribution data.
class TensorNormalDistributionProvider 
	: public TensorDataProvider, 
	  public TensorNormalDistribution
{
public:
	TensorNormalDistributionProvider();
	~TensorNormalDistributionProvider();

	/// Load tensor mean and covariance data (in custom NRRD format)
	bool setup( const char* filename );
	
	void setMaskThreshold( float t ) { m_threshold = t; }
	float getMaskThreshold() const { return m_threshold; }

	/// Set coefficients for linear combinations of modes, added to mean tensor
	void setCoeffs( float (&coeffs)[6] );

protected:
	///@{ Implementation of TensorDataProvider
	virtual void getTensor( double x, double y, double z, 
	                        float (&tensor3x3)[9] );
	virtual void getVector( double x, double y, double z,
	                        float (&vec)[3] );
	virtual bool isValidSamplePoint( double x, double y, double z );
	///@}

	///@{ Data addressing
	enum Offsets {
		OffsetMask       = 0,
		OffsetMean       = 1, 
		OffsetCovariance = 7
	};	
	inline int address( double x, double y, double z, int ofs, int stride=28 );
	inline int address( int x, int y, int z, int ofs, int stride=28 );
	///@}

private:
	float m_threshold; // Threshold for float 1D mask
	float m_coeffs[6];
};

#endif // TENSORNORMALDISTRIBUTIONPROVIDER_H
