#include "VTKVisWidget.h"
#include "PleaseWaitDialog.h"
#include <QtGui>
#include <QVTKWidget.h>
#include <vtkRenderWindow.h>
#include <vtkCamera.h>
#include <iostream>

VTKVisWidget::VTKVisWidget( QWidget* parent )
: QWidget(parent),
  m_selvol(0),
  m_warpvis(NULL)
{
	m_vtkWidget = new QVTKWidget();
#ifdef USE_DEPTH_PEELING
	// Setup Depth-Peeling
	m_vtkWidget->GetRenderWindow()->SetAlphaBitPlanes( 1 );
	m_vtkWidget->GetRenderWindow()->SetMultiSamples  ( 0 );	
	m_vren.getRenderer()->SetUseDepthPeeling( 1 );
	m_vren.getRenderer()->SetMaximumNumberOfPeels( 100 );
	m_vren.getRenderer()->SetOcclusionRatio( 0.1 );
	std::cout << "VTKVisWidget: Depth-Peeling enabled" << std::endl;

	// TODO: Check if depth-peeling was actually used by calling the 
	//       following after rendering:
	// int depthPeelingWasUsed=renderer->GetLastRenderingUsedDepthPeeling();
#endif	

	// setup VTK rendering
	m_vtkWidget->GetRenderWindow()->AddRenderer( m_vren.getRenderer() );
	m_vtkWidget->GetRenderWindow()->SetLineSmoothing( 1 );

	m_vren.getRenderer()->GradientBackgroundOn();
	m_vren.getRenderer()->SetBackground( .7,.7,.7 );
	m_vren.getRenderer()->SetBackground2( 1,1,1 );

	// layout
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget( m_vtkWidget );
	layout->setContentsMargins( 0,0,0,0 );
	setLayout( layout );

	// -- actions --	

	actOpen = new QAction( tr("&Open volume dataset..."), this );
//	actOpen->setShortcut( tr("Ctrl+O") );
	actOpen->setStatusTip( tr("Open volume dataset from disk.") );
	connect( actOpen, SIGNAL(triggered()), this, SLOT(openVolumeDataset()) );

	actOpenWarp = new QAction( tr("Open warpfield dataset..."), this );
	connect( actOpenWarp, SIGNAL(triggered()), this, SLOT(openWarpDataset()) );

	actSaveIsosurface = new QAction( tr("Save isosurface..."), this );
	actSaveIsosurface->setStatusTip( tr("Export current isosurface as Wavefront OBJ to disk.") );
	connect( actSaveIsosurface, SIGNAL(triggered()), this, SLOT(saveIsosurface()) );

	actToggleVolume = new QAction( tr("Show &volume"), this );
//	actToggleVolume->setShortcut( tr("Ctrl+V") );
	actToggleVolume->setCheckable( true );
	actToggleVolume->setChecked( true );
	connect( actToggleVolume, SIGNAL(toggled(bool)), this, SLOT(toggleVolume(bool)) );

	actToggleWarpVis = new QAction( tr("Show warpfield"), this );
	actToggleWarpVis->setCheckable( true );
	actToggleWarpVis->setChecked( true );
	connect( actToggleWarpVis, SIGNAL(toggled(bool)), this, SLOT(toggleWarpVis(bool)) );
	
	actToggleWarpLegend = new QAction( tr("Show warpfield color legend"), this );
	actToggleWarpLegend->setCheckable( true );
	actToggleWarpLegend->setChecked( true );
	connect( actToggleWarpLegend, SIGNAL(toggled(bool)), this ,SLOT(toggleWarpLegend(bool)) );

	actToggleContour = new QAction( tr("Show &contour"), this );
//	actToggleContour->setShortcut( tr("Ctrl+C") );
	actToggleContour->setCheckable( true );
	actToggleContour->setChecked( false );
	connect( actToggleContour, SIGNAL(toggled(bool)), this, SLOT(toggleContour(bool)) );

	actToggleOutline = new QAction( tr("Show outline"), this );
	actToggleOutline->setCheckable( true );
	actToggleOutline->setChecked( true );
	connect( actToggleOutline, SIGNAL(toggled(bool)), this, SLOT(toggleOutline(bool)) );

	actToggleAxes = new QAction( tr("Show &axes"), this );
//	actToggleAxes->setShortcut( tr("Ctrl+A") );
	actToggleAxes->setCheckable( true );
	actToggleAxes->setChecked( true );
	connect( actToggleAxes, SIGNAL(toggled(bool)), this, SLOT(toggleAxes(bool)) );

	actToggleGrid = new QAction( tr("Show &grid"), this );
//	actToggleGrid->setShortcut( tr("Ctrl+G") );
	actToggleGrid->setCheckable( true );
	actToggleGrid->setChecked( true );
	connect( actToggleGrid, SIGNAL(toggled(bool)), this, SLOT(toggleGrid(bool)) );

	actGlyphScale = new QAction( tr("Change glyph scale"), this );
	connect( actGlyphScale, SIGNAL(triggered()), this, SLOT(changeGlyphScale()) );

	actIsovalue = new QAction( tr("Change isovalue"), this );
	connect( actIsovalue, SIGNAL(triggered()), this, SLOT(changeIsovalue()) );

	actGridsize = new QAction( tr("Change gridsize"), this );
	actIsovalue->setStatusTip( tr("Set number of grid cubes along X axis (Y/Z are set according to aspect ratios).") );
	connect( actGridsize, SIGNAL(triggered()), this, SLOT(changeGridsize()) );

	actResetCamera = new QAction( tr("&Reset camera"), this );
//	actResetCamera->setShortcut( tr("Ctrl+R") );
	connect( actResetCamera, SIGNAL(triggered()), this, SLOT(resetCamera()) );

	QAction* sep0 = new QAction( tr(""), this ); sep0->setSeparator( true );
	QAction* sep1 = new QAction( tr(""), this ); sep1->setSeparator( true );
	QAction* sep2 = new QAction( tr(""), this ); sep2->setSeparator( true );

	m_actions.push_back( actOpen );
	m_actions.push_back( actOpenWarp );
	m_actions.push_back( actSaveIsosurface );
	m_actions.push_back( sep0 );
	m_actions.push_back( actToggleVolume );
	m_actions.push_back( actToggleWarpVis );
	m_actions.push_back( actToggleWarpLegend );
	m_actions.push_back( actToggleContour );
	m_actions.push_back( actToggleOutline );
	m_actions.push_back( actToggleAxes );
	m_actions.push_back( actToggleGrid );
	m_actions.push_back( sep1 );
	m_actions.push_back( actIsovalue );
	m_actions.push_back( actGridsize );
	m_actions.push_back( actGlyphScale );
	m_actions.push_back( sep2 );
	m_actions.push_back( actResetCamera );
}

void VTKVisWidget::updateActions()
{	
	// Update checkable actions affected by volume selection change.
	actToggleVolume ->setChecked( m_vren.getVolumeVisibility ( m_selvol ) );
	actToggleContour->setChecked( m_vren.getContourVisibility( m_selvol ) );

	actToggleAxes   ->setChecked( m_vren.getAxesVisibility   () );
	actToggleGrid   ->setChecked( m_vren.getGridVisibility   () );
	actToggleOutline->setChecked( m_vren.getOutlineVisibility() );
}

void VTKVisWidget::changeIsovalue()
{
	bool ok;
	double value = QInputDialog::getDouble( this, tr("Change Isovalue"),
		tr("Isovalue:"), m_vren.getContourValue(), 0.0, 2000.0, 2, &ok );
	if( ok )
	{
		m_vren.setContourValue( value );
		m_vtkWidget->update();  // force redraw
	}
}

void VTKVisWidget::changeGridsize()
{
	bool ok;
	int value = QInputDialog::getInteger( this, tr("Change grid size"), 
		tr("Number of base cubes"), m_vren.getGridSize(), 1, 64, 1, &ok );
	if( ok )
	{
		m_vren.setGridSize( value );
		m_vtkWidget->update();  // force redraw
	}
}

void VTKVisWidget::changeGlyphScale()
{
	if( !m_warpvis ) return; // sanity

	bool ok;
	double value = QInputDialog::getDouble( this, tr("Change glyph scale factor"),
		tr("Glyph scale factor:"), m_warpvis->getGlyph3D()->GetScaleFactor(),
		0.0, 100.0, 4, &ok );
	if( ok )
	{
		m_warpvis->getGlyph3D()->SetScaleFactor( value );
		m_vtkWidget->update();  // force redraw
	}
}

void VTKVisWidget::toggleVolume ( bool visible ) 
{ 
	m_vren.setVolumeVisibility( visible, m_selvol );
	m_vtkWidget->update();  // force redraw
}

void VTKVisWidget::toggleContour( bool visible ) 
{ 
	//statusBar()->showMessage( tr("Extracting isosurface...") );	
	// force status bar update before applications stalls due to computation
	QCoreApplication::sendPostedEvents(); // send showMessage() *now*
	for( int i=0; i < 10; ++i )
		QCoreApplication::processEvents();    // update statusBar *now*

	m_vren.setContourVisibility( visible, m_selvol ); 
	m_vtkWidget->update(); 
	m_vtkWidget->update(); // force redraw
	//statusBar()->showMessage( tr("Ready") );
}

void VTKVisWidget::toggleAxes ( bool visible ) 
{
	m_vren.setAxesVisibility( visible );
	m_vtkWidget->update();  // force redraw
}

void VTKVisWidget::toggleGrid ( bool visible ) 
{
	m_vren.setGridVisibility( visible );
	m_vtkWidget->update();  // force redraw
}

void VTKVisWidget::toggleOutline( bool visible ) 
{
	m_vren.setOutlineVisibility( visible );
	m_vtkWidget->update();  // force redraw
}

void VTKVisWidget::toggleWarpVis( bool visible )
{
	if( !m_warpvis ) return; // sanity
	m_warpvis->getVisActor()->SetVisibility( (int)visible );
	m_vtkWidget->update();  // force redraw
}

void VTKVisWidget::toggleWarpLegend( bool visible )
{
	if( !m_warpvis ) return; // sanity
	m_warpvis->getScalarBar()->SetVisibility( (int)visible );
	m_vtkWidget->update();  // force redraw
}

void VTKVisWidget::resetCamera()
{
	vtkRenderer* ren = m_vren.getRenderer();
	ren->ResetCamera();
		//ren->GetActiveCamera()->SetViewUp( 0, 1, 0 );
		//ren->GetActiveCamera()->SetRoll( 0 );
	ren->GetRenderWindow()->Render();
	update(); // needed?
}

bool VTKVisWidget::setVolume( QString mhdFilename )
{
	//using namespace std;
	//string filename = m_baseDir.toStdString() + "/" + volume->filename();
	//cout << "*" << filename << endl;

	if( !m_vren.setup( mhdFilename.toAscii() ) )  // was: filename.c_str()
	{
		QMessageBox::warning( this, tr("Error setting volume"),
			tr("Error: Could not setup volume rendering correctly!") );
		return false;
	}

	// reset camera
	//resetCamera();
	return true;
}

bool VTKVisWidget::setWarp( QString mhdFilename )
{
	VTKPTR<vtkImageData> img = loadImageData( mhdFilename.toAscii() );
	
	if( !img.GetPointer() )
	{
		QMessageBox::warning( this, tr("Error loading volume"),
			tr("Error: Could not load specified warp field!") );
		return false;
	}

	SHOW_PLEASEWAIT_DIALOG( tr("sdmvis"),
		tr("Setting up vectorfield visualization...") )

	// remove old visualization
	if( m_warpvis ) {
		m_vren.getRenderer()->RemoveActor( m_warpvis->getVisActor() );
		m_vren.getRenderer()->RemoveActor( m_warpvis->getScalarBar() );
	}

	// create new warpvis
	std::cout << "Setting up VectorfieldGlyphVisualization2..." << std::endl;
	delete m_warpvis;
	m_warpvis = new VectorfieldGlyphVisualization2( img.GetPointer(), 23000 );
	std::cout << "Setting up VectorfieldGlyphVisualization2 done" << std::endl;

	// add to visualization
	std::cout << "Adding renderer..." << std::endl;
	m_vren.getRenderer()->AddActor( m_warpvis->getVisActor() );
	m_vren.getRenderer()->AddActor( m_warpvis->getScalarBar() );
	std::cout << "Adding renderer done" << std::endl;

	update(); // FIXME: do we need to call process events to get this update handled?

	HIDE_PLEASEWAIT_DIALOG()

	// reset camera
	//resetCamera();
	return true;
}

void VTKVisWidget::openVolumeDataset()
{
	QString filename;
	filename = QFileDialog::getOpenFileName( this,
		tr("Open volume dataset"),
		m_baseDir, tr("Volume description file (*.mhd)") );

	// cancelled?
	if( filename.isEmpty() )
		return;

	// extract absolute path and filename
	QFileInfo info( filename );
	m_baseDir = info.absolutePath(); // name is info.fileName()	

	// load volume dataset
	//statusBar()->showMessage( tr("Loading volume dataset %1...").arg(info.fileName()) );

	// replace current volume with temporary one
	bool success = setVolume( filename );
	
	updateActions();

	// succesfully loaded
	/*
	if( success ) {
		//statusBar()->showMessage( tr("Successfully loaded %1").arg(info.fileName()) );
		setWindowTitle( APP_NAME + " - " + info.fileName() );
	} else {
		//statusBar()->showMessage( tr("Failed to load %1").arg(info.fileName()) );
		setWindowTitle( APP_NAME );
	}
	*/
}

void VTKVisWidget::openWarpDataset()
{
	QString filename;
	filename = QFileDialog::getOpenFileName( this,
		tr("Open warpfield dataset"),
		m_baseDir, tr("Volume description file (*.mhd)") );

	// cancelled?
	if( filename.isEmpty() )
		return;

	// extract absolute path and filename
	QFileInfo info( filename );
	m_baseDir = info.absolutePath(); // name is info.fileName()	

	// replace current volume with temporary one
	bool success = setWarp( filename );
	
	updateActions();
}

void VTKVisWidget::saveIsosurface()
{
	QString filename;
	filename = QFileDialog::getSaveFileName( this,
		tr("Save Isosurface"),
		m_baseDir, tr("Wavefront OBJ (*.obj)") );

	// cancelled?
	if( filename.isEmpty() )
		return;

	//statusBar()->showMessage( tr("Exporting Isosurface to %1...").arg(filename) );
	m_vren.exportOBJ( filename.toAscii() );
	//statusBar()->showMessage( tr("Ready") );
}
