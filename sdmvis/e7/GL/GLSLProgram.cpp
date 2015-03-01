#include "GLSLProgram.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cassert>

using namespace std;
using namespace GL;

//-----------------------------------------------------------------------------
// Convenience functions
//-----------------------------------------------------------------------------

// internal helper function
char* GLSLProgram::read_shader_from_disk( const char* filename )
{
	ifstream is( filename );
	if( is.good() )
	{	
		is.seekg(0,ios::end);
		int len = is.tellg();
		is.seekg(0,ios::beg);

		char* buf = new char[len+1];
		memset(buf,0,len+1);

		is.read(buf,len);
		is.close();

		//cout <<"[SHADER BEGIN]"<< endl << buf << endl <<"[SHADER END]"<< endl;
		return buf;
	}

	return NULL;
}

bool GLSLProgram::load_from_disk( const char* vs_filename, const char* fs_filename )
{
	// load from disk
	char* vertex_shader   = read_shader_from_disk( vs_filename );
	char* fragment_shader = read_shader_from_disk( fs_filename );
	if( !vertex_shader || !fragment_shader )
	{
		cerr << "Error: Failed to load GLSL shader files '"
			 << vs_filename << "' and '"
			 << fs_filename << "'!" << endl;
		if( vertex_shader   ) delete [] vertex_shader;
		if( fragment_shader ) delete [] fragment_shader;
		return false;
	}

	// compile
	bool success = load( vertex_shader, fragment_shader );

	delete [] vertex_shader;
	delete [] fragment_shader;
	return success;
}

bool GLSLProgram::load( const std::string& vertSrc, const std::string& fragSrc )
{
	return  shaderSource( GL_VERTEX_SHADER  , vertSrc ) &&
	        shaderSource( GL_FRAGMENT_SHADER, fragSrc ) &&
		    compileShader( m_vShader ) &&
			compileShader( m_fShader ) &&
			linkProgram( m_program );
}

bool GLSLProgram::shaderSource( GLenum type, const std::string& src )
{
	GLint size = src.size();
	GLchar* data = new GLchar[size+1];
	memset( data, 0, size+1 );
	memcpy( data, src.data(), size );
	bool ret = shaderSource( type, 1, (const GLchar**)&data, &size );
	delete [] data;
	return ret;
}

//-----------------------------------------------------------------------------
// GLSLProgram
//-----------------------------------------------------------------------------

GLSLProgram::GLSLProgram( int opt )
: m_opt(opt), m_log(&cout)  // default output stream for log messages
{
	m_program = glCreateProgram();
	m_fShader = glCreateShader( GL_FRAGMENT_SHADER );	
	m_vShader = glCreateShader( GL_VERTEX_SHADER );
	
	// it is absolute valid to attach a shader object to a program object before
	// source code has been loaded into the shader object
	glAttachShader( m_program, m_fShader );
	glAttachShader( m_program, m_vShader );

	if( m_opt & WITH_GEOMETRY_SHADER )
	{
		m_gShader = glCreateShader( GL_GEOMETRY_SHADER );
		glAttachShader( m_program, m_gShader );
	}
}

GLSLProgram::~GLSLProgram()
{
	// shader must be detached before deletion
	glDetachShader( m_program, m_vShader );
	glDetachShader( m_program, m_fShader );
	
	glDeleteShader( m_vShader );
	glDeleteShader( m_fShader );	

	if( m_opt & WITH_GEOMETRY_SHADER )
	{
		glDetachShader( m_program, m_gShader );
		glDeleteShader( m_gShader );
	}

	glDeleteProgram( m_program );
}

bool GLSLProgram::load( const GLchar** vShaderSrc, const GLchar** fShaderSrc  )
{
	return shaderSource( GL_VERTEX_SHADER  , vShaderSrc ) &&
	       shaderSource( GL_FRAGMENT_SHADER, fShaderSrc ) &&	
	       compileShader( m_fShader ) && 
	       compileShader( m_vShader ) &&
	       linkProgram( m_program );
}

bool GLSLProgram::shaderSource( GLenum type, const GLchar** src )
{
	return shaderSource( type, 1, src, 0 );
}

bool GLSLProgram::shaderSource( GLenum type, GLsizei count, const GLchar** src, GLint* length )
{
	assert( src );
	GLuint shader;
	switch( type )
	{
	case GL_VERTEX_SHADER  : shader = m_vShader; break;
	case GL_FRAGMENT_SHADER: shader = m_fShader; break;
	case GL_GEOMETRY_SHADER: shader = m_gShader; break;
	default:
		cerr << "Error: Unsupported shader type " << type << "!" << endl;
		return false;
	}
	
	glShaderSource( shader, count, src, length );
	return true;
}

void GLSLProgram::bind() const
{
	glUseProgram( m_program );	
}

void GLSLProgram::release() const
{
	glUseProgram( 0 );
}

void GLSLProgram::release_all()
{
	glUseProgram( 0 );
}

GLint GLSLProgram::getUniformLocation( const GLchar* name )
{	
	return glGetUniformLocation( m_program, name );
}

string GLSLProgram::getShaderLog( GLuint shader )
{
	int len;          // length of info log
	int lenWritten;   // chars written
	stringstream ss;  // stringstream used for copying char buffer to a string
	
	glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &len );
	
	if( len > 0 )
	{
		char* log = new char[ len ];
		
		glGetShaderInfoLog( shader, len, &lenWritten, log );
		
		ss << log;
		
		delete [] log;
	}
	
	return ss.str();
}

string GLSLProgram::getProgramLog( GLuint program )
{
	int len;          // length of info log
	int lenWritten;   // chars written
	stringstream ss;  // stringstream used for copying char buffer to a string
	
	glGetProgramiv( program, GL_INFO_LOG_LENGTH, &len );
	
	if( len > 0 )
	{
		char* log = new char[ len ];
		
		glGetProgramInfoLog( program, len, &lenWritten, log );
		
		ss << log;
		
		delete [] log;
	}

	return ss.str();
}

bool GLSLProgram::compileShader( GLuint shader )
{
	int compileSuccess;
	
	glCompileShader( shader );	
	glGetShaderiv( shader, GL_COMPILE_STATUS, &compileSuccess );
	
	if( !compileSuccess )
	{
		*m_log << getShaderLog( shader ) << endl;
		return false;
	}
	
	return true;
}

bool GLSLProgram::linkProgram( GLuint program )
{
	int linkSuccess;
	
	glLinkProgram( program );
	glGetProgramiv( program, GL_LINK_STATUS, &linkSuccess );
	if( !linkSuccess )
	{
		*m_log << getProgramLog( program ) << endl;
		return false;
	}
	
	return true;
}
