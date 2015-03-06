#include "PointSamples.h"
#include <GL/GLConfig.h>
#include <GL/GLError.h>

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

void PointSamples::destroyGL()
{
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

void PointSamples::render()
{
#if 1
	glBegin( GL_POINTS );
	float *ptr = m_vdata;
	for( int i=0; i < m_numPoints; i++, ptr+=3 )
		glVertex3fv( ptr );
	glEnd();
#else
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
	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 3, GL_FLOAT, 0, 0 );
	glDrawArrays( GL_POINTS, (GLint)0, (GLsizei)m_numPoints );
	glDisableClientState( GL_VERTEX_ARRAY );		
	m_vbo.unbind();
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
