#include "Viewer.h"
#include "qglutils.h"

#include <QDebug>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>

#include "VolumeUtils.h"

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

Viewer::Viewer( QWidget* parent )
	: QGLViewer( parent )
{
	QAction
		*actReloadShader = new QAction(tr("Reload shader"),this);
	actReloadShader->setShortcut(Qt::CTRL+Qt::Key_R);
	actReloadShader->setStatusTip(tr("Reload GLSL streamline shaders from disk."));
	connect( actReloadShader, SIGNAL(triggered()), this, SLOT(reloadShaders()) );

	QAction
		*actLoadTemplate    = new QAction(tr("Load template..."),this),
		*actLoadDeformation = new QAction(tr("Load deformation..."),this),
		*actLoadSeedPoints  = new QAction(tr("Load seed points..."),this);
	connect( actLoadTemplate   , SIGNAL(triggered()), this, SLOT(loadTemplate()) );
	connect( actLoadDeformation, SIGNAL(triggered()), this, SLOT(loadDeformation()) );
	connect( actLoadSeedPoints , SIGNAL(triggered()), this, SLOT(loadSeedPoints()) );

	QAction* sep0 = new QAction(this); sep0->setSeparator( true );
	m_actions.push_back( actLoadTemplate );
	m_actions.push_back( actLoadDeformation );	
	m_actions.push_back( actLoadSeedPoints );
	m_actions.push_back( sep0 );
	m_actions.push_back( actReloadShader );
}

void Viewer::init()
{
	qglutils::initializeGL();	
	m_slr.initGL();

	glHint( GL_POINT_SMOOTH_HINT, GL_NICEST );
	glEnable( GL_POINT_SMOOTH );
}

void Viewer::destroyGL()
{
	makeCurrent();
	m_slr.destroyGL();
	m_vtm.destroy();
}

void Viewer::draw()
{
	//m_slr.bind();
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_LIGHTING );
	glDisable( GL_CULL_FACE );
	glBlendFunc( GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA );
	glEnable( GL_BLEND );
	
	glColor4f(1.f,1.f,1.f,1.f);
	glPointSize( 3.f );
	m_seed.render();

	glDisable( GL_BLEND );	
	glEnable( GL_LIGHTING );
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE );
	//m_slr.release();
}

void Viewer::reloadShaders()
{
	m_slr.reloadShadersFromDisk();
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

bool Viewer::loadTemplate( QString filename )
{
	GL::GLTexture* tex = loadVolume( filename );
	if( !tex )
		return false;

	// Remove old texture from manager (if any)
	m_vtm.erase( m_slr.getVolume() );

	// Set new texture
	m_slr.setVolume( tex );
	return true;
}

bool Viewer::loadDeformation( QString filename )
{
	GL::GLTexture* tex = loadVolume( filename );
	if( !tex )
		return false;

	// Remove old texture from manager (if any)
	m_vtm.erase( m_slr.getWarpfield() );

	// Set new texture
	m_slr.setWarpfield( tex );
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

