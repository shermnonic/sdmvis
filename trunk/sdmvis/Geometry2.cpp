#include "Geometry2.h"
#include <cassert>
#include <iostream>

using namespace Geometry;

//==============================================================================
//	SimpleGeometry
//==============================================================================

SimpleGeometry::Face::Face() 
	{ vi[0]=vi[1]=vi[2]=0; }
SimpleGeometry::Face::Face( int i[3] )
	{ vi[0]=i[0]; vi[1]=i[1]; vi[2]=i[2]; }
SimpleGeometry::Face::Face( int i, int j, int k )
	{ vi[0]=i; vi[1]=j; vi[2]=k; }

/*
void SimpleGeometry::test_pointer_consistency()
{
	using namespace std;
	// test ptr consistency
	float* vp = get_vertices_ptr();
	float err=0.f;
	for( int i=0; i < num_vertices(); ++i )
	{
		vec3 v = get_vertex( i );
		for( int j=0; j < 3; ++j ) 
		{			
			cout << i << ":  " << vp[3*i+j] << "  vs.  " << v[j] << endl;
			err += (vp[3*i+j] - v[j])*(vp[3*i+j] - v[j]);
		}
	}
	cout << "Error = " << err << endl;
	int* tp = get_indices_ptr();
	for( int i=0; i < num_faces(); ++i )
	{
		Face f = get_face(i);
		for( int j=0; j < 3; ++j )
			assert( tp[i*3+j] == f.vi[j] );
	}
}
*/

void SimpleGeometry::clear()
{
	m_vdata.clear();
	m_ndata.clear();
	m_fdata.clear();
}

void SimpleGeometry::reserve_vertices( int n )
{
#ifndef GEOMETRY2_NO_BUFFER_SUPPORT
	m_vdata.reserve( 3*(n-8) );
	m_ndata.reserve( 3*(n-8) );
#else
	m_vertices.reserve( n - 8 );
	m_normals .reserve( n - 8 );
#endif
}

void SimpleGeometry::reserve_faces( int n )
{
#ifndef GEOMETRY2_NO_BUFFER_SUPPORT
	m_fdata.reserve( 3*n );
#else
	m_faces   .reserve( n );
#endif
}

#ifndef GEOMETRY2_NO_BUFFER_SUPPORT
	
int SimpleGeometry::num_vertices() const { return (int)m_vdata.size()/3; }
int SimpleGeometry::num_faces()    const { return (int)m_fdata.size()/3; }

vec3 SimpleGeometry::get_vertex( int i )
{
	return vec3( m_vdata[i*3], m_vdata[i*3+1], m_vdata[i*3+2] );
}

SimpleGeometry::Face SimpleGeometry::get_face( int i )
{
	return Face( &m_fdata[i*3] );
}

int SimpleGeometry::add_face( Face f )
{
	for( int i=0; i < 3; ++i )
		m_fdata.push_back( f.vi[i] );
	assert( m_fdata.size()%3 == 0 );
	return (int)m_fdata.size()/3 - 1;
}

int SimpleGeometry::add_vertex_and_normal( vec3 v, vec3 n )
{	
	for( int i=0; i < 3; ++i ) 
	{
		m_vdata.push_back( v[i] );
		m_ndata.push_back( n[i] );
	}
	assert( m_vdata.size() == m_ndata.size() );
	assert( m_vdata.size()%3 == 0 );
	return (int)m_vdata.size()/3 - 1;
}

float* SimpleGeometry::get_vertex_ptr()  { return &m_vdata[0]; }

float* SimpleGeometry::get_normal_ptr ()  { return &m_ndata[0]; }

int*   SimpleGeometry::get_index_ptr()   { return &m_fdata[0]; }

#else

int SimpleGeometry::num_vertices() const { return m_vertices.size(); }
int SimpleGeometry::num_faces()    const { return m_faces   .size(); }

vec3 SimpleGeometry::get_vertex( int i )
{
	return m_vertices.at(i);
}

SimpleGeometry::Face SimpleGeometry::get_face( int i )
{
	return m_faces.at(i);
}

int SimpleGeometry::add_face( Face f )
{
	m_faces.push_back( f );
	return m_faces.size()-1;
}

int SimpleGeometry::add_vertex_and_normal( vec3 v, vec3 n )
{	
	m_vertices.push_back( v );
	m_normals .push_back( n );
	// enforce identical indices for vertices/normals
	assert( m_vertices.size() == m_normals.size() );
	return m_vertices.size()-1;
}

float* SimpleGeometry::get_vertex_ptr()  { return &m_vertices[0][0]; }

float* SimpleGeometry::get_normal_ptr ()  { return &m_normals[0][0]; }

int*   SimpleGeometry::get_index_ptr()   { return &m_faces[0].vi[0]; }

#endif // GEOMETRY2_NO_BUFFER_SUPPORT


//==============================================================================
//	Icosahedron
//==============================================================================

void Icosahedron::add_face_subdivision( Face f, int levels )
{
	if( levels == 0 )
	{
		// insert face for real
		add_face( f );
		return;
	}

	// get vertices
	vec3 v1 = get_vertex( f.vi[0] ),
		 v2 = get_vertex( f.vi[1] ),
		 v3 = get_vertex( f.vi[2] );
	
	// new vertices
	vec3 v12=v1,v13=v1,v23=v2;
	v12 += v2;
	v13 += v3;
	v23 += v3;
#if 0
	// test subdivision
	v12 /= 2.f;
	v13 /= 2.f;
	v23 /= 2.f;
#else
	// normalize such that the new vertices again lie on the unit sphere surface
	v12.normalize();
	v13.normalize();
	v23.normalize();

	float foo = (float)m_platonicConstantZ;
	v12 *= foo;
	v13 *= foo;
	v23 *= foo;
#endif
	
	// corresponding indices
	int i1  = f.vi[0],
	    i2  = f.vi[1],
	    i3  = f.vi[2],
		i12 = add_vertex_and_normal( v12, v12/v12.magnitude() ),
		i13 = add_vertex_and_normal( v13, v13/v13.magnitude() ),
		i23 = add_vertex_and_normal( v23, v23/v23.magnitude() );
	
	// 4 new faces
	add_face_subdivision( Face( i1,  i12, i13 ), levels-1 );
	add_face_subdivision( Face( i12, i2,  i23 ), levels-1 );
	add_face_subdivision( Face( i13, i23, i3  ), levels-1 );
	add_face_subdivision( Face( i12, i23, i13 ), levels-1 );
}

void Icosahedron::create( int levels )
{
	if( levels < 0 ) 
		levels = m_levels;
	else
		m_levels = levels;

	// initial 20 sided platonic solid
	
	// golden ratio (1+sqrt(5))/2 = 1.6180339887498948482045868343656
	// sqrt(5) = 2,2360679774997896964091736687313
	// 2*pi/5 = 1,2566370614359172953850573533118

	//static double X = 1.287654321; //.525731112119133606;
	//static double Z = 1.723456789; //.850650808352039932;
#if 0
	// Bendels Icosahedron definition
	double X = .525731112119133606 , //m_platonicConstantX,
		   Z = .850650808352039932;  //m_platonicConstantZ,		   
	//Y = 0.3;
	/*static*/ vec3 vdata[12] = {
	   //vec3(-X, -Y, Z), vec3(X, 0.0, Z), vec3(-X, 0.0, -Z), vec3(X, 0.0, -Z),
	   //vec3(0.0, Z, X), vec3(0.0, Z, -X), vec3(0.0, -Z, X), vec3(0.0, -Z, -X),    
	   //vec3(Z, X, -Y), vec3(-Z, X, 0.0), vec3(Z, -X, 0.0), vec3(-Z, -X, 0.0) 
	   vec3(-X, 0.0, Z), vec3(X, 0.0, Z), vec3(-X, 0.0, -Z), vec3(X, 0.0, -Z),
	   vec3(0.0, Z, X), vec3(0.0, Z, -X), vec3(0.0, -Z, X), vec3(0.0, -Z, -X),    
	   vec3(Z, X, 0.0), vec3(-Z, X, 0.0), vec3(Z, -X, 0.0), vec3(-Z, -X, 0.0) 
	};
	static int tindices[20][3] = { 
	   {0,4,1}, {0,9,4}, {9,5,4}, {4,5,8}, {4,8,1},    
	   {8,10,1}, {8,3,10}, {5,3,8}, {5,2,3}, {2,7,3},    
	   {7,10,3}, {7,6,10}, {7,11,6}, {11,0,6}, {0,1,6}, 
	   {6,1,10}, {9,0,11}, {9,11,2}, {9,2,5}, {7,2,11} };
#else
	// Classic Icosahedron definition (based solely on golden ratio constant)
	float tao = (float)m_platonicConstantX; //1.61803399;
	vec3 vdata[12] = { vec3(1,tao,0),vec3(-1,tao,0),vec3(1,-tao,0),vec3(-1,-tao,0),
					   vec3(0,1,tao),vec3(0,-1,tao),vec3(0,1,-tao),vec3(0,-1,-tao),
					   vec3(tao,0,1),vec3(-tao,0,1),vec3(tao,0,-1),vec3(-tao,0,-1) };

	static int tindices[20][3] = { 
	{0,1,4},{1,9,4},{4,9,5},{5,9,3},{2,3,7},{3,2,5},{7,10,2},{0,8,10},{0,4,8},{8,2,10},{8,4,5},{8,5,2},{1,0,6},{11,1,6},{3,9,11},{6,10,7},{3,11,7},{11,6,7},{6,0,10},{9,1,11}
	};
#endif

	// exact memory calulation
	int n = (int)(20.0*pow(4.0,levels));
	reserve_vertices( n - 8 );
	reserve_faces   ( n );
	   
	// insert vertices
	for( int i=0; i < 12; ++i )
	{
		// add normalized vertices lying on the surface of the unit sphere
		vec3 v = vdata[i];
		v.normalize();
		add_vertex_and_normal( v, v );
	}
	
	// insert faces (at the end of subdivision process)
	for( int i=0; i < 20; ++i ) 
	{
		add_face_subdivision( Face(tindices[i]), levels );
	}
}
