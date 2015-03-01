#ifndef TENSORVIS2_H
#define TENSORVIS2_H

#include "TensorVisBase.h"
#include "TensorDataProvider.h"

/// New variant of \a TensorVis using \a TensorDataProvider
class TensorVis2 : public TensorVisBase
{
public:
	TensorVis2()
		: m_tensorDataProvider(NULL)
	{}

	void setDataProvider( TensorDataProvider* ptr )
	{
		m_tensorDataProvider = ptr;
	}

	TensorDataProvider* getDataProvider()
	{
		return m_tensorDataProvider;
	}

//protected:
	/// Create tensor glyph visualization by sampling tensor data
	void updateGlyphs( int/*SamplingStrategy*/ strategy );

private:
	TensorDataProvider* m_tensorDataProvider;
};

#endif // TENSORVIS2_H
