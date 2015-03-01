// Max Hermann, June 2, 2010
#ifndef CLIPCUBE_H
#define CLIPCUBE_H

#include <glm/glm.hpp>  // GLM template library for vector/matrix types
#include <vector>

/**
	\class ClipCube

	Draw RGB colored cube for direct volume rendering.
	Provides near plane clipping.

	Issues:
	- vertex ordering neither strictly CW nor CCW
	- draw_nearplane() doesn't work when glScale() is applied to Modelview
*/
class ClipCube
{
public:
	ClipCube();

	void draw_rgbcube();
	void draw_wireframe();	
	void draw_nearclip( float znear );

	void set_scale( float sx, float sy, float sz );
	void set_texcoord_scale( float sx, float sy, float sz );
	void set_color_like_texcoord( bool b );

protected:
	/// Update vertex/color/texcoord, called internally in setters
	void update();
	
	typedef glm::vec3              Vec3;      ///< Vector type
	typedef std::vector<glm::vec3> Vec3Array; ///< Vector array type
	
	/// Returns index of vertex nearest to viewer
	int       get_front_vertex_index( float* modelview );

	/// Returns array of intersections between plane and unit cube
	/// @param n plane normal
	/// @param d plane distance
	/// @param fvi front vertex index (see \a get_front_vertex_index())
	Vec3Array get_plane_intersections( Vec3 n, float d, int fvi );

	/// Intersect plane with edge
	/// @param n plane normal
	/// @param d plane distance
	/// @param v edge base vertex
	/// @param e edge vector
	/// @return intersection lambda, where vertex = v*lambda+e if 0<=lambda<=1
	float intersect_plane_edge( Vec3 n, float d, Vec3 v, Vec3 e );

private:
	Vec3      m_texcoord_scale,
	          m_scale;
	bool      m_color_like_texcoord;

	Vec3Array m_vertices,
	          m_texcoords,
	          m_colors;
	int       m_faces[8][4];
	
	static float s_verts[8][3];
	static int   s_faces[6][4];
	static int s_path0[4], s_path1[2],
	           s_path2[4], s_path3[2],
	           s_path4[4], s_path5[2];
	static int s_perm[8][8];
};

#endif // CLIPCUBE_H
