#ifndef TENSORVISBASE_H
#define TENSORVISBASE_H

#include "TensorVisRenderBase.h"

/// Virtual base class for tensor glyph visualization in a regular 3D image domain.
/// Defines different sampling strategies. Note that the main implementation
/// of \a updateGlyphs(int strategy) has to be done in a subclass.
class TensorVisBase : public TensorVisRenderBase
{
public:
	enum SamplingStrategy { GridSampling, RandomSampling, ReUseExistingSampling };

protected:
	/// Create tensor glyph visualization by sampling tensor data
	/// \arg refvol Reference volume is required for physical coordinates
	virtual void updateGlyphs( int/*SamplingStrategy*/ strategy ) = 0;

public:
	TensorVisBase();	

	void updateGlyphs() { updateGlyphs( getSamplingStrategy() ); }

	///@{ Get/set sampling strategy, should be one of \a SamplingStrategy.
	int  getSamplingStrategy() const { return m_samplingStrategy; }
	void setSamplingStrategy( int s ) { m_samplingStrategy = s; }
	///@}
	
	///@{ Get/set sampling parameters, changes require calling \a updateGlyphs()
	void   setThreshold( double t );	
	double getThreshold() const;
	void   setSampleSize( int s );
	int    getSampleSize() const;
	void   setGridStepSize( int s );
	int    getGridStepSize() const;
	///@}

private:
	// Sampling parameters
	int    m_samplingStrategy;
	int    m_gridStepSize;// grid sampling stepsize (e.g. 2=sample even indices)
	int    m_sampleSize;  // sample size for random sampling
	double m_threshold;
};

#endif // TENSORVISBASE_H
