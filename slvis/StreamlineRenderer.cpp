#include "StreamlineRenderer.h"
#include "glbase.h"
#include <GL/GLSLProgram.h>
#include <GL/GLError.h>
#include <iostream>

using std::cerr;
using std::endl;
using GL::GLSLProgram;

StreamlineRenderer::StreamlineRenderer()
: m_program(NULL),
  m_texVolume(NULL),
  m_texWarpfield(NULL)
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
}

void StreamlineRenderer::release()
{
	m_program->release();
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
}

void StreamlineRenderer::setVolume( GL::GLTexture* tex )
{
	m_texVolume = tex;
}

void StreamlineRenderer::setWarpfield( GL::GLTexture* tex )
{
	m_texWarpfield = tex;
}
