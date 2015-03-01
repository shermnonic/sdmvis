#ifndef LINEARLOCALCOVARIANCE_H
#define LINEARLOCALCOVARIANCE_H

#include "SDMTensorDataProvider.h"
#include "mattools.h" // mattools::RawMatrix
#include "numerics.h" // Matrix, Vector

/// Replacement for \a EditLocalCovariance class.
class LinearLocalCovariance : public SDMTensorDataProvider
{
public:
	LinearLocalCovariance();

	// --- Interface implementation ---

	void setReferencePoint( double x, double y, double z );
	void getReferencePoint( double (&point)[3] ) const 
	{
		point[0] = m_referencePoint[0];
		point[1] = m_referencePoint[1];
		point[2] = m_referencePoint[2];
	}

	void getTensor( double x, double y, double z, 
	                        float (&tensor3x3)[9] );

	double getGamma() const
	{
		return m_gamma;
	}

	void setGamma( double gamma )
	{
		m_gamma = gamma;
	}

	// WORKAROUND: Disable additional vector field visualization
	vtkDataArray* getVectors() { return NULL; }

	void invalidate() { m_Zp.clear(); }

	// ---

	void getCoefficients( Vector edit, Vector& coeffs );

	enum TensorTypes { 
		CovarianceTensor, 
		InteractionOperator, 
		IntegralTensor,
		SelfInteractionTensor,
		SelfInteractionOperator
	};
	void setTensorType( int type ) { m_tensorType = type; }
	int getTensorType() { return m_tensorType; }

	/// Set sample covariance matrix (identity by default, an alternative could
	/// be global anatomic covariance field).
	void setSampleCovariance( TensorDataProvider* cv ) { m_sampleCovariance=cv; }
	TensorDataProvider* getSampleCovariance() { return m_sampleCovariance; }

protected:
	void getReducedMatrix( double* v, Matrix& B );
	void getReducedMatrix( double x, double y, double z, Matrix& B );

	Matrix getZp( double x, double y, double z );
	
private:	
	std::vector<Vector> m_coeffSet;
	double              m_referencePoint[3];	

	double m_gamma;

	Matrix m_Zp;

	int m_tensorType;

	TensorDataProvider* m_sampleCovariance;
};

#endif // LINEARLOCALCOVARIANCE_H
