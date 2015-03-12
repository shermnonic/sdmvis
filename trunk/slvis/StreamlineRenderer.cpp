#include "StreamlineRenderer.h"
#include "glbase.h"
#include <GL/GLSLProgram.h>
#include <GL/GLTexture.h>
#include <GL/GLError.h>
#include <iostream>
#include <string>
#include <sstream>

using std::cerr;
using std::endl;
using GL::GLSLProgram;

//------------------------------------------------------------------------------
//  string utility function
//------------------------------------------------------------------------------

int string_replace( std::string& text, std::string marker, const char* replacement );
int string_replace( std::string& s, std::string marker, std::string replacement );

int string_replace( std::string& text, std::string marker, const char* replacement )
{
	std::string repl_s( replacement );
	return string_replace( text, marker, repl_s );
}

int string_replace( std::string& s, std::string marker, std::string replacement )
{
	int num_replacements = 0;
	std::size_t pos = s.find( marker );
	while( pos != std::string::npos )
	{
		s.replace( pos, marker.length(), replacement );		
		num_replacements++;
		pos = s.find( marker );
	}
	return num_replacements;
}

std::string string_from_number( int i )
{
	std::stringstream ss;
	ss << i;
	return ss.str();
}

//------------------------------------------------------------------------------
//  StreamlineRenderer
//------------------------------------------------------------------------------

StreamlineRenderer::StreamlineRenderer( int mode )
: m_program(NULL),
  m_texVolume(NULL),
  m_texWarpfield(NULL),
  m_isovalue(0.f),
  m_timescale(1.f)
{
	setMode(mode);
}

void StreamlineRenderer::setMode( int mode )
{
	m_mode = mode;
}

bool StreamlineRenderer::initGL()
{
	if( m_program ) delete m_program;
	m_program = new GLSLProgram( GLSLProgram::WITH_GEOMETRY_SHADER );
	return true;
}

void StreamlineRenderer::destroyGL()
{
	if( m_program )
	{
		delete m_program;
		m_program = NULL;
	}
}

void StreamlineRenderer::bind()
{
	if( !m_program ) return; // Sanity
	m_program->bind();

	if( m_uniforms["voltex"] && m_texVolume )
	{
		glUniform1i( m_uniforms["voltex"], 1 );
		m_texVolume->Bind( 1 );
	}

	if( m_uniforms["warpfield"] && m_texWarpfield )
	{
		glUniform1i( m_uniforms["warpfield"], 0 );
		m_texWarpfield->Bind( 0 );
	}

	if( m_uniforms["isovalue"] )
	{
		glUniform1f( m_uniforms["isovalue"], m_isovalue );
	}

	if( m_uniforms["timescale"] )
	{
		glUniform1f( m_uniforms["timescale"], m_timescale );
	}
	
	GL::checkGLError("StreamlineRenderer::bind()");
}

void StreamlineRenderer::release()
{
	m_program->release();

	if( m_texVolume )
	{
		glActiveTexture( GL_TEXTURE1 );
		m_texVolume->Unbind();
	}

	if( m_texWarpfield )
	{
		glActiveTexture( GL_TEXTURE0 );
		m_texWarpfield->Unbind();
	}

	GL::checkGLError("StreamlineRenderer::release()");
}

std::string read_shader( const char* filename )
{
	char* buf = GL::GLSLProgram::read_shader_from_disk( filename );
	if( !buf )
		return "";
	std::string source( buf );
	delete [] buf;
	return source;
}

void StreamlineRenderer::reloadShadersFromDisk()
{
	if( !m_program ) return; // Sanity

	using std::string;

	string filenames[3] = 
	{
		"shader/streamline.vs.glsl",
		"shader/streamline.gs.glsl",
		"shader/streamline.fs.glsl" 
	};
	enum SourceTypes { 
		VertexShader, 
		GeometryShader, 
		FragmentShader 
	};

	// Reset
	m_uniforms.clear();

	// Load and preprocess	
	string sources[3];
	for( int i=0; i < 3; i++ )
	{
		// Load source code
		string source = read_shader( filenames[i].c_str() );
		if( source.empty() )
		{
			cerr << "StreamlineRenderer::reloadShadersFromDisk() : "
				"Loading streamline shaders failed!" << endl;
			return;
		}

		// Preprocess	
		string_replace( source, "<__opt_MODE__>", string_from_number(m_mode) );

		// Store
		sources[i] = source;
	}

	// Load and compile
	if( !m_program->load( 
			sources[VertexShader], 
			sources[GeometryShader], 
			sources[FragmentShader] ) )
	{
		cerr << "StreamlineRenderer::reloadShadersFromDisk() : "
			"Compilation/linking of streamline shaders failed!" << endl;
		return;
	}

	// Uniform locations
	string uniformNames[4] =
	{
		"voltex",
		"warpfield",
		"isovalue",
		"timescale"
	};
	for( int i=0; i < 4; i++ )
		m_uniforms[uniformNames[i]] = m_program->getUniformLocation(uniformNames[i].c_str());

	GL::checkGLError("StreamlineRenderer::reloadShadersFromDisk()");
}

void StreamlineRenderer::setVolume( GL::GLTexture* tex )
{
	m_texVolume = tex;
}

void StreamlineRenderer::setWarpfield( GL::GLTexture* tex )
{
	m_texWarpfield = tex;
}
