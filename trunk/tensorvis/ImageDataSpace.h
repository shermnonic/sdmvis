#ifndef IMAGEDATASPACE_H
#define IMAGEDATASPACE_H

#include <cassert>

/// Helper class convert between physical and image space coordinates.
class ImageDataSpace
{
public:
	ImageDataSpace(); // Should we really provide a default c'tor?
	ImageDataSpace( const int dims[3] );
	ImageDataSpace( const int dims[3], const double spacing[3] );
	ImageDataSpace( const int dims[3], const double spacing[3], 
	                const double origin[3] );

	/// Get physical coordinates from voxel index
	void getPoint( int i, int j, int k, double (&pt)[3] ) const;
	
	/// Get voxel index from physical coordinates
	void getIJK( double x, double y, double z, int (&ijk)[3] ) const;

	/// Get physical coordinates from normalized ones
	void getPointFromNormalized( double nx, double ny, double nz, double (&pt)[3] ) const;
	
	// Convenience functions	
	void getPoint( const int ijk[3], double (&pt)[3] ) const
	{
		getPoint( ijk[0],ijk[1],ijk[2], pt );
	}	
	void getIJK( const double pt[3], int (&ijk)[3] ) const
	{
		getIJK( pt[0],pt[1],pt[2], ijk );
	}
	void getPointFromNormalized( const double n[3], double (&pt)[3] ) const
	{
		getPointFromNormalized( n[0],n[1],n[2], pt );
	}

	void getDimensions( int (&dims)[3] ) const
	{
		dims[0] = m_dims[0];
		dims[1] = m_dims[1];
		dims[2] = m_dims[2];
	}

	void getSpacing( double (&spacing)[3] ) const
	{
		spacing[0] = m_spacing[0];
		spacing[1] = m_spacing[1];
		spacing[2] = m_spacing[2];
	}

	void getOrigin( double (&origin)[3] ) const
	{
		origin[0] = m_origin[0];
		origin[1] = m_origin[1];
		origin[2] = m_origin[2];
	}

	void getNormalized( const double (&pt)[3], double (&n)[3] ) const;
	void getNormalized( int i, int j, int k, double (&n)[3] ) const;

	int getNumberOfPoints() const { return m_dims[0]*m_dims[1]*m_dims[2]; }

	inline
	int getPointIndex( double x, double y, double z ) const
	{
		static int ijk[3];
		getIJK( x,y,z, ijk );
		return getPointIndex( ijk[0], ijk[1], ijk[2] );
	}

	inline
	int getPointIndex( int i, int j, int k ) const
	{
		assert(i>=0&&i<m_dims[0]&&j>=0&&j<m_dims[1]&&k>=0&&k<m_dims[2]);
		return k*m_dims[0]*m_dims[1] + j*m_dims[0] + i;
	}

	void reset();
	
private:
	int    m_dims[3];
	double m_origin[3];
	double m_spacing[3];
};

#endif // IMAGEDATASPACE_H
