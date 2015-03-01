// Max Hermann, March 22, 2010
#ifndef RENDERTOTEXTURE_H
#define RENDERTOTEXTURE_H
#include "GLConfig.h"

// Use RenderBuffer for depth, but this doesn't work for me.
// If not defined, a usual depth texture will be employed.
//#define RENDERTOTEXTURE_DEPTHBUFFER_AS_RENDERBUFFER

// Combined stencil and depth attachement untested yet, leave this define
// uncommented for pure depth buffer.
#define RENDERTOTEXTURE_NO_STENCILBUFFER


/// Simple wrapper for OpenGL calls which render to TEXTURE_2D target(s).
class RenderToTexture
{
public:
	RenderToTexture(): m_initialized(false),m_width(0),m_height(0),
					   m_fbo(0),m_depth(0)
	{}

	bool init( int width, int height, GLint texid, bool attach_depth );
	bool init( int width, int height, GLint* texids, int numColorAttachements, 
	                                               bool attach_depth );
	bool deinit();
	bool bind( GLint texid, bool bind_depth=false );
	bool bind( GLint* texids, int numColorAttachements, bool bind_depth=false );
	bool unbind();

#ifndef RENDERTOTEXTURE_DEPTHBUFFER_AS_RENDERBUFFER
	GLint getDepthTex() const { return m_depth; }
#endif
	
protected:
	bool attachTextures( GLint* texids, int numColorAttachements );
	bool createDepthBuffer( int width, int height );

private:
	bool   m_initialized;
	int    m_width, m_height;  ///< texture and buffers must be of same size
	bool   m_depth_attached;   ///< is Renderbuffer for depth used ?
	GLuint m_fbo;              ///< Framebuffer object
	GLuint m_depth;            ///< Renderbuffer / texture for depth
};	

#endif

