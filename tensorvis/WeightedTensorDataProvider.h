#ifndef WEIGHTEDTENSORDATAPROVIDER_H
#define WEIGHTEDTENSORDATAPROVIDER_H

#include "TensorDataProvider.h"
//#include "SDMTensorDataProvider.h"

class WeightedTensorDataProvider : public TensorDataProvider
{
private:
	TensorDataProvider* m_tdata;
	TensorDataProvider* m_weights;

public:
	WeightedTensorDataProvider()
		: m_tdata(NULL),
		  m_weights(NULL)
	{}

	void setTensorDataProvider( TensorDataProvider* tdata )
	{
		// Copy settings from the data provider
		if( tdata )
		{
			setImageDataSpace( tdata->getImageDataSpace() );
			setImageThreshold( tdata->getImageThresholdLow(),
				               tdata->getImageThresholdHigh() );
		}
		m_tdata = tdata;
	}
	
	void setWeightsTensorDataProvider( TensorDataProvider* weights )
	{
		m_weights = weights;
	}

	// Implementation of TensorDataProvider
	void getTensor( double x, double y, double z, 
	                        float (&tensor3x3)[9] );
};

/*
	In a first concept, the data provider was specifically a SDM one and some
	and the according SDM specializations had forward functions:

	///@{ Forwards to internal SDMTensorDataProvider
	void setReferencePoint( double x, double y, double z ) 
	{ 
		if( !m_tdata ) return;
		m_tdata->setReferencePoint( x, y, z );
	}
	void getReferencePoint( double (&point)[3] ) const 
	{ 
		if( !m_tdata ) return;
		m_tdata->getReferencePoint( point );
	}
	
	void setSphericalSamplingRadius( double r ) 
	{ 
		if( !m_tdata ) return;
		m_tdata->setSphericalSamplingRadius( r );
	}
	double getSphericalSamplingRadius() const 
	{ 
		if( !m_tdata ) return 0.0;
		return m_tdata->getSphericalSamplingRadius(); 
	}

	double getGamma() const 
	{ 
		if( !m_tdata ) return 0.0;
		return m_tdata->getGamma(); 
	}
	void setGamma( double gamma ) 
	{
		if( !m_tdata ) return;
		m_tdata->setGamma( gamma );
	}
	///@}	
*/

#endif // WEIGHTEDTENSORDATAPROVIDER_H
