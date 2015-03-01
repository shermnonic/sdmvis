#ifndef SDMVISINTERACTIVEEDITING_H
#define SDMVISINTERACTIVEEDITING_H

#include "StatisticalDeformationModel.h"
#include "e7/VolumeRendering/RayPickingInfo.h"
#include "mattools.h" // mattools::RawMatrix
#include "numerics.h" // Matrix, Vector, rednum::solve_ls()
#include <QVector3D>

//==============================================================================
//	SDMVisInteractiveEditing
//==============================================================================

/// Constrained least-squares deformation editing
class SDMVisInteractiveEditing
{
public:
	/// 3D vector type (internally used by \a SDMVisInteractiveEditing)
	typedef QVector3D VectorType;

	/// The used index vector datatype to address rows in the matrix.
	typedef mattools::RawMatrix::IndexVector IndexVector;

	/// Weighting modes of the old variant of the solver, \see solveWeighted()
	enum ScalingModes {
		ModePlain,
		ModeNormalized,
		ModeSqrt,
		ModeInverse
	};

	SDMVisInteractiveEditing();

	void setSDM( StatisticalDeformationModel* sdm ) 
	{ 
		m_sdm = sdm; 
		if( sdm )
		{
			m_eigenmodes  = sdm->getEigenmodesRawMatrix();
			m_eigenvalues = sdm->getEigenvalues();

			m_volumeSize[0] = sdm->getHeader().resolution[0];
			m_volumeSize[1] = sdm->getHeader().resolution[1];
			m_volumeSize[2] = sdm->getHeader().resolution[2];
		}
	}

	///@{ Required input
	void setVolumeSize( int w, int h, int d )
	{
		m_volumeSize[0] = w;
		m_volumeSize[1] = h;
		m_volumeSize[2] = d;
	}
	void setEigenModes ( mattools::RawMatrix X ) { m_eigenmodes = X; }
	void setEigenValues( Vector lambda )         { m_eigenvalues = lambda; }
	///@}

	/// Set start/end point of edit, invokes \a solve() internally.
	void setPickedPoints( RayPickingInfo s, RayPickingInfo t );

	///@{ Regularization parameter
	void   setGamma( double gamma )       { m_gamma = gamma; }
	double getGamma() const               { return m_gamma; }
	///@}

	/// Scaling of eigenmodes prior to setting up the linear system, must be 
	/// one of \a ScalingModes.
	void   setModeScaling( int mode )     { m_modeScaling = mode;	}
	int    getModeScaling() const         { return m_modeScaling; }
	void   setEditScale( double scale )   { m_editScale = scale; }
	double getEditScale() const           { return m_editScale; }
	void   setResultScale( double scale ) { m_resultScale = scale; }
	double getResultScale() const         { return m_resultScale; }

	///@{ Get least-squares solution
	Vector getCoeffs   () const { return m_copt;  }
	double getErrorData() const { return m_Edata; }
	double getErrorReg () const { return m_Ereg;  }
	Vector getDEdit    () const { return m_dedit; }
	Vector getDisplacement() const { return m_displacement; }
	///@}

	/// Get start/end points of last edit in normalized coordinates.
	void getStartEndPoints( VectorType& p, VectorType& q, 
	                        bool displace=true ) const;
	
	/// Solve, where start/end points v0/v1 given in normalized coordinates
	void solve( double* v0, double* v1 );
	/// Solve, where start/end points v0/v1 given in normalized coordinates
	void solve( VectorType v0, VectorType v1 );
	/// Solve, where v0 in normalized and d_edit in physical coordinates
	void solveEdit( double* v0, Vector d_edit );
	void solveEdit( VectorType v0, VectorType d_edit );
	
	/// The main solver
	void solve( IndexVector rows, Vector d_edit );

	/// Old variant supporting additional diagonal weights
	void solveWeighted( VectorType v0, VectorType v1 );

protected:
	void getSubMatrix( std::vector<size_t> rows, Matrix& B );
	void getReducedBasis( VectorType v0, Matrix& B );
	void getSystemMatrices( VectorType v0, Matrix& A, Matrix& B, Matrix& D  );

private:
	StatisticalDeformationModel* m_sdm;
	mattools::RawMatrix m_eigenmodes;
	Vector m_eigenvalues;
	int m_volumeSize[3];
	int m_modeScaling;
	Vector m_copt; ///< Last solution computed with solve()

	double m_gamma;
	double m_editScale;
	double m_resultScale;
	
	double m_Edata;
	double m_Ereg;
	Vector m_dedit;
	Vector m_displacement;

	VectorType m_pStart;
	VectorType m_pEnd;
};

#endif // SDMVISINTERACTIVEEDITING_H
