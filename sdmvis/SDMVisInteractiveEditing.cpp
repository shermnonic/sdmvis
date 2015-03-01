#include "SDMVisInteractiveEditing.h"
#include <cmath>
#include <cstdio>
#include <vector>

typedef SDMVisInteractiveEditing::VectorType VectorType;

//-----------------------------------------------------------------------------
//  Vector helper functions
//-----------------------------------------------------------------------------

float scalar_prod( VectorType v, VectorType w )
{
	return (float)QVector3D::dotProduct( v, w );
}

VectorType normalized( VectorType v )
{
	return v.normalized();
}

VectorType getIntersection( RayPickingInfo info )
{
	float v[3];
	info.getIntersection( v );
	return VectorType( v[0], v[1], v[2] );
}

VectorType getNormal( RayPickingInfo info )
{
	float v[3];
	info.getNormal( v );
	return VectorType( v[0], v[1], v[2] );
}

VectorType getRayStart( RayPickingInfo info )
{
	float v[3];
	info.getRayStart( v );
	return VectorType( v[0], v[1], v[2] );
}

VectorType getRayEnd( RayPickingInfo info )
{
	float v[3];
	info.getRayEnd( v );
	return VectorType( v[0], v[1], v[2] );
}

// Return vectorfield matrix row indices for normalized coordinates in [0,1]
SDMVisInteractiveEditing::IndexVector getMatrixRows( double* v, int* dims )
{
	int ix = std::floor( v[0] * (dims[0]-1) ),
		iy = std::floor( v[1] * (dims[1]-1) ),
		iz = std::floor( v[2] * (dims[2]-1) );

	size_t ofs = (size_t)(iz * dims[1]*dims[0] + iy * dims[0] + ix);
	
	std::vector<size_t> rows(3);
	rows[0] = 3*ofs+0;
	rows[1] = 3*ofs+1;
	rows[2] = 3*ofs+2;
	
	return rows;
}


//-----------------------------------------------------------------------------
//  C'tor
//-----------------------------------------------------------------------------
SDMVisInteractiveEditing::SDMVisInteractiveEditing()
	: m_sdm        ( NULL ),
      m_modeScaling( ModePlain ),
	  m_gamma      (  100.0 ),
	  m_editScale  (    1.0 ),
	  m_resultScale(    1.0 )
{
}

//-----------------------------------------------------------------------------
//  setPickedPoints()
//-----------------------------------------------------------------------------
void SDMVisInteractiveEditing::setPickedPoints( RayPickingInfo s, RayPickingInfo t )
{
	if( !s.hasIntersection() )
	{
		printf("Warning: Picked start point does not intersect with model!\n");
		return;
	}

	///////////////////////////////////////////////////////////////////////////	
	// Find intersection q of ray t with tangent plane at intersection p of s.
	// The tangent plane is defined by all 3D points x fulfilling
	//
	//         <n,(x-p)> = 0
	//
	// where <.,.> denotes the scalar product and n is the normal at p.
	// The ray t is given by
	//
	//         t(alpha) = v0 + alpha*(v1-v0)
	//
	// for real numbers alpha. Plugging in the ray into the tangent plane 
	// equation and solving for alpha gives us
	// 
	//         alpha' = <n,(p-v0)> / <n,(v1-v0)>
	//
	// which exists iff (v1-v0) and n are not parallel. 
	//
	//         q = t(alpha')
	//
	///////////////////////////////////////////////////////////////////////////
	
	VectorType p  = getIntersection( s ),
	           n  = getNormal      ( s ),
	           v0 = getRayStart    ( t ),
			   v1 = getRayEnd      ( t );

	n = normalized(n);
	
	// Check if (v1-v0) and n are nearly parallel
	float denom = scalar_prod( n, v1-v0 );
	if( fabs(denom) < 1e-10 )
	{
		printf("Warning: Picking ray and tangent plane are nearly parallel!\n");
		return;
	}
	
	// Compute intersection point of t with tangent plane at p
	float alpha = scalar_prod( n, p-v0 ) / denom;
	
	VectorType q = v0 + (v1-v0)*alpha;	
	
	// Sanity checks
#if 0
	printf( "Intersection alpha=%3.2f at (%3.2f,%3.2f,%3.2f) with <n,(q-p)>=%g\n", 
		    alpha,
			q.x(), q.y(), q.z(),
	        scalar_prod( n, (q-p) ) );
#endif

	m_pStart = p;
	m_pEnd   = q;

	// Least-squares solution
	solve( p, q );
}

//-----------------------------------------------------------------------------
//  solve()
//-----------------------------------------------------------------------------

void SDMVisInteractiveEditing::
  getSubMatrix( std::vector<size_t> rows, Matrix& B )
{
	mattools::ValueType* modeRows;
	modeRows = new mattools::ValueType[ rows.size() * m_eigenmodes.getNumCols() ];	
	m_eigenmodes.get_rows( rows, modeRows );
	
	B.resize( rows.size(), m_eigenmodes.getNumCols() );
	rednum::matrix_from_rawbuffer< 
		mattools::ValueType, Matrix, ValueType>( B, modeRows, true );
	
	delete [] modeRows;	
}

void SDMVisInteractiveEditing::
  solve( VectorType v0, VectorType v1 )
{
	double w0[3], w1[3];
	w0[0] = v0.x();	w0[1] = v0.y();	w0[2] = v0.z();
	w1[0] = v1.x();	w1[1] = v1.y();	w1[2] = v1.z();
	solve( w0, w1 );
}

void SDMVisInteractiveEditing::
  solve( double* v0, double* v1 )
{		
	// Scale factor from normalized to physical units
	double coordinateScaling[3];
	coordinateScaling[0] = m_sdm->getHeader().spacing[0] * m_sdm->getHeader().resolution[0];
	coordinateScaling[1] = m_sdm->getHeader().spacing[1] * m_sdm->getHeader().resolution[1];
	coordinateScaling[2] = m_sdm->getHeader().spacing[2] * m_sdm->getHeader().resolution[2];

	// Edit vector
	Vector d_edit( 3 );
	d_edit(0) = m_editScale * (v1[0] - v0[0]) * coordinateScaling[0];
	d_edit(1) = m_editScale * (v1[1] - v0[1]) * coordinateScaling[1];
	d_edit(2) = m_editScale * (v1[2] - v0[2]) * coordinateScaling[2];
	
	solve( getMatrixRows( v0, m_volumeSize ), d_edit );	
}


void SDMVisInteractiveEditing::
  solveEdit( VectorType v0, VectorType d_edit )
{
	double w0[3];
	w0[0] = v0.x();	w0[1] = v0.y();	w0[2] = v0.z();

	Vector ded(3);
	ded(0) = d_edit.x();
	ded(1) = d_edit.y();
	ded(2) = d_edit.z();

	solveEdit( w0, ded );
}

void SDMVisInteractiveEditing::
  solveEdit( double* v0, Vector d_edit )
{
	solve( getMatrixRows( v0, m_volumeSize ), d_edit );
}

void SDMVisInteractiveEditing::
  solve( IndexVector rows, Vector d_edit )
{
	using namespace boost::numeric::ublas;
	
	// Restricted eigenmode matrix B
	Matrix B;
	getSubMatrix( rows, B );
	
	// System matrix
	Matrix A = prod( trans(B), B );
	for( unsigned i=0; i < A.size1(); i++ )
	{
		A(i,i) = A(i,i) + m_gamma;
	}	
	
	// Right hand side
	Vector b = prod( d_edit, B );
	
	// Solve A*x=b
	Vector x;
	rednum::solve_ls<Matrix,Vector,ValueType>( A, b, x );	

	// Store solution
	m_copt  = x * m_resultScale;
	m_Edata = norm_2( d_edit - prod( B, m_copt ) );
	m_Ereg  = m_gamma * norm_2( m_copt );;
	m_dedit = d_edit;
	m_displacement = prod( B, m_copt );
}

void SDMVisInteractiveEditing::getReducedBasis( VectorType v0, Matrix& B )
{
	using namespace boost::numeric::ublas;

	// Get row indices

	int ix = std::floor( v0.x() * (m_volumeSize[0]-1) ),
		iy = std::floor( v0.y() * (m_volumeSize[1]-1) ),
		iz = std::floor( v0.z() * (m_volumeSize[2]-1) );

	size_t ofs = iz * m_volumeSize[1]*m_volumeSize[0] + iy * m_volumeSize[0] + ix;

	std::vector<size_t> rows(3);
	rows[0] = 3*ofs+0;
	rows[1] = 3*ofs+1;
	rows[2] = 3*ofs+2;

	// Assemble restricted eigenmode matrix B

	//static std::vector<mattools::ValueType> modeRows;
	//modeRows.resize( rows.size(), m_eigenmodes.getNumCols() );
	mattools::ValueType* modeRows;
	modeRows = new mattools::ValueType[ rows.size() * m_eigenmodes.getNumCols() ];	
	m_eigenmodes.get_rows( rows, modeRows );

	// Number of modes to consider
	unsigned numModes = m_eigenmodes.getNumCols();
	B = Matrix( rows.size(), numModes );
	rednum::matrix_from_rawbuffer< 
		mattools::ValueType, Matrix, ValueType>( B, modeRows, true );
	delete [] modeRows;
}

void SDMVisInteractiveEditing::getSystemMatrices( VectorType v0, 
	Matrix& A, Matrix& B, Matrix& D )
{
	getReducedBasis( v0, B );

	unsigned numModes = m_eigenmodes.getNumCols();
	unsigned numRows = 3;

	// Apply mode scaling
	Matrix D2;
	//if( m_modeScaling != ModePlain )
	{
		// Find intrinsic dimensionality
	  #if 1
		// Assume all eigenvalues are valid
		unsigned m = m_eigenvalues.size();
	  #else
		unsigned m = 0;
		for( m=0; (m < m_eigenvalues.size()) && (m_eigenvalues(m) > 1e-10); m++ )
	  #endif

		// Sanity check
		if( m == 0 )
		{
			std::cerr << "Warning: Encountered ill-posed problem!\n";
			return;
		}

		// Reduce B to only contain 'stable' modes
		if( m < numModes )
		{
			numModes = m;
			B.resize( numRows, numModes );
		}

		// Non-optimized scaling via diagonal matrix
		D  = Matrix( numModes, numModes, (ValueType)0.0 );
		D2 = D;
		for( unsigned i=0; i < numModes; i++ )
		{
			switch( m_modeScaling )
			{
			default:
			case ModePlain     :  D(i,i) =                    1.0; break;
			case ModeNormalized:  D(i,i) =      m_eigenvalues[i] ; break;
			case ModeSqrt      :  D(i,i) = sqrt(m_eigenvalues[i]); break;
			case ModeInverse   :  D(i,i) = 1. / m_eigenvalues[i] ; break;
			}

			D2(i,i) = D(i,i) * D(i,i);
		}
		B = prod( B, D );
	}

	// System matrix
	A = prod( trans(B), B );
	for( unsigned i=0; i < A.size1(); i++ )
	{
		A(i,i) = A(i,i) + D2(i,i)*m_gamma;
	}

}

void SDMVisInteractiveEditing::solveWeighted( VectorType v0, VectorType v1 )
{
	using namespace boost::numeric::ublas;

	// Setup linear system
	Matrix A, B, D;
	getSystemMatrices( v0, A, B, D );

	ValueType gamma     = m_gamma;     // 1e10;
	ValueType editScale = m_editScale; // 1e12;

	// Convert edit from normalized to physical units	
	double coordinateScaling[3];
	coordinateScaling[0] = m_sdm->getHeader().spacing[0] * m_sdm->getHeader().resolution[0];
	coordinateScaling[1] = m_sdm->getHeader().spacing[1] * m_sdm->getHeader().resolution[1];
	coordinateScaling[2] = m_sdm->getHeader().spacing[2] * m_sdm->getHeader().resolution[2];

	Vector d_edit( 3 );
	d_edit(0) = editScale * (v1.x() - v0.x()) * coordinateScaling[0];
	d_edit(1) = editScale * (v1.y() - v0.y()) * coordinateScaling[1];
	d_edit(2) = editScale * (v1.z() - v0.z()) * coordinateScaling[2];


	// Right hand side
	Vector b  = prod( d_edit, B ); // b = trans(B) * d_edit
	    // b += gamma * c0;       --> c0!=0 and without weights
	    // b += gamma * D2 * c0;  --> with diagonal weights D
	// could as well be done via: axpy_prod( d_edit, B, b, true );

	// Solve A*x=b
	Vector x;
	rednum::solve_ls<Matrix,Vector,ValueType>( A, b, x );

#if 1
	rednum::save_matrix<float>( A, "tmp_A.mat" );
	rednum::save_vector<float>( b, "tmp_b.mat" );
	rednum::save_vector<float>( x, "tmp_x.mat" );	
#endif

	// Convert result
#if 0
	//if( m_modeScaling != ModePlain )
	{
		// Apply inverse of diagonal scaling matrix
		for( unsigned i=0; i < x.size(); i++ )
			x(i) = x(i) * ( 1.0 / D(i,i) );
	}
#endif
	// Result coefficients
	Vector copt = x * m_resultScale;

	// Compute error
#if 0
	// without diagonal weights D	
	double Edata = norm_2( d_edit - prod( prod(B,D), copt ) );
	double Ereg  = gamma * norm_2( copt );  // assuming c0=0
#else
	// with diagonal weights D
	double Edata = norm_2( d_edit - prod( prod(B,D), copt ) );
	double Ereg  = gamma * norm_2( prod(D,copt) );  // assuming c0=0
#endif

	Vector displacement = prod( B, copt );
#if 0
	printf("Coefficients %5.4f, %5.4f, %5.4f, %5.4f, %5.4f\n",
		x(0), x(1), x(2), x(3), x(4), x(5) );
#endif
	
	// Set members
	m_copt  = copt;
	m_Edata = Edata;
	m_Ereg  = Ereg;
	m_dedit = d_edit;
	m_displacement = displacement;
}


//-----------------------------------------------------------------------------
//  getStartEndPoints()
//-----------------------------------------------------------------------------
void SDMVisInteractiveEditing::
	getStartEndPoints( VectorType& p, VectorType& q, bool displace ) const 
{		
	p = m_pStart;
	q = m_pEnd;

	// Displace points according to vector field (for correct visualization
	// of points on transformed surface).
	if( displace )
	{
		double coordinateScaling[3];
		coordinateScaling[0] = m_sdm->getHeader().spacing[0] * m_sdm->getHeader().resolution[0];
		coordinateScaling[1] = m_sdm->getHeader().spacing[1] * m_sdm->getHeader().resolution[1];
		coordinateScaling[2] = m_sdm->getHeader().spacing[2] * m_sdm->getHeader().resolution[2];

		// Displacement is given in physical coordinates and has to be converted
		// to (normalized) world coordinates for rendering.
		Vector d = getDisplacement();
		d(0) = d(0) / coordinateScaling[0];
		d(1) = d(1) / coordinateScaling[1];
		d(2) = d(2) / coordinateScaling[2];		
		VectorType v( d(0),d(1),d(2) );

		double scale = 1.0; // was: 1.0 / this->getEditScale();

		p += scale * v;
		q += scale * v;
	}
}
