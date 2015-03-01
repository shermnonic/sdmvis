#include "WeightedTensorDataProvider.h"
#include "TensorSpectrum.h"

#include "numerics.h"

//template<class T>
//	multiply_matrix_3x3( const T* A, const T* B, T* AB )
//{
//	// Assume row-major addressing
//	unsigned ofs=0; 
//	for( unsigned i=0; i < 3; i++ )  // result rows i
//		for( unsigned j=0; j < 3; j++, ofs++ )  // result columns j
//		{
//			AB[ofs]
//		}
//}

void WeightedTensorDataProvider
  ::getTensor( double x, double y, double z, float (&tensor3x3)[9] )
{
	using namespace boost::numeric::ublas;
	using namespace rednum;
	
	// Sanity checks
	if( !m_weights ) {
		if( !m_tdata ) return;		
		return m_tdata->getTensor( x,y,z, tensor3x3 );
	}
	
	float Tbuf[9];  // The "inner" tensor
	float Wbuf[9];  // The "outer" weighting tensor
	
	m_tdata  ->getTensor( x,y,z, Tbuf );
	m_weights->getTensor( x,y,z, Wbuf );

#if 1
	// Weight T with as W'*T*W

	Matrix T(3,3), W(3,3);
	matrix_from_rawbuffer<float,Matrix,rednum::ValueType>( T, Tbuf );
	matrix_from_rawbuffer<float,Matrix,rednum::ValueType>( W, Wbuf );

	Matrix WTW = prod( T, W );
	       WTW = prod( trans(W), WTW );

	// Return weighted tensor
	matrix_to_rawbuffer<float,Matrix>( WTW, tensor3x3 );
#else
	
	// Spectral decomposition of "outer" tensor W = R*D
	float Rbuf[9];
	TensorSpectrum ws;
	ws.compute( Wbuf );
	ws.getRotation<float>( Rbuf ); 
	
	// Assemble rotation and diagonal sqrt(eigenvalue) matrix
	Matrix R(3,3);
	Matrix D(3,3);
	
	matrix_from_rawbuffer<float,Matrix,rednum::ValueType>( R, Rbuf );
	
	for( unsigned i=0; i < 3; i++ )
		for( unsigned j=0; j < 3; j++ )
			D(i,j) = 0.;
	D(0,0) = sqrt( ws.lambda[0] );
	D(1,1) = sqrt( ws.lambda[1] );
	D(2,2) = sqrt( ws.lambda[2] );
	
	// Perform outer weighting as T -> D*R'*T*R*D
	Matrix T(3,3);
	matrix_from_rawbuffer<float,Matrix,rednum::ValueType>( T, Tbuf );

	Matrix RD    = prod( R, D );
	Matrix TRD   = prod( T, RD );
	Matrix DRTRD = prod( trans(RD), TRD );
	
	// Return weighted tensor
	matrix_to_rawbuffer<float,Matrix>( DRTRD, tensor3x3 );
#endif
}
