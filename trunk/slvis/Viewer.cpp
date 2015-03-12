#include "Viewer.h"
#include "qglutils.h"

#include <QDebug>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QInputDialog>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QWidget>

#include <GL/GLSLProgram.h>
#include <GL/GLError.h>

#include "VolumeUtils.h"


#ifdef USE_MESHTOOLS
#include <meshtools.h>

SimpleMesh::SimpleMesh()
	: m_vao(0)
{
}

bool SimpleMesh::load( const char* filename )
{
	meshtools::Mesh mesh;
	if( !meshtools::loadMesh( mesh, filename ) )
	{
		return false;
	}
	meshtools::updateMeshVertexNormals( &mesh );

	m_mb.clear();
	m_mb.addFrame( &mesh );
	return true;
}

void SimpleMesh::render()
{
	if( m_mb.numFrames() > 0 )
	{
		m_mb.draw();
	}
}

void SimpleMesh::sanity()
{
	m_mb.sanity();

	if( m_vao==0 )
		glGenVertexArrays( 1, &m_vao );
}

void SimpleMesh::renderVAO( unsigned shaderProgram )
{
	if( m_mb.numFrames() == 0 )
		return;

	sanity();

	GLuint vbo = m_mb.vbo();
	GLuint ibo = m_mb.ibo();
	
	glBindVertexArray( m_vao );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo );	
	
	GLint posAttrib=-1;		
	glBindBuffer( GL_ARRAY_BUFFER, vbo );	
	posAttrib = glGetAttribLocation( (GLuint)shaderProgram, "Position" );
	glEnableVertexAttribArray( posAttrib );
	glVertexAttribPointer( posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0 );
	
	GLint normAttrib=-1;
	normAttrib = glGetAttribLocation( shaderProgram, "Normal" );
	glEnableVertexAttribArray( normAttrib );
	glVertexAttribPointer( normAttrib, 3, GL_FLOAT, GL_FALSE, 0, 
	                            (GLvoid*)(sizeof(float)*m_mb.numVertices()*3) );
	
	glDrawElements( GL_TRIANGLES, (GLsizei)m_mb.numIndices(), GL_UNSIGNED_INT, 0 );
	
	glDisableVertexAttribArray( posAttrib );
	glDisableVertexAttribArray( normAttrib );

	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	glBindVertexArray( 0 );	
}
#endif // USE_MESHTOOLS


/// Copy information from VolumeDataHeader to VolumeTextureManager::Data
VolumeTextureManager::Data adapt( VolumeDataHeader* mhd )
{
	if( !mhd ) return VolumeTextureManager::Data();

	int type;
	switch( mhd->elementTypeName() )
	{
	case VolumeDataHeader::UCHAR  : type = VolumeTextureManager::UChar;   break;
	case VolumeDataHeader::USHORT : type = VolumeTextureManager::UShort;  break;
	case VolumeDataHeader::FLOAT  : type = VolumeTextureManager::Float32; break;
	// Some newer versions of VolumeData.h support further types:
	//case VolumeDataHeader::CHAR   : type = VolumeTextureManager::Char;   break;
	//case VolumeDataHeader::SHORT  : type = VolumeTextureManager::Short;  break;
	//case VolumeDataHeader::DOUBLE : type = VolumeTextureManager::Float32; break;
	default:
		// Unsupported / unknown type ?!
		type = -1;
	}

	VolumeTextureManager::Data data;
	data.setResolution( mhd->resX(), mhd->resY(), mhd->resZ() )
		.setComponents( mhd->numChannels() )
		.setSpacing( mhd->spacingX(), mhd->spacingY(), mhd->spacingZ() )
		.setType( type );

	return data;
}

QString Viewer::helpString() const
{
	return QString(
		"<h2>slvis</h2>"
		"Max Hermann (<a href='mailto:hermann@cs.uni-bonn.de'>hermann@cs.uni-bonn.de</a>)<br>"
		"University of Bonn, Computer Graphics Group<br>"
		"Mar. 2015");
}

QAction* createSeparator( QWidget* parent )
{
	QAction* sep = new QAction(parent);
	sep->setSeparator( true );
	return sep;
}

Viewer::Viewer( QWidget* parent )
	: QGLViewer( parent )
{
	// -- Actions

	QAction
		*actReloadShader = new QAction(tr("Reload shader"),this),
		*actEnableShader = new QAction(tr("Enable shader"),this);
	actReloadShader->setShortcut(Qt::CTRL+Qt::Key_R);
	actReloadShader->setStatusTip(tr("Reload GLSL streamline shaders from disk."));
	actEnableShader->setShortcut(Qt::CTRL+Qt::Key_T);
	actEnableShader->setCheckable( true );
	actEnableShader->setChecked( false );
	connect( actReloadShader, SIGNAL(triggered()), this, SLOT(reloadShaders()) );

	m_actEnableShader = actEnableShader;

	QAction
		*actLoadTemplate    = new QAction(tr("Load template..."),this),
		*actLoadDeformation = new QAction(tr("Load deformation..."),this),
		*actLoadSeedPoints  = new QAction(tr("Load seed points..."),this),
		*actLoadMesh        = new QAction(tr("Load mesh..."),this),
		*actSetIsovalue     = new QAction(tr("Set isovalue"),this),
		*actShowWarpedMesh  = new QAction(tr("Show warped mesh"),this);
	connect( actLoadTemplate   , SIGNAL(triggered()), this, SLOT(loadTemplate()) );
	connect( actLoadDeformation, SIGNAL(triggered()), this, SLOT(loadDeformation()) );
	connect( actLoadSeedPoints , SIGNAL(triggered()), this, SLOT(loadSeedPoints()) );
	connect( actLoadMesh       , SIGNAL(triggered()), this, SLOT(loadMesh()) );
	connect( actSetIsovalue    , SIGNAL(triggered()), this, SLOT(setIsovalue()) );

	actShowWarpedMesh->setCheckable( true );
	actShowWarpedMesh->setChecked( true );
	
	m_actShowWarpedMesh = actShowWarpedMesh;

	m_actions.push_back( actLoadTemplate );
	m_actions.push_back( actLoadDeformation );	
	m_actions.push_back( actLoadSeedPoints );
	m_actions.push_back( actLoadMesh );
	m_actions.push_back( createSeparator(this) );
	m_actions.push_back( actShowWarpedMesh );
	m_actions.push_back( actSetIsovalue );
	m_actions.push_back( createSeparator(this) );
	m_actions.push_back( actReloadShader );
	m_actions.push_back( actEnableShader );

	// -- Other GUI

	m_spinTimescale = new QDoubleSpinBox;
	m_spinTimescale->setRange( -10.0, +10.0 );
	m_spinTimescale->setSingleStep( 0.1 );
	m_spinTimescale->setValue( (double)m_slr[0].getTimescale() );

	connect( m_spinTimescale, SIGNAL(valueChanged(double)), this, SLOT(setTimescale(double)) );

	QFormLayout* formLayout = new QFormLayout;
	formLayout->addRow(tr("timescale"),m_spinTimescale);
	m_controlWidget = new QWidget;
	m_controlWidget->setLayout( formLayout );
}

void Viewer::init()
{
	qglutils::initializeGL();	
	m_seed.initGL();
	m_slr[0].setMode( StreamlineRenderer::StreamlineShader );
	m_slr[0].initGL();
	m_slr[1].setMode( StreamlineRenderer::MeshwarpShader );
	m_slr[1].initGL();

	glHint( GL_POINT_SMOOTH_HINT, GL_NICEST );
	glEnable( GL_POINT_SMOOTH );

	glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
	glEnable( GL_LINE_SMOOTH );

	GLfloat pos[4] = { 1.0,1.0,1.0,0.0 };
	glLightfv( GL_LIGHT0, GL_POSITION, pos );

	restoreStateFromFile();
}

void Viewer::destroyGL()
{
	makeCurrent();
	m_slr[0].destroyGL();
	m_slr[1].destroyGL();
	m_seed.destroyGL();
	m_vtm.destroy();
}

void setShaderProgramDefaultMatrices( GLuint program )
{
	GLfloat modelview[16], projection[16]; 
	glGetFloatv( GL_MODELVIEW_MATRIX, modelview );
	glGetFloatv( GL_PROJECTION_MATRIX, projection );

	GLint locModelview = glGetUniformLocation( program, "Modelview" );
	GLint locProjection = glGetUniformLocation( program, "Projection" );

	glUniformMatrix4fv( locModelview,  1, GL_FALSE, modelview );
	glUniformMatrix4fv( locProjection, 1, GL_FALSE, projection );
}

unsigned Viewer::bindShader( int mode )
{
	GLuint shaderProgram = 0;
	if( m_actEnableShader->isChecked() )
	{
		m_slr[mode].bind();

		shaderProgram = m_slr[mode].getProgram()->getProgramHandle();

		setShaderProgramDefaultMatrices( shaderProgram );

		// DEBUG: Validate
		if( !GL::GLSLProgram::validate( shaderProgram ) )
		{
			std::cerr << "Viewer::bindShader() - "
				"Invalid GLSL program! Info log:" << std::endl
				<< GL::GLSLProgram::getProgramLog( shaderProgram );
		}
		GL::CheckGLError("Viewer::bindShader() - Enable shader");
	}
	return (unsigned)shaderProgram;
}

void Viewer::draw()
{
	//// QGLViewer workaround (does not fix text rendering bug)
	//glPushAttrib( GL_ALL_ATTRIB_BITS );
	//glPushClientAttrib( GL_CLIENT_ALL_ATTRIB_BITS );

	unsigned program;

	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_BLEND );

	// -- Reference mesh

	glColor4f(.5f,.5f,1.f,0.5f);
	m_mesh.render();

	// -- Streamlines  (draws seed points if shaders are disabled)

	program = bindShader(0);

	glDisable( GL_LIGHTING );
	glDisable( GL_CULL_FACE );	
	
	glColor4f(1.f,1.f,1.f,0.9f);
	glPointSize( 3.f );
	glLineWidth( 1.2f );
	m_seed.render( program );

	glEnable( GL_CULL_FACE );
	glEnable( GL_LIGHTING );	

	m_slr[0].release();

	// -- Warped mesh  (requires shaders enabled)

	if( m_actShowWarpedMesh->isChecked() && m_actEnableShader->isChecked() )
	{
		program = bindShader(1);
		m_mesh.renderVAO( program );
		m_slr[1].release();
	}

	glDisable( GL_BLEND );

	//glPopClientAttrib();
	//glPopAttrib();
}

void Viewer::reloadShaders()
{
	m_slr[0].reloadShadersFromDisk();
	m_slr[1].reloadShadersFromDisk();
}

void Viewer::setBaseDir( QString filename )
{
	QFileInfo info(filename);
	m_baseDir = info.absolutePath();
}

void Viewer::loadTemplate()
{
	QString filename = QFileDialog::getOpenFileName( this, 
		tr("Load scalar template volume"), m_baseDir, tr("Raw Meta Image (*.mhd)") );

	if( filename.isEmpty() )
		return;

	setBaseDir( filename );

	if( !loadTemplate( filename ) )
	{
		QMessageBox::warning( this, tr("Error loading template"),
			tr("Error loading template %1").arg(filename) );
		return;
	}
}

void Viewer::loadDeformation()
{
	QString filename = QFileDialog::getOpenFileName( this, 
		tr("Load deformation field"), m_baseDir, tr("Raw Meta Image (*.mhd)") );

	if( filename.isEmpty() )
		return;

	setBaseDir( filename );

	if( !loadDeformation( filename ) )
	{
		QMessageBox::warning( this, tr("Error loading deformation"),
			tr("Error loading template %1").arg(filename) );
		return;
	}
}

void Viewer::loadSeedPoints()
{
	QString filename = QFileDialog::getOpenFileName( this, 
		tr("Load seed points"), m_baseDir, tr("VTK PolyData (*.vtk)") );

	if( filename.isEmpty() )
		return;

	setBaseDir( filename );

	if( !loadSeedPoints( filename ) )
	{
		QMessageBox::warning( this, tr("Error loading seed points"),
			tr("Error loading seed points from %1").arg(filename) );
		return;
	}
}

void Viewer::loadMesh()
{
	QString filename = QFileDialog::getOpenFileName( this, 
		tr("Load mesh"), m_baseDir, tr("Triangle mesh formats (*.obj *.ply *.off)") );

	if( filename.isEmpty() )
		return;

	setBaseDir( filename );

	if( !loadMesh( filename ) )
	{
		QMessageBox::warning( this, tr("Error loading mesh"),
			tr("Error loading mesh from %1").arg(filename) );
		return;
	}
}

bool Viewer::loadTemplate( QString filename )
{
	GL::GLTexture* tex = loadVolume( filename );
	if( !tex )
		return false;

	// Remove old texture from manager (if any)
	m_vtm.erase( m_slr[0].getVolume() );

	// Set new texture (synchronized between all StreamlineRenderer instances)
	m_slr[0].setVolume( tex );
	m_slr[1].setVolume( tex );
	return true;
}

bool Viewer::loadDeformation( QString filename )
{
	GL::GLTexture* tex = loadVolume( filename );
	if( !tex )
		return false;

	// Remove old texture from manager (if any)
	m_vtm.erase( m_slr[0].getWarpfield() );

	// Set new texture (synchronized between all StreamlineRenderer instances)
	m_slr[0].setWarpfield( tex );
	m_slr[1].setWarpfield( tex );
	return true;
}

bool Viewer::loadSeedPoints( QString filename )
{
	if( m_seed.loadPointSamples( filename.toStdString().c_str() ) )
	{
		updateBoundingBox();
		return true;
	}
	return false;
}

bool Viewer::loadMesh( QString filename )
{
	return m_mesh.load( filename.toStdString().c_str() );
}

GL::GLTexture* Viewer::loadVolume( QString filename )
{
	// Load
	VolumeDataHeader* mhd;
	void* dataPtr;
	mhd = load_volume( filename.toStdString().c_str(), 0, &dataPtr );
	if( !mhd )
	{
		QMessageBox::warning( this, tr("Error loading volume"),
			tr("Error loading volume %1").arg(filename) );
		return NULL;
	}

	// Assemble information for manager
	VolumeTextureManager::Data data = adapt( mhd );
	if( data.type < 0 )
	{
		QMessageBox::warning( this, tr("Error loading volume"),
			tr("Unsupported element type!") );
		mhd->clear(); // free data
		delete mhd;
		return NULL;
	}
	data.setDataPtr( dataPtr ); // Do not forget to set data pointer!

	// Upload to GPU
	makeCurrent();
	GL::GLTexture* tex;
	tex = m_vtm.upload( data );
	doneCurrent();

	if( !tex )
	{
		QMessageBox::warning( this, tr("Error loading volume"),
			tr("Failed to upload volume to GPU!") );
		mhd->clear(); // free data
		delete mhd;
		return NULL;
	}

	return tex;
}

void Viewer::setIsovalue()
{
	bool ok;
	double iso = QInputDialog::getDouble( this, tr("Set isovalue"), 
		tr("Isovalue"), (double)m_slr[0].getIsovalue(), -1000.0, 12000.0, 1, &ok );
	if( ok )
	{
		m_slr[0].setIsovalue( (float)iso );
	}
}

void Viewer::setTimescale( double val )
{
	// Both programs have synchronized timescale
	m_slr[0].setTimescale( (float)val );
	m_slr[1].setTimescale( (float)val );

	// Render
	updateGL();
}

void Viewer::updateBoundingBox()
{
	// Use points to update bounding box for QGLViewer camera

	qglviewer::Vec min_( 
		(double)m_seed.getAABBMin()[0],
		(double)m_seed.getAABBMin()[1],
		(double)m_seed.getAABBMin()[2] );

	qglviewer::Vec max_( 
		(double)m_seed.getAABBMax()[0],
		(double)m_seed.getAABBMax()[1],
		(double)m_seed.getAABBMax()[2] );

	setSceneBoundingBox( min_, max_ );
	showEntireScene();
}

