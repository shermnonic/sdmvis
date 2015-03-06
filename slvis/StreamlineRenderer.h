#ifndef STREAMLINERENDERER_H
#define STREAMLINERENDERER_H

#include <string>
#include <vector>
#include <map>

// Forwards
namespace GL {
	class GLTexture;
	class GLSLProgram;
}

/**
	Geometry shader to render SDM streamlines projected onto an isosurface.
	
	Seed points for streamlines are rendered outside this class. In this class
	integration of streamlines is then performed in the geometry shader. 
	Stationary velocity fields, or an affine combination of multiple ones, are	
	integrated on-the-fly. The integrated trajectory is projected locally on an 
	isosurface of a given scalar volume.

	@TODO Refactor StreamlineRenderer to StreamlineShader ?

	@author Max Hermann (hermann@cs.uni-bonn.de), March 2015
*/ 
class StreamlineRenderer
{
public:	
	StreamlineRenderer();

	bool initGL();
	void destroyGL();

	void bind();
	void release();

	void reloadShadersFromDisk();

	void setVolume( GL::GLTexture* tex );
	void setWarpfield( GL::GLTexture* tex );

	GL::GLTexture* getVolume() { return m_texVolume; }
	GL::GLTexture* getWarpfield() { return m_texWarpfield; }
	
	void setIsovalue( float iso ) { m_isovalue = iso; }
	float getIsovalue() const { return m_isovalue; }


private:
	std::map< std::string, int > m_uniforms; ///< Uniform locations

	GL::GLSLProgram* m_program;
	GL::GLTexture* m_texVolume;
	GL::GLTexture* m_texWarpfield;

	float m_isovalue; ///< Isovalue to trace in texVolume
};

#endif // STREAMLINERENDERER_H
