#include "SphereSelection.h"
#include <GL/GLConfig.h>   // e7/GL
#include <cmath>
#ifndef M_PI
#define M_PI  3.1415926535897932384626433832795028842
#endif

void draw_circle( float* center, float* u, float* v, float radius, int res=120 )
{
	glBegin( GL_LINE_LOOP );
	for( int i=0; i < res; ++i )
	{
		float alpha = (float)(2.*M_PI) * (float)i/res,
		      cosa = (float)cos(alpha),
			  sina = (float)sin(alpha),
			  c[3];

		for( int j=0; j < 3; ++j )
			c[j] = center[j] + radius*(cosa*u[j] + sina*v[j]);

		glVertex3fv( c );
	}	
	glEnd();
}

void draw_axis_aligned_circle_sphere( float* center, float radius, int res=120 )
{
	static float e0[3] = { 1,0,0 },
		         e1[3] = { 0,1,0 },
				 e2[3] = { 0,0,1 };

	draw_circle( center, e0, e1, radius, res );
	draw_circle( center, e0, e2, radius, res );
	draw_circle( center, e1, e2, radius, res );
}

SphereSelection::SphereSelection()
{
	m_center[0] = m_center[1] = m_center[2] = 0.f;
	m_radius = 1.f;
}

SphereSelection::SphereSelection( float x, float y, float z, float radius )
{
	set_center( x,y,z );
	set_radius( radius );
}

void SphereSelection::draw()
{
	draw_axis_aligned_circle_sphere( m_center, m_radius );
}

void SphereSelection::set_center( float x, float y, float z )
{
	m_center[0] = x;
	m_center[1] = y;
	m_center[2] = z;
}

void SphereSelection::get_center( float& x, float &y, float& z ) const
{
	x = m_center[0];
	y = m_center[1];
	z = m_center[2];
}

void  SphereSelection::set_radius( float radius )
{
	m_radius = radius;
}

float SphereSelection::get_radius() const
{
	return m_radius;
}
