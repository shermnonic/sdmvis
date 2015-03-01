#include "GLTexture.h"
#include "GLError.h"
#ifdef _WIN32
#include <windows.h>   // used to check if Gl context is active
#endif
#include <cassert>
#include <iostream>
#include <iomanip>

#define GL_GLTEXTURE_UNDEFINED_DIMENSION_SIZE 0

#ifdef GL_NAMESPACE
using namespace GL;
#endif

GLTexture::GLTexture()
: m_width(0),m_height(0),m_depth(0)
{
#ifdef GL_USE_REFCOUNT
	m_texture.New< GLTextureRefCount >();
#else
	m_texture = new GLTextureRef;
  #ifdef DEBUG
	std::cout << "GLTexture::GLTexture() new m_texture = 0x" << std::hex << m_texture << std::endl;
  #endif
#endif
}

#ifndef GL_USE_REFCOUNT
GLTexture::~GLTexture()
{
  #ifdef DEBUG
	std::cout << "GLTexture::GLTexture() delete m_texture = 0x" << std::hex << m_texture << std::endl;
  #endif
	delete m_texture; m_texture=NULL;
}
#endif

// Convenience extensions
void GLTexture::SetWrapMode( GLint wrapMode )
{
	switch( m_texture->m_target )
	{
	case GL_TEXTURE_3D:
		SetParameter( GL_TEXTURE_WRAP_R, wrapMode );
	default:
	case GL_TEXTURE_2D:
		SetParameter( GL_TEXTURE_WRAP_T, wrapMode );
	case GL_TEXTURE_1D:
		SetParameter( GL_TEXTURE_WRAP_S, wrapMode );
	}
}

void GLTexture::SetFilterMode( GLint mode )
{
	// Set same filtering for MAG/MIN
	SetFilterMode( mode, mode );
}

void GLTexture::SetFilterMode( GLint min_mode, GLint mag_mode )
{
	SetParameter( GL_TEXTURE_MIN_FILTER, min_mode );
	SetParameter( GL_TEXTURE_MAG_FILTER, mag_mode );	
}
// End: Convenience extensions


bool GLTexture::Create(GLenum target)
{
	glGenTextures(1, &m_texture->m_id);
	m_texture->m_target = target;

	// Convenience: Set default parameters
	SetWrapMode( GL_CLAMP );
	SetFilterMode( GL_LINEAR );

	return CheckGLError("GLTexture::Create()");
}

bool GLTexture::Destroy()
{
#ifdef GL_USE_REFCOUNT
	m_texture = (GLTextureRefCount *)NULL;
#else
	assert(m_texture);
	glDeleteTextures(1, &m_texture->m_id);
	//delete m_texture; m_texture=NULL; //--> FIX: why should we delete m_texture here?
	                                    //         moved this to d'tor!
#endif
	return true;
}

bool GLTexture::Bind( int texunit )
{
	assert(m_texture);
	if( texunit >= 0 )
		glActiveTexture( GL_TEXTURE0 + texunit );
	glBindTexture(m_texture->m_target, m_texture->m_id);
	return CheckGLError("GLTexture::Bind()");
}

void GLTexture::Unbind()
{
	assert(m_texture);
	glBindTexture(m_texture->m_target, 0);
}

bool GLTexture::Image(GLint level, GLint internalformat,
	GLsizei width, GLsizei height, GLsizei depth, GLint border,
	GLenum format, GLenum type, void *data)
{
	assert(m_texture);
	if(!Bind()) return false;
	glTexImage3D(m_texture->m_target, level, internalformat, width, height,
		depth, border, format, type, data);
	SetSize( width, height, depth );
	return CheckGLError("GLTexture::Image()");
}

bool GLTexture::Image(GLint level, GLint internalformat,
	GLsizei width, GLsizei height, GLint border,
	GLenum format, GLenum type, void *data)
{
	assert(m_texture);
	if(!Bind()) return false;
	glTexImage2D(m_texture->m_target, level, internalformat, width, height,
		border, format, type, data);
	SetSize( width, height, GL_GLTEXTURE_UNDEFINED_DIMENSION_SIZE );
	return CheckGLError("GLTexture::Image()");
}

bool GLTexture::Image(GLint level, GLint internalformat,
	GLsizei width, GLint border, GLenum format, GLenum type, void *data)
{
	assert(m_texture);
	if(!Bind()) return false;
	glTexImage1D(m_texture->m_target, level, internalformat, width,
		border, format, type, data);
	SetSize( width, GL_GLTEXTURE_UNDEFINED_DIMENSION_SIZE, GL_GLTEXTURE_UNDEFINED_DIMENSION_SIZE );
	return CheckGLError("GLTexture::Image()");
}

bool GLTexture::SubImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
	GLsizei width, GLsizei height, GLsizei depth,
	GLenum format, GLenum type, void *data, bool bind )
{
	assert(m_texture);
	if( bind )
		if(!Bind()) return false;
	glTexSubImage3D(m_texture->m_target, level, xoffset, yoffset, zoffset,
		width, height, depth, format, type, data);
	return CheckGLError("GLTexture::SubImage()");
}

bool GLTexture::SubImage(GLint level, GLint xoffset, GLint yoffset,
	GLsizei width, GLsizei height,
	GLenum format, GLenum type, void *data, bool bind )
{
	assert(m_texture);
	if( bind )
		if(!Bind()) return false;
	glTexSubImage2D(m_texture->m_target, level, xoffset, yoffset,
		width, height, format, type, data);
	return CheckGLError("GLTexture::SubImage()");
}

bool GLTexture::SubImage(GLint level, GLint xoffset,
	GLsizei width, GLenum format, GLenum type, void *data, bool bind )
{
	assert(m_texture);
	if( bind )
		if(!Bind()) return false;
	glTexSubImage1D(m_texture->m_target, level, xoffset,
		width, format, type, data);
	return CheckGLError("GLTexture::SubImage()");
}

bool GLTexture::SetParameter(GLenum pname, GLint value)
{
	assert(m_texture);
	Bind();
	glTexParameteri(m_texture->m_target, pname, value);
	return CheckGLError("GLTexture::SetParameter()");
}

bool GLTexture::SetParameter(GLenum pname, GLfloat value)
{
	assert(m_texture);
	Bind();
	glTexParameterf(m_texture->m_target, pname, value);
	return CheckGLError("GLTexture::SetParameter()");
}

bool GLTexture::ActivateLinearMagnification()
{
	return SetParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

GLenum GLTexture::Target() const
{
	assert(m_texture);
	return m_texture->m_target;
}

GLuint GLTexture::Name() const
{
	assert(m_texture);
	return m_texture->m_id;
}

#ifdef GL_USE_REFCOUNT
GLTexture::GLTextureRefCount::GLTextureRefCount()
: m_id(0)
, m_target(0)
{}

GLTexture::GLTextureRefCount::~GLTextureRefCount()
{
	if(m_id)
	{
#ifdef _WIN32
		if(::wglGetCurrentContext() == NULL)
			return;
#endif
		glDeleteTextures(1, &m_id);
		if(glGetError() != GL_NO_ERROR)
			throw bad_delete();
	}
}
#endif // GL_USE_REFCOUNT

