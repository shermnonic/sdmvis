#include "ImageDataSpace.h"
#include <cmath>

void ImageDataSpace::
  reset()
{
	m_dims[0] = 0;
	m_dims[1] = 0;
	m_dims[2] = 0;
	m_spacing[0] = 1.0;
	m_spacing[1] = 1.0;
	m_spacing[2] = 1.0;
	m_origin[0] = 0.0;
	m_origin[1] = 0.0;
	m_origin[2] = 0.0;
}

ImageDataSpace::
  ImageDataSpace()
{
	reset();
}

ImageDataSpace::
  ImageDataSpace( const int dims[3] )
{
	reset();
	m_dims[0] = dims[0];
	m_dims[1] = dims[1];
	m_dims[2] = dims[2];
}

ImageDataSpace::
  ImageDataSpace( const int dims[3], const double spacing[3] )
{
	reset();
	m_dims[0] = dims[0];
	m_dims[1] = dims[1];
	m_dims[2] = dims[2];
	m_spacing[0] = spacing[0];
	m_spacing[1] = spacing[1];
	m_spacing[2] = spacing[2];
}

ImageDataSpace::
  ImageDataSpace( const int dims[3], const double spacing[3], 
	 		      const double origin[3] )
{
	m_dims[0] = dims[0];
	m_dims[1] = dims[1];
	m_dims[2] = dims[2];
	m_spacing[0] = spacing[0];
	m_spacing[1] = spacing[1];
	m_spacing[2] = spacing[2];
	m_origin[0] = origin[0];
	m_origin[1] = origin[1];
	m_origin[2] = origin[2];
}

void ImageDataSpace::
  getPoint( int i, int j, int k, double (&pt)[3] ) const
{
	pt[0] = (double)i*m_spacing[0] + m_origin[0];
	pt[1] = (double)j*m_spacing[1] + m_origin[1];
	pt[2] = (double)k*m_spacing[2] + m_origin[2];
};

void ImageDataSpace::
  getPointFromNormalized( double nx, double ny, double nz, double (&pt)[3] ) const
{
	pt[0] = nx*m_spacing[0]*m_dims[0] + m_origin[0];
	pt[1] = ny*m_spacing[1]*m_dims[1] + m_origin[1];
	pt[2] = nz*m_spacing[2]*m_dims[2] + m_origin[2];
}

void ImageDataSpace::
  getIJK( double x, double y, double z, int (&ijk)[3] ) const
{
	double di,dj,dk;
	di = (x - m_origin[0]) / m_spacing[0];
	dj = (y - m_origin[1]) / m_spacing[1];
	dk = (z - m_origin[2]) / m_spacing[2];
	
	// We could add support for different rounding strategies.
	//  double round(double r) {
	//      return (r > 0.0) ? floor(r + 0.5) : ceil(r - 0.5);
	//  }		
	ijk[0] = (int)std::floor(di);
	ijk[1] = (int)std::floor(dj);
	ijk[2] = (int)std::floor(dk);
	
	// Make sure we don't leave the data grid
	if( ijk[0] >= m_dims[0] ) ijk[0] = m_dims[0]-1;
	if( ijk[1] >= m_dims[1] ) ijk[1] = m_dims[1]-1;		
	if( ijk[2] >= m_dims[2] ) ijk[2] = m_dims[2]-1;
	
	if( ijk[0] < 0 ) ijk[0] = 0;
	if( ijk[1] < 0 ) ijk[1] = 0;
	if( ijk[2] < 0 ) ijk[2] = 0;
};

void ImageDataSpace::
  getNormalized( const double (&pt)[3], double (&n)[3] ) const
{
	n[0] = (pt[0] - m_origin[0]) / (m_dims[0]*m_spacing[0]);
	n[1] = (pt[1] - m_origin[1]) / (m_dims[1]*m_spacing[1]);
	n[2] = (pt[2] - m_origin[2]) / (m_dims[2]*m_spacing[2]);
}

void ImageDataSpace:: 
  getNormalized( int i, int j, int k, double (&n)[3] ) const
{
	n[0] = i  / m_dims[0];
	n[1] = j  / m_dims[1];
	n[2] = k  / m_dims[2];
}
