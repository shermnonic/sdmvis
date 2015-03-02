#include "EditLocalCovariance.h"
#include "ImageDataSpace.h"

EditLocalCovariance::EditLocalCovariance()
	: m_sphericalSamplingRadius( 5.0 )
{
	setDirectionalSamplingLevel( 1 );
}

void EditLocalCovariance::
  setSDM( StatisticalDeformationModel* sdm )
{
	// base implementation
	SDMTensorDataProvider::setSDM( sdm );

	m_edit.setSDM( sdm );
}

void EditLocalCovariance::
  setReferencePoint( double x, double y, double z )
{
	// Sanity check
	if( !getSDM() )
		return;

	m_coeffSet.clear();	

	m_referencePoint[0] = x;
	m_referencePoint[1] = y;
	m_referencePoint[2] = z;

	SDMVisInteractiveEditing::VectorType v0(x,y,z);

	// Sample edit directions

	for( int i=0; i < m_icosahedron.num_vertices(); i++ )
	{
		std::cout << "Sampling edit directions " 
			<< i+1 << "/" << m_icosahedron.num_vertices() << " \r";

		Geometry::vec3 vdir( m_icosahedron.get_vertex(i) );
#if 1
		// PROBE IN PHYSICAL COORDINATES

		// Convert to normalized coordinates
		vdir *= m_sphericalSamplingRadius; // Apply physical radius
		//double vx=vdir.x, vy=vdir.y, vz=vdir.z;
		//getSDM()->getHeader().normalizeCoordinates( vx, vy, vz );

		SDMVisInteractiveEditing::VectorType d_edit(vdir.x,vdir.y,vdir.z);
		m_edit.solveEdit( v0, d_edit );
#else
		// PROBE IN NORMALIZED COORDINATES
		SDMVisInteractiveEditing::VectorType dir(vdir.x,vdir.y,vdir.z);
		// Undo anisotropic scaling in normalized coordinates.
		// Take radius in x as reference for aspect ratio calculation.
		double aspectY, aspectZ;
		getSDM()->getHeader().getAspectX( aspectY, aspectZ );
		dir.setY( dir.y() / aspectY );
		dir.setZ( dir.z() / aspectZ );
		// TODO: Check that v0+dir does not leave unit cube?!
		//       Or is this taken care of in solve()?
		m_edit.solve( v0, v0 + m_sphericalSamplingRadius*dir );
#endif


		m_coeffSet.push_back( m_edit.getCoeffs() );
	}
	std::cout << "\n";
}

void EditLocalCovariance::
  setDirectionalSamplingLevel( int l )
{
	m_icosahedron.create( l );
}

void outer_product_add( float x, float y, float z, float (&result3x3)[9] )
{
	// Result is row-major:
	//  0  1  2
	//  3  4  5
	//  6  7  8
	result3x3[0] += x*x;
	result3x3[4] += y*y;
	result3x3[8] += z*z;
	result3x3[1] += x*y;
	result3x3[2] += x*z;
	result3x3[5] += y*z;
	// Force symmetry
	result3x3[3] = result3x3[1];
	result3x3[6] = result3x3[2];
	result3x3[7] = result3x3[5];
}

void EditLocalCovariance::
  getTensor( double x, double y, double z, float (&tensor3x3)[9] )
{
	using namespace boost::numeric::ublas;

	// Reset tensor (important since we later add outer product to it)
	for( unsigned i=0; i < 9; i++ )
		tensor3x3[i] = 0.f;

	// Normalize (x,y,z) to conform to SDM format
	getSDM()->getHeader().normalizeCoordinates( x, y, z );

	// Get vectorfield matrix row indices corresponding to (x,y,z)
	StatisticalDeformationModel::IndexVector rows;
	getSDM()->appendRowIndices( x, y, z, rows );

	// Buffer for the sub-matrix U with rows corresponding to (x,y,z)
	mattools::ValueType* buf;
	buf = new mattools::ValueType[ getSDM()->getRowSize(rows) ];

	Matrix U( rows.size(), getSDM()->getRowSize(rows) / rows.size() );
	getSDM()->getEigenmodeRows( rows, buf );
	rednum::matrix_from_rawbuffer<
		mattools::ValueType, Matrix, ValueType>( U, buf, true );

	// SCATTER MATRIX WITH CENTERING
	Matrix X( 3, m_coeffSet.size() );
	for( unsigned i=0; i < m_coeffSet.size(); i++ )
	{
		Vector c = m_coeffSet.at(i);
		Vector disp = prod( U, c );

		X(0,i) = disp(0);
		X(1,i) = disp(1);
		X(2,i) = disp(2);

		//// Outer product (without centering)
		//outer_product_add( disp(0),disp(1),disp(2), tensor3x3 );
	}
	//// Scale by 1/n
	//for( unsigned i=0; i < 9; i++ )
	//	tensor3x3[i] /= m_coeffSet.size();

	// Center matrix and store mean displacement
	Vector mu = rednum::center_rows<Matrix,Vector>( X );

	Matrix S = prod( X, trans(X) );
	S /= m_coeffSet.size();
	rednum::matrix_to_rawbuffer( S, tensor3x3 );

	delete [] buf;
}
