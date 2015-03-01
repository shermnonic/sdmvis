#ifndef GLSLPROGRAM_H
#define GLSLPROGRAM_H

#include "GLConfig.h"
#include <iostream>
#include <string>

#ifndef GL_GEOMETRY_SHADER
#pragma message("Warning: Define GL_GEOMETRY_SHADER not found!")
#define GL_GEOMETRY_SHADER 42
#endif

#ifdef GL_NAMESPACE
namespace GL {
#endif

/** 
	GLSL Program

	For classical vertex and fragment shader programs you may use the \a load()
	convenience functions. Supports geometry but no tesselation shaders yet.
	To use a geometry shader you have to specify WITH_GEOMETRY_SHADER on
	construction.

	TODO: Make geometry shader optional for older hardware.
*/
class GLSLProgram
{
public:
	/// Constructor options (bitmasks which can be or'ed together)
	enum Options { WITH_GEOMETRY_SHADER=1 };

	GLSLProgram( int opt=0 );
	~GLSLProgram();

	/// Read textfile from disk.
	/// Returns NULL on error, else new char buffer (caller is responsible
	/// to free buffer via delete[])
	static char* read_shader_from_disk( const char* filename );

	///@{ Load and compile GLSL program consisting of vertex & fragment shader
	bool load( const GLchar** vShaderSrc, const GLchar** fShaderSrc );
	bool load( const std::string& vertSrc, const std::string& fragSrc );
	bool load_from_disk( const char* vs_filename, const char* fs_filename );
	///@}

	/// Convenience function for std::string support
	bool shaderSource( GLenum type, const std::string& src );

	/// Same as glUseProgram(program)
	void bind()    const;
	/// Same as glUseProgram(0)
	void release() const;
	/// Same as glUseProgram(0)
	static void release_all();

	///@{ Thin wrappers for OpenGL shader program functions
	bool shaderSource( GLenum type, GLsizei count, const GLchar** src, GLint* length );
	bool shaderSource( GLenum type, const GLchar** src );

	bool compileShader( GLuint shader  );
	bool linkProgram  ( GLuint program );
	
	std::string getShaderLog ( GLuint shader  );
	std::string getProgramLog( GLuint program );

    GLint getUniformLocation( const GLchar* name );
	///@}

	/// Redirect log output
	void redirectLog( std::ostream& os ) { m_log = &os; }

protected:
	std::ostream* m_log;  ///< pointer to output stream used for log messages

private:
	int m_opt; ///< options bitmask as passed to the constructor

	GLuint m_program;  ///< GLSL program handle
	GLuint m_fShader;  ///< fragment shader handle
	GLuint m_vShader;  ///< vertex shader handle
	GLuint m_gShader;  ///< geometry shader handle

	/// copy assignment not allowed
	GLSLProgram& operator= ( const GLSLProgram& other );
	/// copy constructor not allowed
	GLSLProgram( const GLSLProgram& other );
};

#ifdef GL_NAMESPACE
} // namespace GL 
#endif

#endif // GLSLPROGRAM_H
