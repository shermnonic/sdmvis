#include "TensorVisBase.h"

// Visual Studio, disable the following irrelevant warnings:
// C4800: 'int' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning (disable : 4800)

//-----------------------------------------------------------------------------
//  C'tor
//-----------------------------------------------------------------------------
TensorVisBase::TensorVisBase()
	: TensorVisRenderBase(),
	  m_samplingStrategy(GridSampling),
	  m_gridStepSize(2),
	  m_sampleSize  (1000),
	  m_threshold   (23.0)
{
}

//-----------------------------------------------------------------------------
//  get/set Threshold
//-----------------------------------------------------------------------------
void   TensorVisBase::setThreshold( double t )  { m_threshold = t; }
double TensorVisBase::getThreshold() const      { return m_threshold; }

//-----------------------------------------------------------------------------
//  get/set SampleSize
//-----------------------------------------------------------------------------
void   TensorVisBase::setSampleSize( int s )    { m_sampleSize = s; }
int    TensorVisBase::getSampleSize() const     { return m_sampleSize; }

//-----------------------------------------------------------------------------
//  get/set GridStepSize
//-----------------------------------------------------------------------------
void   TensorVisBase::setGridStepSize( int s )  { m_gridStepSize = s; }
int    TensorVisBase::getGridStepSize() const   { return m_gridStepSize; }
