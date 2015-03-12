#include "StreamlineRenderer.h"
#include "glbase.h"
#include <GL/GLSLProgram.h>
#include <GL/GLTexture.h>
#include <GL/GLError.h>
#include <iostream>

using std::cerr;
using std::endl;
using GL::GLSLProgram;

StreamlineRenderer::StreamlineRenderer()
: m_program(NULL),
  m_texVolume(NULL),
  m_texWarpfield(NULL),
  m_isovalue(0.f)
{
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

void StreamlineRenderer::reloadShadersFromDisk()
{
	if( !m_program ) return; // Sanity

	// Reset
	m_uniforms.clear();

	// Load and compile
	if( !m_program->load_from_disk( 
		"shader/streamline.vs.glsl",
		"shader/streamline.gs.glsl",
		"shader/streamline.fs.glsl" ) )
	{
		cerr << "StreamlineRenderer::reloadShadersFromDisk() : "
			"Reloading streamline shaders failed!" << endl;
		return;
	}

	// Uniform locations
	m_uniforms["voltex"   ] = m_program->getUniformLocation("voltex");
	m_uniforms["warpfield"] = m_program->getUniformLocation("warpfield");
	m_uniforms["isovalue" ] = m_program->getUniformLocation("isovalue");

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
