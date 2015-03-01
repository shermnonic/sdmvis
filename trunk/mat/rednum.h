/*
rednum - template mini matrix library
=====================================
Max Hermann, March 10, 2011

Provides:
- wrapper for Numerical Recipes SVD code
- simple disk IO for raw matrix files

Note that some functions rely on boost::numerics::ublas.

Function overview:
------------------
//	mean / centering / normalization
Vector col_mean( const Matrix& M );
Vector row_mean( const Matrix& M );
Vector center_cols( Matrix& M );
Vector center_rows( Matrix& M );
void normalize_columns( Matrix& M );
//	io
void save_matrix( const Matrix& M, const char* filename, bool row_major=true );
void save_vector( const Vector& v, const char* filename );
bool load_matrix( Matrix& M, const char* filename, bool row_major=true );
//	conversion
T*   matrix_to_rawbuffer( const Matrix& M, bool row_major=true )
void matrix_from_rawbuffer( Matrix& M, T* buf, bool row_major=true )
T*   vector_to_rawbuffer( const Vector& v )
void vector_from_rawbuffer( Vector& v, T* buf )
//  svd / pcacov
void compute_svd( const Matrix& M, Matrix& U, Vector& s, Matrix& V );
void compute_pcacov( const Matrix& M, Matrix& U, Matrix& C, Vector& s,
                      bool normalizeColumns=false, bool quiet=true );

*/
#ifndef REDNUM_H
#define REDNUM_H

#ifndef NOMINMAX
#define NOMINMAX
#endif

#pragma warning(disable: 4267)

#include <algorithm>  // for std::min, std::max
#include <vector>
#include <limits>
#include <cmath>

#include <assert.h>

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/banded.hpp>

//linux/unix specific 
#ifndef WIN32
	#include <stdio.h>
	#include <stdlib.h>
	#include <istream>
	#include <fstream>
	#include <iostream>
	// used vector and matrix types
	#include <boost/numeric/ublas/matrix.hpp>
	#include <boost/numeric/ublas/vector.hpp>
	typedef double ValueType;
	typedef boost::numeric::ublas::matrix<ValueType> Matrix;
	typedef boost::numeric::ublas::vector<ValueType> Vector; 
#endif

//==============================================================================
//  Template mini matrix library (partly based on ublas)
//==============================================================================

namespace rednum {

//------------------------------------------------------------------------------
//	mean / centering / normalization
//------------------------------------------------------------------------------
	
/// Vector of matrix column means
template<class Matrix,class Vector>
inline Vector col_mean( const Matrix& M )
{
	// implementation without the use of row or column proxies
	Vector mu( M.size2() );
	for( unsigned int j=0; j < M.size2(); ++j )
	{
		mu(j) = 0;
		for( unsigned int i=0; i < M.size1(); ++i )
			mu(j) += M(i,j);
	}
	mu *= 1./M.size1();
	return mu;
}

/// Vector of matrix row means
template<class Matrix,class Vector>
inline Vector row_mean( const Matrix& M )
{
	// implementation without the use of row or column proxies
	Vector mu( M.size1() );	
	for( unsigned int i=0; i < M.size1(); ++i )
	{
		mu(i) = 0;
		for( unsigned int j=0; j < M.size2(); ++j )
			mu(i) += M(i,j);
	}
	mu *= 1./M.size2();
	return mu;
}

/// Center matrix columns
template<class Matrix,class Vector>
inline Vector center_cols( Matrix& M )
{
	Vector trans;
	trans = col_mean<Matrix,Vector>( M );
	for( unsigned int i=0; i < M.size1(); ++i )
		for( unsigned int j=0; j < trans.size(); ++j )
			M(i,j) -= trans(j);
	return trans;
}

/// Center matrix rows
template<class Matrix,class Vector>
inline Vector center_rows( Matrix& M )
{	
	Vector trans;
	trans = row_mean<Matrix,Vector>( M );
	for( unsigned int j=0; j < M.size2(); ++j )
		for( unsigned int i=0; i < trans.size(); ++i )
			M(i,j) -= trans(i);
	return trans;	
}

/// Normalize matrix columns (uses ublas functionality)
template<class Matrix>
void normalize_columns( Matrix& M )
{	
	for( unsigned int j=0; j < M.size2(); ++j )
	{
		boost::numeric::ublas::matrix_column<Matrix> v( M, j );
		
		// norm
		double norm = 0.;
		for( unsigned int i=0; i < v.size(); ++i )
			norm += v(i)*v(i);
		norm = sqrt(norm);
		
		// normalize
		for( unsigned int i=0; i < v.size(); ++i )
			v(i) = v(i) / norm;
	}
}


//------------------------------------------------------------------------------
//	io
//------------------------------------------------------------------------------

/// Save matrix to a binary file
/// This function copies the data to allow casting operations.
/// \tparam T template parameter is the output value type (as stored in the file)
/// \param row_major indicates if the file is stored in row major or column major
///        (e.g. Matlab stores column major)
template<class T,class Matrix>
void save_matrix( const Matrix& M, const char* filename, bool row_major=true )
{
	using namespace std;
	
	unsigned m = M.size1(),
	         n = M.size2();
	
	// copy ublas matrix to plain buffer
	T* buf = new T[m*n];  						// TODO: catch memory exception
	for( unsigned i=0; i < m; ++i )
		for( unsigned j=0; j < n; ++j )
		{
			unsigned ofs = row_major ? (i * n + j) : (i + j * m);
			buf[ofs] = (T) M(i,j);   // explicit cast	
		}
	
	// save to binary file
	ofstream of( filename, ios::binary );	
	of.write( (char*)buf, m*n*sizeof(T) );
	
	// clean up
	of.close();
	delete [] buf;	
}

/// Save vector to a binary file
template<class T,class Vector>
void save_vector( const Vector& v, const char* filename )
{
	// copy into matrix and use save_matrix() function
	Matrix M( v.size(), 1, v.data() );	
	save_matrix<T>( M, filename );	
}

/// Load matrix from binary file
/// This function copies the data to allow casting operations.
/// template paramter T is input value type
/// assume matrix is already resized to expected input!
/// \tparam T template parameter is the input value type (as stored in the file)
/// \param row_major indicates if the file is stored in row major or column major
///        (e.g. Matlab stores column major)
template<class T,class Matrix,class ValueType>
bool load_matrix( Matrix& M, const char* filename, bool row_major=true )
{
	using namespace std;

	// assume matrix is already resized to expected input!
	unsigned m = M.size1(),
		     n = M.size2();

	// open file
	ifstream f( filename, ios::binary );
	if( !f.is_open() )
	{
		cerr << "Error: Could not open \"" << filename << "\"!" << endl;
		return false;
	}
	
	// get file size
	f.seekg( 0, ios::end );
	size_t fsize = f.tellg();
	f.seekg( 0, ios::beg );
	
	// assert correct size
	if( (fsize/sizeof(T)) != m*n )
	{
		cerr << "Error: Mismatching matrix and file size \"" << filename << "\"!" << endl;
		return false;		
	}
	
	// read whole file into buffer
	T* buf = new T[m*n];
	f.read( (char*)buf, fsize ); // fsize == m*n*sizeof(T)s	
		
	// copy plain buffer to ublas matrix
	for( unsigned i=0; i < m; ++i )
		for( unsigned j=0; j < n; ++j )
		{
			unsigned ofs = row_major ? (i * n + j) : (i + j * m);
			M(i,j) = (ValueType) buf[ofs];   // explicit cast
		}	
		
	// clean up
	f.close();
	delete [] buf;
	return true;
}

template<class T,class Vector,class ValueType,class Matrix>
bool load_vector( Vector& v, const char* filename )
{
	Matrix M( v.size(), 1 );
	if( load_matrix<T,Matrix,ValueType>( M, filename ) )
	{
		for( int i=0; i < v.size(); ++i )
			v[i] = M(i,0);
		return true;
	}
	return false;
}


//------------------------------------------------------------------------------
//	conversion
//------------------------------------------------------------------------------

/// Allocate raw buffer and copy matrix into it
template<class T,class Matrix>
T* matrix_to_rawbuffer( const Matrix& M, bool row_major=true )
{
	unsigned m = M.size1(),
		     n = M.size2();

	// allocate buffer	
	T* buf = new T[m*n];

	// copy plain buffer to ublas matrix
	for( unsigned i=0; i < m; ++i )
		for( unsigned j=0; j < n; ++j )
		{
			unsigned ofs = row_major ? (i * n + j) : (i + j * m);
			buf[ofs] = (T)M(i,j) ;   // explicit cast
		}

	return buf;
}

/// Copy matrix into pre-allocated raw buffer
template<class T,class Matrix>
void matrix_to_rawbuffer( const Matrix& M, T* buf, bool row_major=true )
{
	unsigned m = M.size1(),
		     n = M.size2();

	// copy plain buffer to ublas matrix
	for( unsigned i=0; i < m; ++i )
		for( unsigned j=0; j < n; ++j )
		{
			unsigned ofs = row_major ? (i * n + j) : (i + j * m);
			buf[ofs] = (T)M(i,j) ;   // explicit cast
		}
}

template<class T,class Matrix>
void matrix_to_stdvector( const Matrix& M, std::vector<T>& out, bool row_major=true )
{
	unsigned m = M.size1(),
		     n = M.size2();

	// allocate buffer	
	out.resize( m*n );

	// copy plain buffer to ublas matrix
	for( unsigned i=0; i < m; ++i )
		for( unsigned j=0; j < n; ++j )
		{
			unsigned ofs = row_major ? (i * n + j) : (i + j * m);
			out[ofs] = (T)M(i,j) ;   // explicit cast
		}
}


/// Copy matrix from memory
/// This function copies the data to allow casting operations.
/// template paramter is input value type
/// assume matrix is already resized to expected input!
template<class T,class Matrix,class ValueType>
void matrix_from_rawbuffer( Matrix& M, T* buf, bool row_major=true )
{
	// assume matrix is already resized to expected input!
	unsigned m = M.size1(),
		     n = M.size2();
	
	// copy plain buffer to ublas matrix
	for( unsigned i=0; i < m; ++i )
		for( unsigned j=0; j < n; ++j )
		{
			unsigned ofs = row_major ? (i * n + j) : (i + j * m);
			M(i,j) = (ValueType) buf[ofs];   // explicit cast
		}		
}

/// Allocate raw buffer and copy vector into it
template<class T,class Vector>
T* vector_to_rawbuffer( const Vector& v )
{
	unsigned n = v.size();

	// allocate buffer	
	T* buf = new T[n];

	// copy plain buffer to ublas matrix
	for( unsigned i=0; i < n; ++i )
		buf[i] = (T)v(i) ;   // explicit cast

	return buf;
}

/// Copy vector from memory
/// This function copies the data to allow casting operations.
/// template paramter is input value type
/// assume vector is already resized to expected input!
template<class T,class Vector,class ValueType>
void vector_from_rawbuffer( Vector& v, T* buf )
{
	// assume matrix is already resized to expected input!
	unsigned n = v.size();
	
	// copy plain buffer to ublas vector
	for( unsigned i=0; i < n; ++i )
		v(i) = (ValueType) buf[i];   // explicit cast
}


	
//------------------------------------------------------------------------------
//  svd / pcacov
//------------------------------------------------------------------------------	
	
/// Compute Singular Value Decomposition
/// (simple	wrapper for super-old Numerical Recipes code)
template<class Matrix,class Vector,class ValueType>
class SVD
{
public:
	//typedef boost::numeric::ublas::matrix<ValueType> Matrix;
	//typedef boost::numeric::ublas::vector<ValueType> Vector;

	/// \brief Compute Singular Value Decomposition
	/// Compute the singular value decomposition of a matrix A into A=UDV'.
	/// \param[in,out] matrix row-major matrix, gets overwritten by matrix U of decomposition
	/// \note Input matrix will be destroyed!
	SVD( Matrix& matrix );

	/// Return singular values sorted from largest to smallest.
	/// \return singular values sorted from largest to smallest
	const Vector& getSingularValues() const { return m_S; }
	
	/// Return matrix V of decomposition (this is really V and not V transposed).
	/// \return matrix V of decomposition
	const Matrix& getV() const { return m_V; }
	
private:
	int m_rows, m_cols;
	Vector  m_S;
	Matrix  m_V;
};

/// Singular value decomposition
template<class Matrix,class Vector,class ValueType>
void compute_svd( const Matrix& M, Matrix& U, Vector& s, Matrix& V )
{
	U = M;
	SVD<Matrix,Vector,ValueType> svd(U);
	s = svd.getSingularValues();
	V = svd.getV();
}

/// Principal component analysis on covariance matrix
/// \param[out] U are the eigenvectors of M
/// \param[out] C are the coefficients / scores
/// \param[out] s are the eigenvalues / PC variances
template<class Matrix,class Vector,class ValueType>
void compute_pcacov( const Matrix& M, Matrix& U, Matrix& C, Vector& s,
                      bool normalizeColumns=false, bool quiet=true )
{
	compute_svd<Matrix,Vector,ValueType>( M, U, s, C );

	// print explained variance for first 10 PC's
	ValueType total_var = 0.0;
	for( size_t i=0; i < s.size(); ++i )
		total_var += s[i];
	
	if(!quiet) {
		std::cout << "Explained Variance:" << std::endl;
		for( int i=0; i < std::min((int)s.size(),10); ++i )
			std::cout << "["<<i<<"]" << 100.0*s[i]/total_var << "%" << std::endl;
		std::cout << "First five eigenvalues:" << std::endl;
		for( int i=0; i < std::min((int)s.size(),10); ++i )
			std::cout << s[i] << std::endl;
	}
}

/// Solve Ax=b least squares via Penrose inverse
/// \param[in]  A system matrix
/// \param[in]  b right hand side
/// \param[out] x least squares solution to Ax=b
template<class Matrix,class Vector,class ValueType>
void solve_ls( const Matrix& A, const Vector& b, Vector& x )
{
	using namespace boost::numeric::ublas;

	Matrix U,V;
	Vector sigma;
	compute_svd<Matrix,Vector,ValueType>( A, U, sigma,V );

	ValueType eps = 1e-12;

	// Estimate number of leading non-zero entries in sigma
	unsigned nz;
	for( nz=0; nz < sigma.size() && sigma[nz]>eps; nz++ );

	// Invert singular values
	Vector sigma_inverse(nz);
	for( unsigned i=0; i < nz; i++ )
		sigma_inverse[i] = 1.0 / sigma[i];

	// Setup reduced matrices
	U.resize( U.size1(), nz );
	V.resize( V.size1(), nz );

	// Compute x = V * diag(sigma_inverse) * trans(U) * b
	unsigned n = A.size2(); // == b.size1();
	x = prod( trans(U), b );
	for( unsigned i=0; i < n; i++ )
		x[i] = x[i] * sigma_inverse[i];
	x = prod( V, x ); // x = V * x	
}

template<class Matrix,class Vector,class ValueType>
void pseudo_inverse( const Matrix& A, Matrix& Ainv, ValueType eps=1e-12 )
{
	using namespace boost::numeric::ublas;

	Matrix U,V;
	Vector sigma;
	compute_svd<Matrix,Vector,ValueType>( A, U, sigma,V );

	// Estimate number of leading non-zero entries in sigma
	unsigned nz;
	for( nz=0; nz < sigma.size() && sigma[nz]>eps; nz++ );

	// Invert singular values
	Vector sigma_inverse(nz);
	for( unsigned i=0; i < nz; i++ )
		sigma_inverse[i] = 1.0 / sigma[i];

#if 1
	// Debug
	save_matrix<float,Matrix>( U, "temp_svd_U.mat" );
	save_matrix<float,Matrix>( V, "temp_svd_V.mat" );
	save_vector<float,Vector>( sigma, "temp_svd_lambda.mat" );
#endif

	// Setup reduced matrices
	U.resize( U.size1(), nz );
	V.resize( V.size1(), nz );
	
	// Compute x = V * diag(sigma_inverse) * trans(U)
#if 1
	diagonal_matrix<ValueType> SigmaInv( sigma_inverse.size(), sigma_inverse.data() );
#else
	//banded_matrix<ValueType> SigmaInv( sigma_inverse.size(), sigma_inverse.size(), 0,0 );
	Matrix SigmaInv( sigma_inverse.size(), sigma_inverse.size() );
	for( unsigned i=0; i < sigma_inverse.size(); i++ )
		SigmaInv(i,i) = sigma_inverse[i];
#endif
	//Ainv = prod( V, prod( SigmaInv, trans(U) ) ); // nested prod()'s not allowed!
	Ainv = prod<Matrix>( SigmaInv, trans(U) );
	Ainv = prod( V, Ainv );
}

} // namespace rednum



//==============================================================================
//	SVD implementation (anonymous helper functions + rednum::SVD c'tor)
//==============================================================================
namespace {  // use anonymous namespace to avoid namespace pollution
	
template< class T >
T Pythag(T a, T b){
	T absa, absb;
	absa = fabs(a);
	absb = fabs(b);
	if(absa > absb)
		return absa * sqrt(T(1) + (absb / absa) * (absb / absa));
	return (absb == 0)? 0 :
		(absb * sqrt(T(1) + (absa / absb) * (absa / absb)));
}

/// Given a matrix a[0..m-1][0..n-1], this routine computes its singular value 
/// decomposition, A = U · W · V^T. The matrix U replaces a on output. 
/// The diagonal matrix of singular values W is output as a vector w[0..n-1]. 
/// The matrix V (not the transpose V^T) is output as v[0..n-1][0..n-1].
template< class Matrix, class Vector, class T >
bool svdcmp(Matrix *u, Vector *w, Matrix *v)
{
#define SIGN(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))
#ifndef min
	using std::min;
#endif
#ifndef max
	using std::max;
#endif
	assert(u); assert(w); assert(v);
	int flag,i,its,j,jj,k,l,nm;
	T anorm,c,f,g,h,s,scale,x,y,z;
	Vector rv1 = *w;
	int M = (int)u->size1();
	int N = (int)u->size2();

	g=scale=anorm=0.0;
	for (i=0;i<N;i++) {
		l=i+1;
		rv1(i)=scale*g;
		g=s=scale=0.0;
		if (i < M) {
			for (k=i;k<M;k++) scale += fabs((*u)(k,i));
			if (scale) {
				for (k=i;k<M;k++) {
					(*u)(k,i) /= scale;
					s += (*u)(k,i)*(*u)(k,i);
				}
				f=(*u)(i,i);
				g = -SIGN(sqrt(s),f);
				h=f*g-s;
				(*u)(i,i)=f-g;
				for (j=l;j<N;j++) {
					for (s=0.0,k=i;k<M;k++) s += (*u)(k,i)*(*u)(k,j);
					f=s/h;
					for (k=i;k<M;k++) (*u)(k,j) += f*(*u)(k,i);
				}
				for (k=i;k<M;k++) (*u)(k,i) *= scale;
			}
		}
		(*w)(i)=scale *g;
		g=s=scale=0.0;
		if (i < M && i != N - 1) {
			for (k=l;k<N;k++) scale += fabs((*u)(i,k));
			if (scale) {
				for (k=l;k<N;k++) {
					(*u)(i,k) /= scale;
					s += (*u)(i,k)*(*u)(i,k);
				}
				f=(*u)(i,l);
				g = -SIGN(sqrt(s),f);
				h=f*g-s;
				(*u)(i,l)=f-g;
				for (k=l;k<N;k++) rv1(k)=(*u)(i,k)/h;
				for (j=l;j<M;j++) {
					for (s=0.0,k=l;k<N;k++) s += (*u)(j,k)*(*u)(i,k);
					for (k=l;k<N;k++) (*u)(j,k) += s*rv1(k);
				}
				for (k=l;k<N;k++) (*u)(i,k) *= scale;
			}
		}
		anorm=std::max<T>(anorm,(T)(fabs((*w)(i))+fabs(rv1(i))));
	}
	for (i=N-1;i>=0;i--) {
		if (i < N-1) {
			if (g) {
				for (j=l;j<N;j++)
					(*v)(j,i)=((*u)(i,j)/(*u)(i,l))/g;
				for (j=l;j<N;j++) {
					for (s=0.0,k=l;k<N;k++) s += (*u)(i,k)*(*v)(k,j);
					for (k=l;k<N;k++) (*v)(k,j) += s*(*v)(k,i);
				}
			}
			for (j=l;j<N;j++) (*v)(i,j)=(*v)(j,i)=0.0;
		}
		(*v)(i,i)=1.0;
		g=rv1(i);
		l=i;
	}
	for (i=min((int)M-1, (int)N-1);i>=0;i--) {
		l=i+1;
		g=(*w)(i);
		for (j=l;j<N;j++) (*u)(i,j)=0.0;
		if (g) {
			g=T(1)/g;
			for (j=l;j<N;j++) {
				for (s=0.0,k=l;k<M;k++) s += (*u)(k,i)*(*u)(k,j);
				f=(s/(*u)(i,i))*g;
				for (k=i;k<M;k++) (*u)(k,j) += f*(*u)(k,i);
			}
			for (j=i;j<M;j++) (*u)(j,i) *= g;
		} else for (j=i;j<M;j++) (*u)(j,i)=0.0;
		++(*u)(i,i);
	}
	for (k=N-1;k>=0;k--) {
		for (its=1;its<=100;its++) {
			flag=1;
			for (l=k;l>=0;l--) {
				nm=l-1;
				if ((T)(fabs(rv1(l))+anorm) == anorm) {
					flag=0;
					break;
				}
				if ((T)(fabs((*w)(nm))+anorm) == anorm) break;
			}
			if (flag) {
				c=0.0;
				s=1.0;
				for (i=l;i<=k;i++) {
					f=s*rv1(i);
					rv1(i)=c*rv1(i);
					if ((T)(fabs(f)+anorm) == anorm) break;
					g=(*w)(i);
					h=Pythag(f,g);
					(*w)(i)=h;
					h=T(1)/h;
					c=g*h;
					s = -f*h;
					for (j=0;j<M;j++) {
						y=(*u)(j,nm);
						z=(*u)(j,i);
						(*u)(j,nm)=y*c+z*s;
						(*u)(j,i)=z*c-y*s;
					}
				}
			}
			z=(*w)(k);
			if (l == k) {
				if (z < 0.0) {
					(*w)(k) = -z;
					for (j=0;j<N;j++) (*v)(j,k) = -(*v)(j,k);
				}
				break;
			}
			if (its == 100) return false;
			x=(*w)(l);
			nm=k-1;
			y=(*w)(nm);
			g=rv1(nm);
			h=rv1(k);
			f=((y-z)*(y+z)+(g-h)*(g+h))/(T(2)*h*y);
			g=Pythag(f,T(1));
			f=((x-z)*(x+z)+h*((y/(f+SIGN(g,f)))-h))/x;
			c=s=1.0;
			for (j=l;j<=nm;j++) {
				i=j+1;
				g=rv1(i);
				y=(*w)(i);
				h=s*g;
				g=c*g;
				z=Pythag(f,h);
				rv1(j)=z;
				c=f/z;
				s=h/z;
				f=x*c+g*s;
				g = g*c-x*s;
				h=y*s;
				y *= c;
				for (jj=0;jj<N;jj++) {
					x=(*v)(jj,j);
					z=(*v)(jj,i);
					(*v)(jj,j)=x*c+z*s;
					(*v)(jj,i)=z*c-x*s;
				}
				z=Pythag(f,h);
				(*w)(j)=z;
				if (z) {
					z=T(1)/z;
					c=f*z;
					s=h*z;
				}
				f=c*g+s*y;
				x=c*y-s*g;
				for (jj=0;jj<M;jj++) {
					y=(*u)(jj,j);
					z=(*u)(jj,i);
					(*u)(jj,j)=y*c+z*s;
					(*u)(jj,i)=z*c-y*s;
				}
			}
			rv1(l)=0.0;
			rv1(k)=f;
			(*w)(k)=x;
		}
	}
	return true;
#undef SIGN
}

template<class Matrix,class Vector> void swap_columns( Matrix& M, int i, int j )
{
	using namespace boost::numeric::ublas;
	typedef matrix_column<Matrix> column;
#if 0
	column tmp = column(M,i);
	column(M,i) = column(M,j);
	column(M,j) = tmp;
#else
	column cj( M, j );
	column ci( M, i );
	Vector tmp = ci;
	ci = cj;
	cj = tmp;
#endif
}

} // -- end of anonymous namespace

namespace rednum {
// class template implementation
template<class Matrix,class Vector,class T>
SVD<Matrix,Vector,T>::SVD( Matrix& matrix )
	: m_rows( matrix.size1() ), 
	  m_cols( matrix.size2() ),
	  m_S( m_cols ),
	  m_V( m_cols, m_cols )
{
	svdcmp<Matrix,Vector,T>( &matrix, &m_S, &m_V );	
#if 1
	// BOGO BUG: sorting of eigenvalues is not correct (-> hint that i f***ed up the code?)
	// use simple bubble sort because i only observed one fehlstand
	for( unsigned int run=0; run < m_S.size(); ++run )
	{
		bool ordered = true;		
		for( unsigned int i=0; i < m_S.size()-1; ++i )
		{
			if( m_S[i] < m_S[i+1] )
			{
//std::cout << "Redukt::SVD<> Debug: Fehlstand found in eigenvalues!" << std::endl;				
				T tmp = m_S[i];
				m_S[i] = m_S[i+1];
				m_S[i+1] = tmp;
				
				swap_columns<Matrix,Vector>( matrix, i, i+1 );
				swap_columns<Matrix,Vector>( m_V,    i, i+1 );				
				ordered = false;
			}
		}		
		if( ordered ) break;		
	}
#endif	
}
} // namespace rednum

#endif // REDNUM_H
