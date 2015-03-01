#ifndef GEOMETRY2_H
#define GEOMETRY2_H

// First approach was to use vectors of vec3 and Face type, but the corresponding
// float/int data is not tightly packed in memory because class/struct overhead.
// (Leave commented out!)
//#define GEOMETRY2_NO_BUFFER_SUPPORT

#include "Vector.h"  // Geometry::vec3
#include <vector>

//==============================================================================
//	SimpleGeometry
//==============================================================================

/// Indexed face set in linear tightly packed vertex/index buffers
/// Restricted to triangle faces and vertex normals.
/// Internally getters map to more convenient custom vector/face type.
class SimpleGeometry
{
public:
	/// Return number of vertices/normals
	int num_vertices() const;
	/// Return number of faces
	int num_faces()    const;

	///@{ Get pointer to tightly packed buffer
	float* get_vertex_ptr();
	float* get_normal_ptr();
	int*   get_index_ptr();
	///@}

	void clear();

//protected:
	/// Reserve memory for vertices
	void reserve_vertices( int n );
	/// Reserve memory for face indices
	void reserve_faces( int n );
	
	/// Triangle face
	/// Assume identical indices for vertices/normals
	struct Face 
	{ 
		Face();
		Face( int i[3] );
		Face( int i, int j, int k );
		int vi[3];
	};
	
	Geometry::vec3 get_vertex( int i );
	Face get_face( int i );
	
	/// Insert face
	int  add_face( Face f );

	/// Insert vertex with normal, returns vertex index
	/// (Yes, you *have* to supply a normal :-)
	int add_vertex_and_normal( Geometry::vec3 v, Geometry::vec3 n );
	
private:
#ifndef GEOMETRY2_NO_BUFFER_SUPPORT
	std::vector<float> m_vdata;
	std::vector<float> m_ndata;	 // vertex normals
	std::vector<int>   m_fdata;
#else
	std::vector<Geometry::vec3> m_vertices;
	std::vector<Geometry::vec3> m_normals;
	std::vector<Geometry::Face> m_faces;
#endif
};

//==============================================================================
//	Icosahedron
//==============================================================================

/// Icosahedron with subdivision, nice regular sphere approximation
class Icosahedron : public SimpleGeometry
{
public:
	Icosahedron():
		m_levels(4),
		m_platonicConstantX(.525731112119133606),
		m_platonicConstantZ(.850650808352039932)
		{}

	void setPlatonicConstants( double X, double Z )
		{
			m_platonicConstantX = X;
			m_platonicConstantZ = Z;
		}
	double getPlatonicConstantsX() const { return m_platonicConstantX; }
	double getPlatonicConstantsZ() const { return m_platonicConstantZ; }

	void setLevels( int levels ) 
		{ 
			m_levels = levels; 
			if( m_levels <= 0 ) m_levels=1;
		};
	int getLevels() const { return m_levels; }

	/// Create an Icosahedron model where each face is subdivided level times
	/// In the limit the subdivision surface is a sphere.
	void create( int level=-1 );

protected:
	/// Recursive face subdivision routine
	/// For levels=0 the face is inserted into the model
	void add_face_subdivision( Face f, int levels );

private:
	int    m_levels;
	double m_platonicConstantX,
		   m_platonicConstantZ;
};

#endif
