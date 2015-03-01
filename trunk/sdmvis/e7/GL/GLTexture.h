#ifndef GL_GLTEXTURE_H
#define GL_GLTEXTURE_H
#include "GLConfig.h"
#include <stdexcept>
#include <cassert>

#ifdef GL_NAMESPACE
namespace GL
{
#endif

class GLTexture
{
public:
	// Extension
	GLTexture();
#ifndef GL_USE_REFCOUNT
	~GLTexture();
#endif

	bool Create(GLenum target = GL_TEXTURE_2D);
	bool Destroy();
	bool Bind( int texunit=-1 ); // -1 implies not to call glActiveTexture
	void Unbind();
	bool Image(GLint level, GLint internalformat,
		GLsizei width, GLsizei height, GLsizei depth, GLint border,
		GLenum format, GLenum type, void *data);
	bool Image(GLint level, GLint internalformat,
		GLsizei width, GLsizei height, GLint border,
		GLenum format, GLenum type, void *data);
	bool Image(GLint level, GLint internalformat,
		GLsizei width, GLint border,
		GLenum format, GLenum type, void *data);
	bool SubImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
		GLsizei width, GLsizei height, GLsizei depth,
		GLenum format, GLenum type, void *data, bool bind = true );
	bool SubImage(GLint level, GLint xoffset, GLint yoffset,
		GLsizei width, GLsizei height,
		GLenum format, GLenum type, void *data, bool bind = true );
	bool SubImage(GLint level, GLint xoffset,
		GLsizei width, GLenum format, GLenum type, void *data, bool bind = true );
	bool SetParameter(GLenum pname, GLint value);
	bool SetParameter(GLenum pname, GLfloat value);
	bool ActivateLinearMagnification();
	GLenum Target() const;
	GLuint Name() const;
#ifdef GL_USE_REFCOUNT
	void swap(GLTexture &tex) { m_texture.swap(tex.m_texture); }
#else
	void swap(GLTexture &tex) 
	{ 
		// default copy constructors sufficient
		GLTextureRef tmp = *m_texture;
		*m_texture       = *tex.m_texture;
		*tex.m_texture   = tmp;
		// swap members
		std::swap( m_width , tex.m_width  );
		std::swap( m_height, tex.m_height );
		std::swap( m_depth , tex.m_depth  );
	}
#endif

// Extensions
	///@{ Get texture size (set on call to \a Image()) 
	int GetWidth () const { return m_width;  }
	int GetHeight() const { return m_height; }
	int GetDepth () const { return m_depth;  }
	///@}
	GLint GetID() const { assert(m_texture); return m_texture->m_id; }
	void SetWrapMode( GLint mode );
	void SetFilterMode( GLint mode );
	void SetFilterMode( GLint min_mode, GLint mag_mode );

protected:
	// only used internally
	void SetSize( GLint w, GLint h, GLint d )
	{
		m_width=w; m_height=h; m_depth=d;
	}

private:
#ifdef GL_USE_REFCOUNT
	class GLTextureRefCount
	: public Misc::RefCount
	{
	public:
		class bad_delete
		: public std::logic_error
		{
		public:
			bad_delete() : std::logic_error("Failed to delete GLTexture") {}
		};
		GLTextureRefCount();
		~GLTextureRefCount();
		GLuint m_id;
		GLenum m_target;
	};
	Misc::RefCountPtr< GLTextureRefCount > m_texture;
#else
	struct GLTextureRef
	{
		GLTextureRef(): m_id(0) {}
		GLuint m_id;
		GLenum m_target;
	};
	GLTextureRef* m_texture;
#endif

	// Extensions
	GLint m_width, m_height, m_depth;  ///< texture size according to last \a Image() call
};

#ifdef GL_NAMESPACE
};
#endif

namespace std
{
inline void swap(GL::GLTexture &a, GL::GLTexture &b)
{
	a.swap(b);
}
}

#endif
