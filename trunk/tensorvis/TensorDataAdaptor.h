#ifndef TENSORDATAADAPTOR_H
#define TENSORDATAADAPTOR_H

#include "TensorDataProvider.h"
#include <cassert>

/// Adapt a different point sampling to another tensor provider
class TensorDataAdaptor : public TensorDataProvider
{
public:
	TensorDataAdaptor()
		: m_tdata(NULL)
	{}

	/// Forward getTensor() calls to this data provider
	void setTensorProvider( TensorDataProvider* tdata )
	{
		m_tdata = tdata;
	}

	/// TensorDataProvider implementation
	void getTensor( double x, double y, double z, 
	                        float (&tensor3x3)[9] )
	{
		assert( m_tdata );
		m_tdata->getTensor( x, y, z, tensor3x3 );
	}

private:
	TensorDataProvider* m_tdata;
};

#endif // TENSORDATAADAPTOR_H
