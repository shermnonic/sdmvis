#include "RayPickingInfo.h"
#include <cstring> // for: memcpy(), memset()

RayPickingInfo::RayPickingInfo()
{
	reset();
}
	
void RayPickingInfo::reset()
{
	m_hasIntersection = false;
	m_hasNormal       = false;
	m_hasRayStart     = false;
	m_hasRayEnd       = false;

	memset(m_intersection,0,3*sizeof(float));
	memset(m_normal      ,0,3*sizeof(float));
	memset(m_rayStart    ,0,3*sizeof(float));
	memset(m_rayEnd      ,0,3*sizeof(float));
}

#define RAYPICKINFO_SET( VECTOR, X, Y, Z ) \
	VECTOR[0] = X; VECTOR[1] = Y; VECTOR[2] = Z;
	
void RayPickingInfo::setIntersection( float x, float y, float z )
{
	RAYPICKINFO_SET( m_intersection, x,y,z );	
	m_hasIntersection = true;
}

void RayPickingInfo::setNormal      ( float x, float y, float z )
{
	RAYPICKINFO_SET( m_normal, x,y,z );	
	m_hasNormal = true;
}

void RayPickingInfo::setRayStart    ( float x, float y, float z )
{
	RAYPICKINFO_SET( m_rayStart, x,y,z );	
	m_hasRayStart = true;
}

void RayPickingInfo::setRayEnd      ( float x, float y, float z )
{
	RAYPICKINFO_SET( m_rayEnd,   x,y,z );	
	m_hasRayEnd   = true;
}


void RayPickingInfo::getIntersection( float* v )
{
	memcpy( v, &m_intersection, 3*sizeof(float) );
}

void RayPickingInfo::getNormal      ( float* v )
{
	memcpy( v, &m_normal      , 3*sizeof(float) );
}

void RayPickingInfo::getRayStart    ( float* v )
{
	memcpy( v, &m_rayStart    , 3*sizeof(float) );
}

void RayPickingInfo::getRayEnd      ( float* v )
{
	memcpy( v, &m_rayEnd      , 3*sizeof(float) );
}
