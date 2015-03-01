#include "Trackball2.h"
#include <glm/gtc/matrix_transform.hpp>  // glm::translate, ::rotate, ...
#include <cmath> // sqrt()


//#define MICHAEL
#ifdef MICHAEL
#define SCREEN_NORMALIZE(ax,ay) .5f * ( 2.f * ax - w ) / w, .5f * ( h - 2.f * ay ) / h
#else
#define SCREEN_NORMALIZE(ax,ay) ax/(float)w - .5f, .5f - ay/(float)h 
#endif


//-----------------------------------------------------------------------------
glm::vec3 Trackball2::projectToSphere( float x, float y  )
{
	static const float sqrt2 = sqrt(2.f);
	float z,
	      d = sqrt( x*x + y*y ),
	      r = m_trackballRadius;

#ifdef MICHAEL
	if( d < r*.5*sqrt2 )
	{
		// Inside disc, choose z to fulfill  x^2 + y^2 + z^2 = r^2
		z = sqrt( r*r - d*d );
	}
	else
	{
		// Outside disc - Project onto hyperbola
		float t = r / sqrt2;
		z = t*t / d;
	}
#else
	// unit box (-.5,.5) - (.5,-.5)
	if( d < .5f )
	{
		// Inside disc, choose z to fulfill  x^2 + y^2 + z^2 = r^2
		x = 2.f*x;
		y = 2.f*y;
		z = sqrt(1.f - 4.f*d*d);
	}
	else
	{
		// Outside disc 
		// Project onto great circle parallel to viewer, i.e. xy plane
		x = x / d;
		y = y / d;
		z = 0;
	}
#endif
	return glm::vec3(x,y,z);
}

//-----------------------------------------------------------------------------
glm::fquat Trackball2::combineRotations( glm::fquat q1, glm::fquat q2 )
{
	using namespace glm;
#if 0
	vec3 v1( q1.x, q1.y, q1.z ),
	     v2( q2.x, q2.y, q2.z );

	fquat q = fquat( q1.w * q2.w - dot(v1,v2),           // scalar value
	                 v1*q2.w + v2*q1.w + cross(v1,v2) ); // vectorial part
#else
	// inconveniently, cross() denots the quaternion multiplication in glm
	fquat q = cross( q2, q1 ); 
#endif
	q = normalize( q );
	return q;
}

//-----------------------------------------------------------------------------
void Trackball2::update( float ax, float ay, float bx, float by, int mode )
{
	const float eps = 2*std::numeric_limits<float>::epsilon();

	if( mode==Rotate )
	{
		// Get coordinates on unit sphere, a and b
		glm::vec3 a( projectToSphere(ax,ay ) );
		glm::vec3 b( projectToSphere(bx,by ) );
		
		// Rotation angle
		float ab = glm::dot( a, b );
		// Check if a is parallel to b to avoid singular acos()
		//if( fabs(1.f-ab) < eps )
		//	return;
		float theta = acos( ab );		
		
		// Rotation axis c
		glm::vec3 R = glm::normalize( glm::cross(a,b) );
		
		// Unit quaternion for this rotation
		glm::fquat q( cos(theta/2.f), R*sin(theta/2.f) );
		
		if( m_immediateUpdate )
		{
			// Mix with current trackball rotation
			m_qrot = combineRotations( m_qrot, q );
		}
		else
		{
			// Update temporary rotation
			m_cur_qrot = q;
		}
	}
	else
	if( mode==Translate )
	{
		// Get current rotation matrix
		glm::mat3 R = getRotationMatrix();
		
		// Translate along directions Rx and Ry
		R = glm::transpose(R);
		m_trans += R[0] * (bx - ax) * ( m_speed + m_zoom ) / 100.f;
		m_trans += R[1] * (by - ay) * ( m_speed + m_zoom ) / 100.f;
	}
	else
	if( mode==Zoom )
	{
		m_zoom += (by - ay) * ( m_speed + m_zoom ) / 50.f;
	}
}

//-----------------------------------------------------------------------------
glm::mat4 Trackball2::getCameraMatrix() const
{
	glm::mat4 
	  Zoom  = glm::translate( glm::mat4(1.0), glm::vec3(0.f,0.f,-m_zoom) ),
	  Rot   = glm::mat4( getRotationMatrix() ),
	  Trans = glm::translate( glm::mat4(1.0), m_trans );
	//return Trans * Rot * Zoom;
	return Zoom * Rot * Trans;
}

//-----------------------------------------------------------------------------
glm::mat3 Trackball2::getRotationMatrix() const
{
	if( !m_immediateUpdate ) //&& m_mode == Rotate
		return glm::mat3_cast( glm::cross( m_cur_qrot, m_qrot ) );

	return glm::mat3_cast( m_qrot );
}


//-----------------------------------------------------------------------------

void Trackball2::update( int ax, int ay, int bx, int by, int mode )
{
	float w = (float)m_viewWidth,
		    h = (float)m_viewHeight;
	update(
		SCREEN_NORMALIZE(ax,ay),
		SCREEN_NORMALIZE(bx,by),
		mode
	);
}

void Trackball2::start( int ax, int ay, int mode )
{
	float w = (float)m_viewWidth,
		    h = (float)m_viewHeight;
	start(
		SCREEN_NORMALIZE(ax,ay),
		mode 
	);
}

void Trackball2::update( int bx, int by )
{
	float w = (float)m_viewWidth,
		  h = (float)m_viewHeight;
	update(
		SCREEN_NORMALIZE(bx,by)
	);
}
