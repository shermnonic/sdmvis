#ifndef EDITLOCALCOVARIANCE_H
#define EDITLOCALCOVARIANCE_H

#include "SDMVisInteractiveEditing.h"
#include "SDMTensorDataProvider.h"
#include "Geometry2.h" // Icosahedron
#include "mattools.h" // mattools::RawMatrix
#include "numerics.h" // Matrix, Vector

class EditLocalCovariance : public SDMTensorDataProvider
{
public:
	EditLocalCovariance();

	// --- Interface implementation ---

	void setSDM( StatisticalDeformationModel* sdm );

	void setReferencePoint( double x, double y, double z );
	void getReferencePoint( double (&point)[3] ) const 
	{
		point[0] = m_referencePoint[0];
		point[1] = m_referencePoint[1];
		point[2] = m_referencePoint[2];
	}

	void getTensor( double x, double y, double z, 
	                        float (&tensor3x3)[9] );

	void setSphericalSamplingRadius( double r )
	{
		m_sphericalSamplingRadius = r;
	}

	double getSphericalSamplingRadius() const 
	{
		return m_sphericalSamplingRadius;
	}

	double getGamma() const
	{
		return m_edit.getGamma();
	}

	void setGamma( double gamma )
	{
		m_edit.setGamma( gamma );
	}

	// ---

protected:
	void setDirectionalSamplingLevel( int l );
	
private:	
	std::vector<Vector> m_coeffSet;
	double              m_referencePoint[3];
	
	SDMVisInteractiveEditing     m_edit;
	Icosahedron                  m_icosahedron;
	double m_sphericalSamplingRadius;
};

#endif // EDITLOCALCOVARIANCE_H
