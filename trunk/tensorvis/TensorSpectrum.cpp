#include "TensorSpectrum.h"
#include <vtkMath.h>  // vtkMath::Jacobi(),::Normalize()
#include <algorithm>  // min(),max()
#include <cmath>      // sqrt()

void TensorSpectrum::compute( double* tensor9 )
{
	// The following code is partly taken from vtkTensorGlyph

	// set up temporary working matrices
	double *m[3], *v[3];
	double m0[3], m1[3], m2[3];
	double v0[3], v1[3], v2[3];
	m[0] = m0; m[1] = m1; m[2] = m2;
	v[0] = v0; v[1] = v1; v[2] = v2;
	for( int j=0; j<3; j++ )
		for( int i=0; i<3; i++ )
			m[i][j] = tensor9[i+3*j];

	// compute eigenvectors (v) and ~values (lambda) from psd matrix (m)
	vtkMath::Jacobi(m, lambda, v);

	// copy eigenvectors
	ev1[0] = v[0][0];  ev1[1] = v[1][0];  ev1[2] = v[2][0];
	ev2[0] = v[0][1];  ev2[1] = v[1][1];  ev2[2] = v[2][1];
	ev3[0] = v[0][2];  ev3[1] = v[1][2];  ev3[2] = v[2][2];
}

void TensorSpectrum::set( double* basis9 )
{
	for( int j=0; j < 3; j++ )
	{
		ev1[j] = basis9[0+3*j];
		ev2[j] = basis9[1+3*j];
		ev3[j] = basis9[2+3*j];
	}

	lambda[0] = vtkMath::Normalize(ev1);
	lambda[1] = vtkMath::Normalize(ev2);
	lambda[2] = vtkMath::Normalize(ev3);
}

// WORKAROUND: Simply copied above double* functions to take float* input!

void TensorSpectrum::compute( float* tensor9 )
{
	// The following code is partly taken from vtkTensorGlyph

	// set up temporary working matrices
	double *m[3], *v[3];
	double m0[3], m1[3], m2[3];
	double v0[3], v1[3], v2[3];
	m[0] = m0; m[1] = m1; m[2] = m2;
	v[0] = v0; v[1] = v1; v[2] = v2;
	for( int j=0; j<3; j++ )
		for( int i=0; i<3; i++ )
			m[i][j] = tensor9[i+3*j];

	// compute eigenvectors (v) and ~values (lambda) from psd matrix (m)
	vtkMath::Jacobi(m, lambda, v);

	// copy eigenvectors
	ev1[0] = v[0][0];  ev1[1] = v[1][0];  ev1[2] = v[2][0];
	ev2[0] = v[0][1];  ev2[1] = v[1][1];  ev2[2] = v[2][1];
	ev3[0] = v[0][2];  ev3[1] = v[1][2];  ev3[2] = v[2][2];
}

void TensorSpectrum::set( float* basis9 )
{
	for( int j=0; j < 3; j++ )
	{
		ev1[j] = basis9[0+3*j];
		ev2[j] = basis9[1+3*j];
		ev3[j] = basis9[2+3*j];
	}

	lambda[0] = vtkMath::Normalize(ev1);
	lambda[1] = vtkMath::Normalize(ev2);
	lambda[2] = vtkMath::Normalize(ev3);
}


// Return maximum length of eigenvectors projected onto plane with normal n
double TensorSpectrum::maxProjectedLen( const double n[3] ) const
{		
	// assume eigenvectors are normalized
	double sp1 = n[0]*ev1[0] + n[1]*ev1[1] + n[2]*ev1[2],
		   sp2 = n[0]*ev2[0] + n[1]*ev2[1] + n[2]*ev2[2],
		   sp3 = n[0]*ev3[0] + n[1]*ev3[1] + n[2]*ev3[2];

	// projections
	double p1[3], p2[3], p3[3];
	p1[0] = ev1[0]-sp1*n[0];  p1[1] = ev1[1]-sp1*n[1];  p1[2] = ev1[2]-sp1*n[2];
	p2[0] = ev2[0]-sp2*n[0];  p2[1] = ev2[1]-sp2*n[1];  p2[2] = ev2[2]-sp2*n[2];
	p3[0] = ev3[0]-sp3*n[0];  p3[1] = ev3[1]-sp3*n[1];  p3[2] = ev3[2]-sp3*n[2];

	// lengths
	double l1, l2, l3;
	l1 = sqrt(lambda[0]*(p1[0]*p1[0] + p1[1]*p1[1] + p1[2]*p1[2]));
	l2 = sqrt(lambda[1]*(p2[0]*p2[0] + p2[1]*p2[1] + p2[2]*p2[2]));
	l3 = sqrt(lambda[2]*(p3[0]*p3[0] + p3[1]*p3[1] + p3[2]*p3[2]));

	return std::min( l1, std::min( l2, l3 ) );
}

double TensorSpectrum::maxEigenvalue() const
{
	return lambda[0];
}

double TensorSpectrum::meanDiffusivity() const
{
	return (lambda[0]+lambda[1]+lambda[2]) / 3.;
}

double TensorSpectrum::fractionalAnisotropy() const
{
	double mu = meanDiffusivity();
	return sqrt(3./2.) * sqrt( 
							  ( (lambda[0]-mu)*(lambda[0]-mu) + 
							    (lambda[1]-mu)*(lambda[1]-mu) + 
							    (lambda[2]-mu)*(lambda[2]-mu) )
							/
							  ( lambda[0]*lambda[0] +
							    lambda[1]*lambda[1] +
								lambda[2]*lambda[2] )
						    );
}

double TensorSpectrum::frobeniusNorm() const 
{
	return 
	   sqrt( lambda[0]*lambda[0] + lambda[1]*lambda[1] + lambda[2]*lambda[2] );
}
