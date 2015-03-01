#ifndef SDMTENSORPROBE_H
#define SDMTENSORPROBE_H

#include "SDMTensorDataProvider.h"
#include "TensorVisBase.h"
#include "ImageDataSpace.h"
#include <string>

class TensorVisProperties
{
public:
	std::string name;
	
	// TensorvisBase sampling
	int    samplingStrategy;
	double samplingThreshold;
	int    samplingSize;
	int    samplingStepsize;
	
	// TensorvisBase glyph
	int    glyphType;
	double glyphScaleFactor;
	int    glyphColorBy;
	bool   glyphExtractEigenvalues;
	double glyphSuperquadricGamma;

	void getFrom( const TensorVisBase* tvb )
	{
		samplingStrategy  = tvb->getSamplingStrategy();
		samplingThreshold = tvb->getThreshold();
		samplingSize      = tvb->getSampleSize();
		samplingStepsize  = tvb->getGridStepSize();
		
		glyphType         = tvb->getGlyphType();
		glyphScaleFactor  = tvb->getGlyphScaleFactor();
		glyphColorBy      = tvb->getColorMode();		
		glyphExtractEigenvalues= tvb->getExtractEigenvalues();
		glyphSuperquadricGamma = tvb->getSuperquadricGamma();
	}
	
	void applyTo( TensorVisBase* tvb )
	{
		tvb->setSamplingStrategy( samplingStrategy );
		tvb->setThreshold       ( samplingThreshold );
		tvb->setSampleSize      ( samplingSize );
		tvb->setGridStepSize    ( samplingStepsize );
		
		tvb->setGlyphType       ( glyphType );
		tvb->setGlyphScaleFactor( glyphScaleFactor );
		tvb->setColorMode       ( glyphColorBy );		
		tvb->setExtractEigenvalues( glyphExtractEigenvalues );
		tvb->setSuperquadricGamma ( glyphSuperquadricGamma );
	}
};

class SDMTensorProbe : public TensorVisProperties
{
public:
	// SDMTensorDataProvider
	double point[3]; ///< Position (in physical coordinates)
	int    ijk[3];   ///< Position (voxel address, implicitly computed)
	double gamma;
	double radius;	

	void getFrom( const TensorVisBase* tvb, const SDMTensorDataProvider* tdp )
	{
		TensorVisProperties::getFrom( tvb );

		gamma  = tdp->getGamma();
		radius = tdp->getSphericalSamplingRadius();
		
		tdp->getReferencePoint( point );
		
		ImageDataSpace space = tdp->getImageDataSpace();
		space.getIJK( point, ijk );
	}
	
	void applyTo( TensorVisBase* tvb, SDMTensorDataProvider* tdp )
	{
		tdp->setGamma( gamma );
		tdp->setSphericalSamplingRadius( radius );
		
		tdp->setReferencePoint( point );

		TensorVisProperties::applyTo( tvb );
	}	
};

#endif // SDMTENSORPROBE_H
