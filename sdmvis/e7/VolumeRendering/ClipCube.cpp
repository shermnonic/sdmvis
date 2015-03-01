// Max Hermann, June 2, 2010
#include "ClipCube.h"
#include <GL/glew.h> // GL.h would be sufficient
#include <cassert>
#include <limits>

//linux/ unix specific includes
#ifndef WIN32
#include <string.h>
#endif
//==============================================================================
//	Cube definition
//==============================================================================

/*
	   6-----7     Arbitrary cube notation                                  
      /|    /|                                                              
     2-----3 |     Faces defined CCW, e.g. (0,1,3,2)                        
     | 5---|-4     Colors directly correspond to canonical coordinates.     
     |/    |/      v0=(0,0,1), v7=(1,1,0)                                   
     0-----1                                                                
*/	
float ClipCube::s_verts[8][3] = { {0,0,1}, {1,0,1}, {0,1,1}, {1,1,1},
				 		          {1,0,0}, {0,0,0}, {0,1,0}, {1,1,0} };
int   ClipCube::s_faces[6][4] = { {0,1,3,2}, {1,4,7,3}, {4,5,6,7}, {5,0,2,6},
						          {2,3,7,6}, {5,4,1,0} };

/*
	Unique unit cube paths for intersection computation according to [1]:
	(for our default front/back vertices case (3,6))

	   6-----7		          		          		   6     7
	   |    / 	              	              	      /      |
	   |   3               3         2-----3         2       |
	   5      	       5---|-4	     | 5      	             4
						   |/ 	     |/      	             
						   1	     0      	     0-----1

	   path 0           path 2        path 4      remaining edges 1,3,5

	   6-----7		   6-----7		   6-----7		   6-----7
	  /|    /|	      /|    /|	      /|    /|	      /|    /|
	 2-----3 |       2-----3 |       2-----3 |       2-----3 |
	 | 5---|-4	     | 5---|-4	     | 5---|-4	     | 5---|-4
	 |/    |/ 	     |/    |/ 	     |/    |/	     |/    |/
	 0-----1	     0-----1	     0-----1	     0-----1

	[1] C.R.Salama and Andreas Kolb, VMV2005,
		"A Vertex Program for Efficient Box-Plane Intersection"
*/
	
// paths for front/back vertices (3,5)
int ClipCube::s_path0[] = { 3,7,6,5 },
	ClipCube::s_path2[] = { 3,1,4,5 },
	ClipCube::s_path4[] = { 3,2,0,5 },
	ClipCube::s_path1[] = { 7,4 },
	ClipCube::s_path3[] = { 1,0 },
	ClipCube::s_path5[] = { 2,6 };

// permutation of indices for front/back cases (2,4), (0,7), (1,6)
// (note that case (a,b) is symmetric to (b,a))
int ClipCube::s_perm[8][8] =
{
	{ 6,5,2,0, 4,7,3,1 },  // (0,7)
	{ 5,4,0,1, 7,6,2,3 },  // (1,6)
	{ 5,0,6,2, 1,4,7,3 },  // (2,4)
	{ 0,1,2,3, 4,5,6,7 },  // (3,5)
	// symmetric cases (copies of above)
	{ 5,0,6,2, 1,4,7,3 },  // (2,4)
	{ 0,1,2,3, 4,5,6,7 },  // (3,5)
	{ 5,4,0,1, 7,6,2,3 },  // (1,6)
	{ 6,5,2,0, 4,7,3,1 },  // (0,7)
};


//==============================================================================
//	Utility functions
//==============================================================================

inline glm::mat4 gl2mat4( float* m, bool transpose=false )
{
	using namespace glm;
	//return mat4( vec4(&m[0]), vec4(&m[4]), vec4(&m[8]), vec4(&m[12]) );
	if( transpose )
		return mat4( m[0], m[4], m[ 8], m[12],
					 m[1], m[5], m[ 9], m[13],
					 m[2], m[6], m[10], m[14],
					 m[3], m[7], m[11], m[15] );
	else
		return mat4( m[ 0], m[ 1], m[ 2], m[ 3],
					 m[ 4], m[ 5], m[ 6], m[ 7],
					 m[ 8], m[ 9], m[10], m[11],
					 m[12], m[13], m[14], m[15] );
}

//==============================================================================
//	ClipCube implementation
//==============================================================================

//------------------------------------------------------------------------------
// C'tor
//------------------------------------------------------------------------------
ClipCube::ClipCube()
	: m_texcoord_scale( 1,1,1 ),
	  m_scale         ( 1,1,1 ),
	  m_color_like_texcoord(true),
	  m_vertices ( 8 ),
	  m_texcoords( 8 ),
	  m_colors   ( 8 )
{
	memcpy( m_faces, s_faces, sizeof(m_faces) );
	update();
}

//------------------------------------------------------------------------------
// Setters
//------------------------------------------------------------------------------
void ClipCube::set_scale( float sx, float sy, float sz )
{
	m_scale = Vec3( sx, sy, sz );
	update();
}

void ClipCube::set_texcoord_scale( float sx, float sy, float sz )
{
	m_texcoord_scale = Vec3( sx, sy, sz );
	update();
}

void ClipCube::set_color_like_texcoord( bool b )
{
	m_color_like_texcoord = b;
	update();
}

void ClipCube::update()
{
	for( int i=0; i < 8; ++i )
	{
		Vec3 v = Vec3( s_verts[i][0], s_verts[i][1], s_verts[i][2] );
		
		m_texcoords[i] = v * m_texcoord_scale;
		m_vertices [i] = v * m_scale;
		m_colors   [i] = m_color_like_texcoord ? m_texcoords[i] : v;
	}
}

//------------------------------------------------------------------------------
//  draw_rgbcube()
//------------------------------------------------------------------------------
void ClipCube::draw_rgbcube()
{
	glBegin( GL_QUADS );
	for( int i=0; i < 6; ++i )
		for( int j=0; j < 4; ++j )
		{
			int idx = m_faces[i][j];
			glColor3f( m_colors[idx].r, m_colors[idx].g, m_colors[idx].b );
			glTexCoord3f( m_texcoords[idx].x, m_texcoords[idx].y, m_texcoords[idx].z );
			glVertex3f( m_vertices[idx].x, m_vertices[idx].y, m_vertices[idx].z );
		}
	glEnd();
}

//------------------------------------------------------------------------------
//  draw_wireframe()
//------------------------------------------------------------------------------
void ClipCube::draw_wireframe()
{	
	glPushAttrib( GL_POLYGON_BIT );
	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	glBegin( GL_QUADS );
	for( int i=0; i < 6; ++i )
		for( int j=0; j < 4; ++j )
		{
			// no color & no texture coordinates
			int idx = m_faces[i][j];
			glVertex3f( m_vertices[idx].x, m_vertices[idx].y, m_vertices[idx].z );
		}
	glEnd();
	glPopAttrib();
}

//------------------------------------------------------------------------------
//  draw_nearclip()
//------------------------------------------------------------------------------
void ClipCube::draw_nearclip( float znear )
{
	using namespace glm;
	
	// get modelview matrix
	float mv[16];
	glGetFloatv( GL_MODELVIEW_MATRIX, mv );

	// compute viewing vector (use modelview transpose?!)
	vec4 viewdir = gl2mat4( mv, true ) * vec4( 0,0,1,1 );
	
	// distance unit cube to near plane
	vec4 cubecenter = gl2mat4(mv) * vec4( 0,0,0,1 );
	float cubedist = cubecenter.z;

	// viewer orthogonal clip plane
	vec3 n = vec3(viewdir.x,viewdir.y,viewdir.z);
	n = normalize(n);
	float d = -znear - cubedist;
	
	// get viewer nearest vertex
	int fi = get_front_vertex_index( mv );
	
	// compute intersection
	Vec3Array clip = get_plane_intersections( n, d, fi );

	// draw clipped polygon
//#define DEBUG_CLIPCUBE
#ifdef DEBUG_CLIPCUBE
	glPointSize( 16 );
	glColor4f( 1,0,0,1 );
	glBegin( GL_POINTS );
#else
	glBegin( GL_POLYGON );
#endif
	for( size_t i=0; i < clip.size(); ++i )  
	{
		Vec3 clipv = clip[i];

		// apply scalings / options
		Vec3 tc    = (clipv/m_scale) * m_texcoord_scale, 
		     v     = clipv,   // scale implicitly in clip vertex calculation
		     color = m_color_like_texcoord ? tc : clipv;		

#ifndef DEBUG_CLIPCUBE
		glColor3f( color.r, color.g, color.b );
#endif
		glTexCoord3f( tc.x, tc.y, tc.z );
		glVertex3f( v.x, v.y, v.z );
	}
	glEnd();	
}

//------------------------------------------------------------------------------
//  get_front_vertex_index()
//------------------------------------------------------------------------------
int ClipCube::get_front_vertex_index( float* modelview )
{
	using namespace glm;
	
	mat4 MV = gl2mat4(modelview);
	
	float max=-std::numeric_limits<float>::max();
	int imax=-1;
	for( int i=0; i < 8; ++i )
	{
		// eyecoords z is distance
		vec4 v( m_vertices[i], 1 );
		vec4 eyecoords = MV * v;
		float dist = eyecoords.z;
		if( dist > max ) 
		{
			max = dist;
			imax = i;
		}
	}
	assert( imax >= 0 ); // front index must exist!
	return imax;	
}

//------------------------------------------------------------------------------
//  get_plane_intersections()
//------------------------------------------------------------------------------
ClipCube::Vec3Array 
ClipCube::get_plane_intersections( ClipCube::Vec3 n, float d, int fvi )
{
	// permutation of vertex indices acc. to front-vertex
	int* perm = s_perm[3];  // our default is (3,5)
	if( fvi >= 0 )
		perm = s_perm[fvi];

	/* 
		Compute intersections 
	*/
	typedef std::vector<int>    Path;
	typedef std::vector< Path > PathArray;
	PathArray paths( 3 );             // unique cube paths
	PathArray edges( 3 );             // remaining cube edges
	Vec3Array clip; clip.reserve(6);  // computed intersection

	paths[0] = Path( s_path0, s_path0 + 4 );
	paths[1] = Path( s_path2, s_path2 + 4 );
	paths[2] = Path( s_path4, s_path4 + 4 );
	edges[0] = Path( s_path1, s_path1 + 2 );
	edges[1] = Path( s_path3, s_path3 + 2 );
	edges[2] = Path( s_path5, s_path5 + 2 );

	// compute first intersections along paths
	for( PathArray::const_iterator ip=paths.begin(); ip!=paths.end(); ++ip )
	{
		// traverse edges along path
		for( size_t i=0; i < ip->size()-1; ++i )
		{
			// indices of edge vertices
			int i0 = perm[ ip->at(i)   ],
				i1 = perm[ ip->at(i+1) ];

			// edge vertices
			Vec3 v0 = m_vertices[i0],
				 v1 = m_vertices[i1];

			// compute intersection
			float lambda;
			lambda = intersect_plane_edge( n, d, v0, (v1-v0) );			
			if( lambda >= 0 && lambda <= 1 )
			{
				// intersection found:  (-> stop traversing current path)
				clip.push_back( v0 + lambda*(v1-v0) );
				continue;
			}			
		}
	}

	// compute (possible) missing intersection on remaining edges
	int path_index;
	PathArray::const_iterator ip;
	for( path_index=0, ip=edges.begin(); ip!=edges.end(); 
		 ++ip, ++path_index )
	{
		// ASSUME: single edge per path, 3 missing edges		
		assert( ip->size() == 2 );
		assert( path_index < 3 );

		// intersection computation the same as above but with the difference
		// that found intersection is not appended to clip array but inserted
		// at the correct position
		{
			// indices of edge vertices
			int i0 = perm[ ip->at(0) ], //ip->at(i),
				i1 = perm[ ip->at(1) ]; //ip->at(i+1);

			// edge vertices
			Vec3 v0 = m_vertices[i0], //( s_verts[i0][0], s_verts[i0][1], s_verts[i0][2] ),
				 v1 = m_vertices[i1]; //( s_verts[i1][0], s_verts[i1][1], s_verts[i1][2] );

			// compute intersection
			float lambda;
			lambda = intersect_plane_edge( n, d, v0, (v1-v0) );
			if( lambda >= 0 && lambda <= 1 )
			{
				// intersection found
				Vec3 intersection = v0 + lambda*(v1-v0);

				// insert after corresponding path intersection vertex
				// (count from end of clip array)
				clip.insert( clip.end()-(2-path_index), intersection );
			}
		}
	}

	return clip;	
}

//------------------------------------------------------------------------------
//  intersect_plane_edge()
//------------------------------------------------------------------------------
float ClipCube::intersect_plane_edge( Vec3 n, float d, Vec3 v, Vec3 e )
{
	return (d - glm::dot( n, v )) / glm::dot( n, e );
}
