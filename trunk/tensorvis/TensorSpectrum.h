#ifndef TENSORSPECTRUM_H
#define TENSORSPECTRUM_H

/// Computes spectral decomposition of a symmetric 3x3 tensor.
/// Initially this was a helper class for \a TensorVisBase.
struct TensorSpectrum
{
	void compute( double* tensor9 );
	void set( double* basis9 );

	// Provided for convenience
	void compute( float* tensor9 );
	void set( float* basis9 );

	/// Return maximum length of eigenvectors projected onto plane with normal n
	double maxProjectedLen( const double n[3] ) const;

	double maxEigenvalue() const;
	double meanDiffusivity() const;
	double fractionalAnisotropy() const;
	double frobeniusNorm() const;

	// Provided for convenience
	template<class T> void getRotation( T* M3x3, bool row_major = true )
	{
		// Put eigenvectors into columns
		if( row_major )
		{
			M3x3[0]=ev1[0];	 M3x3[1]=ev2[0];  M3x3[2]=ev3[0];
			M3x3[3]=ev1[1];	 M3x3[4]=ev2[1];  M3x3[5]=ev3[1];
			M3x3[6]=ev1[2];	 M3x3[7]=ev2[2];  M3x3[8]=ev3[2];
		}
		else
		{
			// FIXME: Column major *not* tested!
			M3x3[0]=ev1[0];	 M3x3[3]=ev2[0];  M3x3[6]=ev3[0];
			M3x3[1]=ev1[1];	 M3x3[4]=ev2[1];  M3x3[7]=ev3[1];
			M3x3[2]=ev1[2];	 M3x3[5]=ev2[2];  M3x3[8]=ev3[2];
		}
	}
	
	double ev1[3], ev2[3], ev3[3]; ///< eigenvectors
	double lambda[3];              ///< eigenvalues	
};


#endif // TENSORSPECTRUM_H
