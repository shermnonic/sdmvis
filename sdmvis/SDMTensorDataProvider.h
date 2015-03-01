#ifndef SDMTENSORDATAPROVIDER
#define SDMTENSORDATAPROVIDER

#include "TensorDataProvider.h"
#include <cassert>

class StatisticalDeformationModel;

/// Pure interface with (mostly) empty implementations
class SDMTensorDataProvider : public TensorDataProvider
{
public:
	/// Use the same image data space as the SDM
	virtual void setSDM( StatisticalDeformationModel* sdm );

	void setReferencePoint( double* pt )
	{
		setReferencePoint( pt[0], pt[1], pt[2] );
	}
	// FIXME: Please check why we did not make the following two func's virtual!
	virtual void setReferencePoint( double x, double y, double z ) { assert(false); }
	virtual void getReferencePoint( double (&point)[3] ) const { assert(false); }
	virtual void getTensor( double x, double y, double z, 
	                        float (&tensor3x3)[9] ) {};

	virtual void setSphericalSamplingRadius( double r ) { m_dummyRadius = r; }
	virtual double getSphericalSamplingRadius() const { return m_dummyRadius; }

	virtual double getGamma() const { return 0.0; }
	virtual void setGamma( double gamma ) {}

	virtual void invalidate() {}
	
protected:
	StatisticalDeformationModel* getSDM() { return m_sdm; }
		
private:
	StatisticalDeformationModel* m_sdm;
	double m_dummyRadius; // unused
};

#endif // SDMTENSORDATAPROVIDER
