#include "PointSamples.h"
#include <GL/GLConfig.h>
#include <GL/GLError.h>

#include <GL/GLSLProgram.h>
#include <iostream>

//-----------------------------------------------------------------------------
//	VTK IO functions
//-----------------------------------------------------------------------------

#include <vtkPolyDataReader.h>
#include <vtkPolyData.h>

#ifndef VTKPTR
 #include <vtkSmartPointer.h>
 #define VTKPTR vtkSmartPointer
#endif

float* createVertexBufferData( vtkPolyData* pd )
{
	if( !pd ) return NULL;
	int numPoints = (int)pd->GetNumberOfPoints();
	float* buf = new float[ 3*numPoints ];
	float* ptr = buf;
	for( int i=0; i < numPoints; i++ )
	{
		vtkIdType id = i; // Assume 1-1 index to id mapping
		double pt[3];
		pd->GetPoint( id, pt );
		*ptr = pt[0]; ptr++;
		*ptr = pt[1]; ptr++;
		*ptr = pt[2]; ptr++;
	}
	return buf;
}

float* loadPointSamplesVTK( const char* filename, int& numPoints )
{
	VTKPTR<vtkPolyDataReader> reader = VTKPTR<vtkPolyDataReader>::New();
	reader->SetFileName( filename );
	reader->Update();
	
	numPoints = reader->GetOutput()->GetNumberOfPoints();
	if( numPoints > 0 )
	{	
		return createVertexBufferData( reader->GetOutput() );
	}
	return NULL;
}

//-----------------------------------------------------------------------------
//	PointSamples
//-----------------------------------------------------------------------------

PointSamples::~PointSamples()
{
	delete [] m_vdata;
}

void PointSamples::initGL()
{
	// Create VAO
	m_vao = 0;
	glGenVertexArrays( 1, &m_vao );
	m_vbo.create();
}

void PointSamples::destroyGL()
{
	glDeleteVertexArrays( 1, &m_vao );
	m_vbo.destroy();
}

bool PointSamples::loadPointSamples( const char* filename )
{
	// Load
	int numPoints = 0;
	float* data = loadPointSamplesVTK( filename, numPoints );
	if( !data )
		return false;

	// Set new data
	if( m_vdata ) delete [] m_vdata;
	m_vdata = data;
	m_numPoints = numPoints;
	updateAABB();

	return true;
}

void PointSamples::render( unsigned shaderProgram )
{
	GL::CheckGLError("PointSamples::render() - on invocation");
#if 0
	// Render in immediate mode (for debugging)
	glBegin( GL_POINTS );
	float *ptr = m_vdata;
	for( int i=0; i < m_numPoints; i++, ptr+=3 )
		glVertex3fv( ptr );
	glEnd();
	GL::CheckGLError("PointSamples::render() - glBegin/glEnd");
#else
	// Setup VBO
	if( !m_vbo.initialized && !m_vbo.create() )
		return;	
	if( m_dirty && m_vdata )
	{
		if( !m_vbo.upload( m_vdata, m_numPoints ) )
			return;
		else
			// upload successful 
			m_dirty = false;
	}
	m_vbo.bind();

	// Setup VAO
	GLint posAttrib=-1;
	if( shaderProgram > 0 && m_vao > 0 )
	{
		// VAO (required for GS, see https://www.opengl.org/wiki_132/index.php/Vertex_Rendering)
		glBindVertexArray( m_vao );
		// We define now fixed layout locations in the shader like this:
		//   layout(location=0) in vec3 Position;
		//   layout(location=1) in vec3 Normal;
		// Previously we used automatic assignment and query:
		//   posAttrib = glGetAttribLocation( shaderProgram, "Position" );
		//   glEnableVertexAttribArray( posAttrib );
		posAttrib = 0;
		glEnableVertexAttribArray( posAttrib );
		glVertexAttribPointer( posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0 );
		GL::CheckGLError("PointSamples::render() - VAO setup");
	}
	else
	{
		// Fall-back solution w/o VAO
		glEnableClientState( GL_VERTEX_ARRAY );
		glVertexPointer( 3, GL_FLOAT, 0, 0 );
	}

	// Draw
	glDrawArrays( GL_POINTS, (GLint)0, (GLsizei)m_numPoints );
	GL::CheckGLError("PointSamples::render() -- glDrawArrays()");

	// Release
	if( shaderProgram > 0 && m_vao > 0 )
	{
		glDisableVertexAttribArray( posAttrib );
		GL::CheckGLError("PointSamples::render() -- glDisableVertexAttribArray()");
	}
	else
	{
		glDisableClientState( GL_VERTEX_ARRAY );		
		GL::CheckGLError("PointSamples::render() -- glDisableClientState()");
	}
	m_vbo.unbind();
	GL::CheckGLError("PointSamples::render() -- VBO unbind");
#endif
	GL::CheckGLError("PointSamples::render()");
}

void PointSamples::updateAABB()
{
	m_aabb.reset();
	float* ptr = m_vdata;
	for( int i=0; i < m_numPoints; i++, ptr+=3 )
		m_aabb.include( ptr );
}


//-----------------------------------------------------------------------------
//	PointSamples::VertexBufferObject
//-----------------------------------------------------------------------------

bool PointSamples::VertexBufferObject::create()
{
	glGenBuffers( 1, &vbo );
	initialized = GL::CheckGLError("VertexBufferObject::create()");
	return initialized;
}

bool PointSamples::VertexBufferObject::bind()
{
	glBindBuffer( GL_ARRAY_BUFFER, vbo );	
	return GL::CheckGLError("VertexBufferObject::bind()");
}

void PointSamples::VertexBufferObject::unbind()
{
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
}

bool PointSamples::VertexBufferObject::upload( float* data, int numVertices )
{
	bind();
	glBufferData( GL_ARRAY_BUFFER, 3*numVertices*sizeof(float), data, GL_STATIC_DRAW );	
	return GL::CheckGLError("VertexBufferObject::upload()");
}

void PointSamples::VertexBufferObject::destroy()
{
	if( vbo >= 0 )
		glDeleteBuffers( 1, &vbo );
}
