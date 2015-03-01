#include "TensorVis2.h"
#include "ImageDataSpace.h"
#include <iostream>

//-----------------------------------------------------------------------------
void TensorVis2::updateGlyphs( int/*SamplingStrategy*/ strategy )
{
	// Sanity check
	if( !m_tensorDataProvider )
	{
		std::cout << "Warning: No tensor data provider!\n";
		return;
	}

	// Set threshold
	m_tensorDataProvider->setImageThreshold( getThreshold() );

	// Generate sample points
	if( strategy == GridSampling )
	{
		m_tensorDataProvider->generateGridPoints( getGridStepSize() );
	}
	else
	if( strategy == RandomSampling )
	{
		m_tensorDataProvider->generateRandomPoints( getSampleSize() );
	}
	else
	if( strategy == ReUseExistingSampling )
	{
		// Do nothing, assume that sampling is already done and still valid!
	}
	else
	{
		std::cerr << "Error: Unsupported sampling strategy!\n";
		return;
	}

	std::cout << "Number of sample points = " << m_tensorDataProvider->getNumValidPoints() << "\n";
	if( m_tensorDataProvider->getNumValidPoints() < 1 )
		return;
	
	// Update tensor data
	m_tensorDataProvider->updateTensorData();
	
	// Update visualization
	updateGlyphData( 
			m_tensorDataProvider->getPoints(),
			m_tensorDataProvider->getTensors(),
			m_tensorDataProvider->getVectors() 
		);
}
