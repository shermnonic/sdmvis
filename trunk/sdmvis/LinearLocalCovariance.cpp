#include "LinearLocalCovariance.h"
#include "ImageDataSpace.h"
#include "StatisticalDeformationModel.h"
#include <iostream>
#include <iomanip>

LinearLocalCovariance::LinearLocalCovariance()
	: m_tensorType(CovarianceTensor),
	  m_sampleCovariance(NULL)
{
}

std::ostream& operator << ( std::ostream& os, const Matrix& M  )
{
	os << std::setprecision(5);
	for( int i=0; i < M.size1(); i++ )
	{
		for( int j=0; j < M.size2(); j++ )
			os << std::setw(12) << M(i,j);
		os << std::endl;
	}
	return os;
}

Matrix LinearLocalCovariance::
	getZp( double x, double y, double z )
{
	using namespace boost::numeric::ublas;	
	
	Matrix Bp;
	getReducedMatrix( x,y,z, Bp );
	
	// Inner system matrix  A = Bp'Bp + gamma
	Matrix A = prod( trans(Bp), Bp );
	for( unsigned i=0; i < A.size1(); i++ )
		A(i,i) = A(i,i) + getGamma();
	
	// Pseudo inverse
	Matrix Ainv;
	rednum::pseudo_inverse<Matrix,Vector,ValueType>( A, Ainv );

	// Precomputed part of linear co-variation depending only on reference
	// position p:
	//               Zpq = Bq (Bp'Bp + gamma)^(-1) Bp'
	//                   = Bq Zp
	return prod( Ainv, trans(Bp) );
}

void LinearLocalCovariance::
  setReferencePoint( double x, double y, double z )
{
	// Sanity check
	if( !getSDM() )
		return;

	m_referencePoint[0] = x;
	m_referencePoint[1] = y;
	m_referencePoint[2] = z;
	
	m_Zp = getZp( x, y, z );

#if 0
	// Sanity checks
	using std::cout;
	using std::endl;
	cout << "A = " << endl << A << endl;
	cout << "A * Ainv = " << endl << prod(A,Ainv) << endl;
	cout << "Ainv * A = " << endl << prod(Ainv,A) << endl;
	cout << "gamma = " << m_gamma << endl;
	cout << "Zp = " << endl << m_Zp << endl;

	char filename[255];
	sprintf(filename,"temp_Zp_gamma-%010.4f.mat",this->m_gamma);
	rednum::save_matrix<float,Matrix>( m_Zp, filename );
#endif

}

void LinearLocalCovariance::
	getReducedMatrix( double x, double y, double z, Matrix& B )
{
	// Get vectorfield matrix row indices corresponding to (x,y,z)
	StatisticalDeformationModel::IndexVector rows;
	getSDM()->appendRowIndices( x, y, z, rows );

	// Buffer for the sub-matrix B with rows corresponding to (x,y,z)
	mattools::ValueType* buf;
	buf = new mattools::ValueType[ getSDM()->getRowSize(rows) ];
	
	getSDM()->getEigenmodeRows( rows, buf );
	
	B = Matrix( rows.size(), getSDM()->getRowSize(rows) / rows.size() );
	rednum::matrix_from_rawbuffer<
		mattools::ValueType, Matrix, ValueType>( B, buf, true );
	
	delete [] buf;	
}

void LinearLocalCovariance::
	getReducedMatrix( double* v, Matrix& B )
{
	getReducedMatrix( v[0], v[1], v[2], B );
}


Matrix prod3( Matrix A, Matrix B, Matrix C )
{
	Matrix T = prod(B,C);
	return prod(A,T);
}

void LinearLocalCovariance::
  getTensor( double x_, double y_, double z_, float (&tensor3x3)[9] )
{
	using namespace boost::numeric::ublas;	

	// Normalize (x,y,z) to conform to SDM format
	double x=x_, y=y_, z=z_;
	getSDM()->getHeader().normalizeCoordinates( x, y, z );

	// WORKAROUND: For sample point generation getTensor() also may be called
	// (without prior specification of any valid point p) to check for close to
	// zero tensors. As long as we have some kind of sample covariance data this
	// is O.K. since we can safely return that tensor to estimate empty areas
	// in our domain. See also TensorDataProvider::isValidSamplePoint().
	if( m_Zp.size1()==0 || m_Zp.size2()==0 )
	{
		if( m_sampleCovariance )
		{
			m_sampleCovariance->getTensor( x_, y_, z_, tensor3x3 );
			return;
		}
		else
		{
			//cout << " LinearLocalCovariance::getTensor() called without reference point "
			//	"and invalid sampleCovariance pointer! Returning some default tensor "
			//	"in this case!" << endl;
			for( int i=0; i < 9; i++ )
				tensor3x3[i] = (i%4)==0 ? 1.0 : 0.0; // Return diagonal matrix
			return;

			//was: assert( false );
		}
	}

	// Zpq = Bq (Bp'Bp + gamma)^(-1) Bp'
	//     = Bq Zp
	Matrix Bq;
	getReducedMatrix( x,y,z, Bq );
	Matrix Zpq = prod( Bq, m_Zp );

	// Sample covariance matrix
	float sigma3x3[9] = { // Identity by default
		1.f, 0.f, 0.f,
		0.f, 1.f, 0.f,
		0.f, 0.f, 1.f };
	if( m_sampleCovariance )
	{
		// Use user specified sample covariance
		m_sampleCovariance->getTensor( x_, y_, z_, sigma3x3 );
	}
	Matrix Sigma(3,3);
	rednum::matrix_from_rawbuffer<float,Matrix,double>( Sigma, (float*)sigma3x3 );
	
	// Compute tensor
	if( m_tensorType == CovarianceTensor )
	{
		// Zpq is the complete linear relationship between displacements at
		// positions p and q, so this is what we want to visualize.
		// The canonical symmetric tensor for Zpq is Zpq*Zpq' = UDU',
		// although more direct visualization techniques could be employed.	
	
		Matrix T;
		if( m_sampleCovariance )
			// Weight with sample covariance
			T = prod3( Zpq, Sigma, trans(Zpq) );
		else
			// Avoid unecessary multiplication with identity
			T = prod( Zpq, trans(Zpq) );
		rednum::matrix_to_rawbuffer( T, tensor3x3 );
	}
	else 
	if( m_tensorType == InteractionOperator )
	{
		// Return Zpq
		rednum::matrix_to_rawbuffer( Zpq, tensor3x3 );
	}
	else
	if( m_tensorType == IntegralTensor )
	{
		// Tensor used for statistical integration of tensorfield
		Matrix T;
		if( m_sampleCovariance )
			// Weight with sample covariance
			T = prod3( trans(Zpq), Sigma, Zpq );
		else
			// Avoid unecessary multiplication with identity
			T = prod( trans(Zpq), Zpq );
		rednum::matrix_to_rawbuffer( T, tensor3x3 );
	}
	else
	if( m_tensorType == SelfInteractionTensor )
	{
		// Return Tq(q)
		Matrix Zq = getZp( x, y, z );
		Matrix Zqq = prod( Bq, Zq );
		Matrix T = prod( Zqq, trans(Zqq) );
		rednum::matrix_to_rawbuffer( T, tensor3x3 );
	}
	else
	if( m_tensorType == SelfInteractionOperator )
	{
		// Return Zqq
		Matrix Zq = getZp( x, y, z );
		Matrix Zqq = prod( Bq, Zq );
		rednum::matrix_to_rawbuffer( Zqq, tensor3x3 );
	}
	else
	{
		std::cerr << "Error: Unknown local covariance tensor type!\n";
		assert(false);
	}
}

void LinearLocalCovariance::
  getCoefficients( Vector edit, Vector& coeffs )
{
	using namespace boost::numeric::ublas;	

	assert(edit.size()==3);
	// Assume that edit is already in SDM coordinate system

	// coeffs = (Bp'Bp + gamma)^(-1) Bp' * edit
	//        = Zp * edit
	coeffs = prod( m_Zp, edit );
}
