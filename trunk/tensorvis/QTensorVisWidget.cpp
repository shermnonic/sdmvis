#include "QTensorVisWidget.h"
#include "QTensorVisOptionsWidget.h"

#include <QtGui>
#ifdef QTENSORVISWIDGET_USE_QVTKWIDGET2
  #include <QVTKWidget2.h>
  #include <vtkGenericOpenGLRenderWindow.h>
#else
  #include <QVTKWidget.h>
#endif
#include <vtkRenderWindow.h>
#include <vtkCamera.h>
#include <vtkRenderLargeImage.h>
#include <vtkPNGWriter.h>
#include <vtkOBJExporter.h>
#include <vtkCornerAnnotation.h>

#include <iostream>

//------------------------------------------------------------------------------
//	D'tor
//------------------------------------------------------------------------------
QTensorVisWidget::~QTensorVisWidget()	
{	
	std::cout << "~QTensorVisWidget()\n";
}

//------------------------------------------------------------------------------
//	C'tor
//------------------------------------------------------------------------------
QTensorVisWidget::QTensorVisWidget( QWidget* parent )
: QWidget(parent),
  m_hasVolume(false),
  m_tdataProvider(NULL)
{	
	// -- VTK Visualizations --

	// VTK Renderer
	m_renderer = VTKPTR<vtkRenderer>::New();

	// Add visualizations to renderer	
	m_tvis  .setRenderer( m_renderer );
	m_volvis.setRenderer( m_renderer );

	// Some default render settings
	m_volvis.setOutlineVisibility( false );
	m_volvis.setContourVisibility( false );
	m_volvis.setVisibility( true );

	// -- Timer --

	m_animationTimer = new QTimer(this);
	connect( m_animationTimer, SIGNAL(timeout()), this, SLOT(forceRedraw()) );

	// -- Widgets --

#ifdef QTENSORVISWIDGET_USE_QVTKWIDGET2
	m_vtkWidget = new QVTKWidget2();	
#else
	m_vtkWidget = new QVTKWidget();	
#endif
	m_vtkWidget->GetRenderWindow()->AddRenderer( m_renderer );
	m_vtkWidget->GetRenderWindow()->SetPointSmoothing( 1 );
	m_vtkWidget->GetRenderWindow()->SetLineSmoothing( 1 );
	//m_vtkWidget->GetRenderWindow()->SetPolygonSmoothing( 1 );
	m_vtkWidget->GetRenderWindow()->SetMultiSamples( 4 );

	// -- Layout --

	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget( m_vtkWidget );
	layout->setContentsMargins( 0,0,0,0 );
	setLayout( layout );

	// -- Control dock with options--

	m_optionsWidget = new QTensorVisOptionsWidget;
	m_optionsWidget->setTensorVis( &m_tvis );

	connect( m_optionsWidget, SIGNAL(visChanged()), this, SLOT(redraw()) );
	connect( m_optionsWidget, SIGNAL(visChanged()), this, SLOT(updateColorMap()) );

	// -- Actions --

	// REMARK: The visibility actions below can be automated by introducing
	//         a thin GUI wrapper around the visualization classes which provide
	//         custom toggle actions, e.g. like QDockWidget::toggleViewAction.
	QAction* actOpenVolume;
	QAction* actOpenTensorData;
	QAction* actOpenTensorData2;
	QAction* actOpenTensorDistribution;
	QAction* actSaveTensorData;
	QAction* actOpenColorMap;
	QAction* actViewTensorVis;
	QAction* actViewVolVis;
	QAction* actEditScreenshot;
	QAction* actEditMakeModeAnimation;
	QAction* actBackgroundColors;
	QAction* actAnimationTimer;

	actOpenVolume     = new QAction( tr("Open volume dataset..."), this );
	actOpenTensorData = new QAction( tr("Open tensor dataset..."), this );
	actOpenTensorData2= new QAction( tr("Open second tensor dataset..."), this );
	actOpenTensorData2->setToolTip( tr("The second tensor field will be applied as outer weights to the first one.") );
	actOpenTensorDistribution = new QAction( tr("Open tensor distribution..."), this );
	actSaveTensorData = new QAction( tr("Save tensor dataset..."), this );
	actSaveTensorData->setToolTip( tr("Save the currently visible tensor dataset sampled at image resolution (stepsize 1) to disk.") );
	actOpenColorMap   = new QAction( tr("Open color map..."), this );
	actViewVolVis = new QAction( tr("Volume"), this );
	actViewVolVis->setCheckable( true );
	actViewVolVis->setChecked( m_volvis.getVisibility() );
	actViewTensorVis = new QAction( tr("Tensor Glyphs"), this );
	actViewTensorVis->setCheckable( true );
	actViewTensorVis->setChecked( true );
	actEditScreenshot = new QAction( tr("Screenshot"), this );
	actEditMakeModeAnimation = new QAction( tr("Make mode animation"), this );
	actBackgroundColors = new QAction( tr("Background colors"), this );
	actAnimationTimer = new QAction( tr("Start animation (30Hz)"), this );
	actAnimationTimer->setCheckable( true );
	actAnimationTimer->setChecked( false );
	connect( actViewVolVis,    SIGNAL(toggled(bool)), this, SLOT(toggleVolVis   (bool)) );
	connect( actViewTensorVis, SIGNAL(toggled(bool)), this, SLOT(toggleTensorVis(bool)) );
	connect( actEditScreenshot,SIGNAL(triggered()), this, SLOT(saveScreenshot()) );
	connect( actEditMakeModeAnimation, SIGNAL(triggered()), this, SLOT(showModeAnimationDialog()) );
	connect( actOpenVolume,    SIGNAL(triggered()), this, SLOT(openVolumeDataset()) );
	connect( actOpenTensorData,SIGNAL(triggered()), this, SLOT(openTensorDataset()) );
	connect(actOpenTensorData2,SIGNAL(triggered()), this, SLOT(openTensorDataset2()) );
	connect( actOpenTensorDistribution, SIGNAL(triggered()), this, SLOT(openTensorDistribution()) );
	connect( actSaveTensorData,SIGNAL(triggered()), this, SLOT(saveTensorDataset()) );
	connect( actOpenColorMap,  SIGNAL(triggered()), this, SLOT(openColorMap()) );
	connect(actBackgroundColors,SIGNAL(triggered()), this, SLOT(changeBackgroundColors()) );
	connect( actAnimationTimer, SIGNAL(toggled(bool)), this, SLOT(toggleAnimationTimer(bool)) );

	m_fileActions.push_back( actOpenVolume );
	m_fileActions.push_back( actOpenTensorData );
	m_fileActions.push_back( actOpenTensorData2 );
	m_fileActions.push_back( actOpenTensorDistribution );
	m_fileActions.push_back( actOpenColorMap );
	m_fileActions.push_back( actSaveTensorData );
	m_viewActions.push_back( actViewTensorVis );
	m_viewActions.push_back( actViewVolVis );
	m_viewActions.push_back( actBackgroundColors );
	m_viewActions.push_back( actAnimationTimer );
	m_editActions.push_back( actEditScreenshot );
	m_editActions.push_back( actEditMakeModeAnimation );

	// Re-signal status messages from options widget
	connect( m_optionsWidget, SIGNAL(statusMessage(QString)), 
		this, SIGNAL(statusMessage(QString)) );

	// Copy some actions to members
	m_animationTimerAction = actAnimationTimer;
	toggleAnimationTimer( false );

	// Mode animation
	m_modeAnimationWidget = new ModeAnimationWidget;
	connect( m_modeAnimationWidget, SIGNAL(bakeAnimation()), this, SLOT(bakeModeAnimation()) );
	connect( m_modeAnimationWidget, SIGNAL(renderAnimation()), this, SLOT(renderModeAnimation()) );
}

void QTensorVisWidget::toggleVolVis( bool visible )
{
	m_volvis.setVisibility( visible );
	redraw();
}

void QTensorVisWidget::toggleTensorVis( bool visible )		
{
	m_tvis.setVisibility( visible );
	redraw();
}

void QTensorVisWidget::toggleAnimationTimer( bool animate )
{
	// Run a bit faster than 30hz to produce smooth animation, e.g. for screen 
	// capture.
	if( animate )
	{
		m_animationTimer->start( 33 );
		m_animationTimerAction->setText(tr("Stop animation"));
	}
	else
	{
		m_animationTimer->stop();
		m_animationTimerAction->setText(tr("Start animation (30Hz)"));
	}
}

void QTensorVisWidget::forceRedraw()
{
	redraw();
}


//------------------------------------------------------------------------------
//	reset()
//------------------------------------------------------------------------------
void QTensorVisWidget::reset()
{		
	// was: setVolume( 0 ); ??
}

//------------------------------------------------------------------------------
//	updateColorMap()
//------------------------------------------------------------------------------
void QTensorVisWidget::updateColorMap()
{
	m_tvis.updateColorMap();
	m_colormap.applyTo( m_tvis.getLookupTable() );
	redraw();
}

//------------------------------------------------------------------------------
//	redraw()
//------------------------------------------------------------------------------
void QTensorVisWidget::redraw()
{
	m_renderer->GetRenderWindow()->Render();
	update();
}

//------------------------------------------------------------------------------
//	resetCamera()
//------------------------------------------------------------------------------
void QTensorVisWidget::resetCamera()
{
	vtkRenderer* ren = m_renderer;
	ren->ResetCamera();
		//ren->GetActiveCamera()->SetViewUp( 0, 1, 0 );
		//ren->GetActiveCamera()->SetRoll( 0 );
	redraw();
}

//------------------------------------------------------------------------------
//	updateTensorVis()
//------------------------------------------------------------------------------
void QTensorVisWidget::updateTensorVis()
{	
	m_tvis.updateGlyphs( m_tvis.getSamplingStrategy() );
	updateColorMap();
}

//------------------------------------------------------------------------------
//	setTensorDataProvider()
//------------------------------------------------------------------------------
void QTensorVisWidget::setTensorDataProvider( TensorDataProvider* ptr )
{
	m_tdataProvider = ptr;
	if( ptr )
	{
		m_tvis.setDataProvider( ptr );

		// WORKAROUND: Update GUI elements
		m_optionsWidget->setTensorVis( &m_tvis );

		// WORKAROUND: (Re-)set image mask
		if( m_hasVolume )
			ptr->setImageMask( m_volvis.getImageData() );
	}
}

//------------------------------------------------------------------------------
//	setTensorDataset()
//------------------------------------------------------------------------------
bool QTensorVisWidget::setTensorDataset( QString filename )
{
	// Legacy support of RAW files where the dimensionality / spacing / origin
	// is determined by the currently loaded image data.
	bool ok;
	if( filename.endsWith(".raw",Qt::CaseInsensitive) )
		ok = m_tdataFileProvider.setup( filename.toAscii(), m_volvis.getImageData() );
	else
		ok = m_tdataFileProvider.setup( filename.toAscii() );

	if( !ok )
	{
		QMessageBox::warning( this, tr("Error setting tensor field"),
			tr("Error: Could not setup tensor visualization correctly!") );
		return false;
	}

	// Set the built-in file data provider
	setTensorDataProvider( &m_tdataFileProvider );
	updateTensorVis();
	resetCamera();
	return true;
}

//------------------------------------------------------------------------------
//	setTensorDataset2()
//------------------------------------------------------------------------------
bool QTensorVisWidget::setTensorDataset2( QString filename )
{
	// Legacy support of RAW files where the dimensionality / spacing / origin
	// is determined by the currently loaded image data.
	bool ok;
	if( filename.endsWith(".raw",Qt::CaseInsensitive) )
		ok = m_tdataFileProvider2.setup( filename.toAscii(), m_volvis.getImageData() );
	else
		ok = m_tdataFileProvider2.setup( filename.toAscii() );

	if( !ok )
	{
		QMessageBox::warning( this, tr("Error setting second tensor field"),
			tr("Error: Could not setup tensor visualization correctly!") );
		return false;
	}

	// Switch to weighted mode, using this second tensor field as outer 
	// weighting to the first one. Silently assumes, the first one is already
	// loaded!

	m_tdataWeightedProvider.setTensorDataProvider( &m_tdataFileProvider );
	m_tdataWeightedProvider.setWeightsTensorDataProvider( &m_tdataFileProvider2 );

	// Set the built-in file data provider
	setTensorDataProvider( &m_tdataWeightedProvider );
	updateTensorVis();
	resetCamera();
	return true;
}

//------------------------------------------------------------------------------
//	setTensorDistribution()
//------------------------------------------------------------------------------
bool QTensorVisWidget::setTensorDistribution( QString filename )
{
	bool ok;
	ok = m_tdataGaussianProvider.setup( filename.toAscii() );

	if( !ok )
	{
		QMessageBox::warning( this, tr("Error setting tensor distribution"),
			tr("Error: Could not setup tensor visualization correctly!") );
		return false;
	}

	// Set the built-in data provider
	setTensorDataProvider( &m_tdataGaussianProvider );
	updateTensorVis();
	resetCamera();
	return true;
}

//------------------------------------------------------------------------------
//	setVolume()
//------------------------------------------------------------------------------
void QTensorVisWidget::setVolume( VTKPTR<vtkImageData> volume )
{
	m_volvis.setup( volume );
	m_hasVolume = true;

		// Volume was successfully loaded
		// Set as image mask
		if( m_tdataProvider )
			m_tdataProvider->setImageMask( m_volvis.getImageData() );

		resetCamera();
}

bool QTensorVisWidget::setVolume( QString mhdFilename )
{
	m_hasVolume = m_volvis.setup( mhdFilename.toAscii() );
	if( !m_hasVolume )  // was: filename.c_str()
	{
		// Volume could not be loaded
		QMessageBox::warning( this, tr("Error setting volume"),
			tr("Error: Could not setup volume rendering correctly!") );
	}
	else
	{
		// Volume was successfully loaded
		// Set as image mask
		if( m_tdataProvider )
			m_tdataProvider->setImageMask( m_volvis.getImageData() );

		resetCamera();
	}
	return m_hasVolume;
}

//------------------------------------------------------------------------------
//	openTensorDataset()
//------------------------------------------------------------------------------
void QTensorVisWidget::openTensorDataset()
{
	QString filename;
	filename = QFileDialog::getOpenFileName( this,
		tr("Open tensor dataset"),
		m_baseDir, tr("Custom tensor+vector data (*.mxvol);;"
		              "RAW tensor data file (*.raw)") );

	// cancelled?
	if( filename.isEmpty() )
		return;

	// extract absolute path and filename
	QFileInfo info( filename );
	m_baseDir = info.absolutePath(); // name is info.fileName()	

	// load tensor dataset
	emit statusMessage( tr("Loading tensor field %1...")
	                        .arg(info.fileName()) );

	if( !setTensorDataset( filename ) )
		// failure
		emit statusMessage( tr("Failed to load tensor data from %1")
								  .arg(info.fileName()) );
	else
		// success
		emit statusMessage( tr("Successfully loaded tensor data from %1")
								  .arg(info.fileName()) );
}

//------------------------------------------------------------------------------
//	openTensorDataset2()
//------------------------------------------------------------------------------
void QTensorVisWidget::openTensorDataset2()
{
	QString filename;
	filename = QFileDialog::getOpenFileName( this,
		tr("Open second tensor dataset"),
		m_baseDir, tr("Custom tensor+vector data (*.mxvol);;"
		              "RAW tensor data file (*.raw)") );

	// cancelled?
	if( filename.isEmpty() )
		return;

	// extract absolute path and filename
	QFileInfo info( filename );
	m_baseDir = info.absolutePath(); // name is info.fileName()	

	// load tensor dataset
	emit statusMessage( tr("Loading second tensor field %1...")
	                        .arg(info.fileName()) );

	if( !setTensorDataset2( filename ) )
		// failure
		emit statusMessage( tr("Failed to load second tensor data from %1")
								  .arg(info.fileName()) );
	else
		// success
		emit statusMessage( tr("Successfully loaded second tensor data from %1")
								  .arg(info.fileName()) );
}

//------------------------------------------------------------------------------
//	openTensorDistribution()
//------------------------------------------------------------------------------
void QTensorVisWidget::openTensorDistribution()
{
	QString filename;
	filename = QFileDialog::getOpenFileName( this,
		tr("Open tensor distribution"),
		m_baseDir, tr("Custom tensor normal distribution (*.nrrd)") );

	// cancelled?
	if( filename.isEmpty() )
		return;

	// extract absolute path and filename
	QFileInfo info( filename );
	m_baseDir = info.absolutePath(); // name is info.fileName()	

	// load tensor dataset
	emit statusMessage( tr("Loading tensor distribution %1...")
	                        .arg(info.fileName()) );

	if( !setTensorDistribution( filename ) )
		// failure
		emit statusMessage( tr("Failed to load tensor distribution data from %1")
								  .arg(info.fileName()) );
	else
		// success
		emit statusMessage( tr("Successfully loaded tensor distribution data from %1")
								  .arg(info.fileName()) );
}

//------------------------------------------------------------------------------
//	saveTensorDataset()
//------------------------------------------------------------------------------
bool QTensorVisWidget::saveTensorDataset( QString filename )
{	
	return m_tdataProvider->writeTensorData( filename.toAscii() );
}

void QTensorVisWidget::saveTensorDataset()
{
	QString filename;
	filename = QFileDialog::getSaveFileName( this,
		tr("Save current tensor field"),
		m_baseDir, tr("Custom tensor+vector data (*.mxvol);;"
		              "Custom tensorfield format (*.tensorfield)") );

	// cancelled?
	if( filename.isEmpty() )
		return;

	// extract absolute path and filename
	QFileInfo info( filename );
	m_baseDir = info.absolutePath(); // name is info.fileName()	

	// load tensor dataset
	emit statusMessage( tr("Saving tensor field to %1...")
	                        .arg(info.fileName()) );

	if( !saveTensorDataset( filename ) )
		// failure
		emit statusMessage( tr("Failed to save tensor data to %1")
								  .arg(info.fileName()) );
	else
		// success
		emit statusMessage( tr("Successfully saved tensor data to %1")
								  .arg(info.fileName()) );
}

//------------------------------------------------------------------------------
//	openVolumeDataset()
//------------------------------------------------------------------------------
void QTensorVisWidget::openVolumeDataset()
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
	emit statusMessage( tr("Loading volume dataset %1...").arg(info.fileName()) );
	if( !setVolume( filename ) )
	{
		// failure
		emit statusMessage( tr("Failed to load %1").arg(info.fileName()) );
	}
	else
	{
		// success
		emit statusMessage( tr("Successfully loaded %1").arg(info.fileName()) );
		emit loadedFile( info.fileName() );
	}
}

//------------------------------------------------------------------------------
//	setColorMap()
//------------------------------------------------------------------------------
bool QTensorVisWidget::setColorMap( QString filename )
{
	ColorMapRGB colormap;
	if( !colormap.read( filename.toStdString().c_str() ) )
	{
		return false;
	}

	colormap.applyTo( m_tvis.getLookupTable() );
	redraw();

	m_colormap = colormap;

	return true;
}

//------------------------------------------------------------------------------
//	openColorMap()
//------------------------------------------------------------------------------
void QTensorVisWidget::openColorMap()
{
	static QString colorBaseDir;

	QString filename;
	filename = QFileDialog::getOpenFileName( this,
		tr("Open RGB colormap"),
		colorBaseDir, tr("RGB colormap (*.map)") );

	// cancelled?
	if( filename.isEmpty() )
		return;

	// extract filename w/o path for messages
	QFileInfo info( filename );

	if( !setColorMap( filename ) )
		emit statusMessage( tr("Failed to load %1").arg(info.fileName()) );
	else
	{
		emit statusMessage( tr("Set colormap to %1").arg(info.fileName()) );
		colorBaseDir = info.absolutePath();
	}
}

//------------------------------------------------------------------------------
//	saveScreenshot()
//------------------------------------------------------------------------------
void QTensorVisWidget::saveScreenshot()
{
	// Query filename
	QString filename = QFileDialog::getSaveFileName( this,
		tr("Save screenshot as PNG"), 
		m_baseDir, tr("PNG image (*.png)") );
	
	// cancelled?
	if( filename.isEmpty() )
		return;
	
	saveScreenshot( filename );

	// Remember directory next time
	QFileInfo info(filename);
	m_baseDir = info.absolutePath();
}

void QTensorVisWidget::saveScreenshot( QString filename )
{
#if 0
	//-----------------------------
	// take screenshot with Qt4
	//-----------------------------

	// Pointer to render widget required
	if( !m_renderS ) {
		std::cout << "Warning: VisWidget::saveScreenshot() failed since pointer to render widget is NULL!\n";
		return;
	}

	QPixmap screenshot = QPixmap::grabWidget( m_renderS );
	screenshot.save( filename, "PNG" );

#else
	//-----------------------------
	// take screenshot with VTK 
	//-----------------------------
	
	vtkRenderer* renderer = m_renderer;
	// ... or get first renderer from collection
	//	 m_vtkWidget->GetRenderWindow()->GetRenderers()->GetFirstRenderer();
	if( !renderer ) {
		std::cout << "Warning: QTensorVisWidget::saveScreenshot() failed since pointer to renderer is NULL!\n";
		return;
	}

    vtkRenderLargeImage* renderLarge = vtkRenderLargeImage::New();
    renderLarge->SetInput( renderer );
	renderLarge->SetMagnification( 1 );
 
    vtkPNGWriter* writer = vtkPNGWriter::New();
    writer->SetInputConnection(renderLarge->GetOutputPort());
    writer->SetFileName(qPrintable(filename));
    writer->Write();
 
    writer->Delete();
    renderLarge->Delete(); 
#endif
}

//-----------------------------------------------------------------------------
//  exportOBJ()
//-----------------------------------------------------------------------------
void QTensorVisWidget::exportOBJ()
{	
	// Query filename
	QString filename = QFileDialog::getSaveFileName( this,
		tr("Export visible geometry as OBJ"), 
		m_baseDir, tr("Wavefront OBJ (*.obj)") );	
	
	// cancelled?
	if( filename.isEmpty() )
		return;
	
	exportOBJ( filename );
}

void QTensorVisWidget::exportOBJ( QString filename )
{
	VTKPTR<vtkOBJExporter> exporter = VTKPTR<vtkOBJExporter>::New();

	exporter->SetInput( m_renderer->GetRenderWindow() );
	exporter->SetFilePrefix(qPrintable(filename));
	exporter->Write();
}

//-----------------------------------------------------------------------------
//  changeGradientColors()
//-----------------------------------------------------------------------------

#include <QColorDialog>

void QTensorVisWidget::changeBackgroundColors()
{
	double color0[3], color1[3];
	getRenderer()->GetBackground( color0 );
	getRenderer()->GetBackground2( color1 );
		
	QColor qcol0( color0[0]*255., color0[1]*255., color0[2]*255. ),
		   qcol1( color1[0]*255., color1[1]*255., color1[2]*255. );

	qcol0 = QColorDialog::getColor( qcol0, this );
	if( qcol0.isValid() )
	{
		color0[0] = (double)qcol0.redF();
		color0[1] = (double)qcol0.greenF();
		color0[2] = (double)qcol0.blueF();

		QColor qcol1 = QColorDialog::getColor( qcol1, this );
		if( qcol1.isValid() )
		{
			// Set gradient

			color1[0] = (double)qcol1.redF();
			color1[1] = (double)qcol1.greenF();
			color1[2] = (double)qcol1.blueF();

			getRenderer()->SetBackground( color0 );
			getRenderer()->SetBackground2( color1 );
			getRenderer()->SetGradientBackground( 1 );

			redraw();
		}
		else
		{
			// Set single color

			getRenderer()->SetBackground( color0 );
			getRenderer()->SetGradientBackground( 0 );

			redraw();
		}
	}
}

//-----------------------------------------------------------------------------
//  Mode animation
//-----------------------------------------------------------------------------

void QTensorVisWidget::bakeModeAnimation()
{
	makeModeAnimation( true, true );
}

void QTensorVisWidget::renderModeAnimation()
{
	makeModeAnimation( false );
}

void QTensorVisWidget::makeModeAnimation( bool saveToDisk, bool allModes )
{
	ModeAnimationParameters p =
						m_modeAnimationWidget->getModeAnimationParameters();

	// Text overlay
	VTKPTR<vtkCornerAnnotation> corner = VTKPTR<vtkCornerAnnotation>::New();
	corner->SetMinimumFontSize( 20 );
	m_renderer->AddActor2D( corner );

	std::vector<int> modes;
	if( allModes )
	{
		for( int i=0; i < 6; i++ )
			modes.push_back( i );
	}
	else
		modes.push_back( p.mode );

	for( int i=0; i < modes.size(); i++ )
	{
		p.mode = modes.at(i);
		int frame=0;
		for( double val=p.range_min; val <= p.range_max; val += p.range_stepsize, frame++ )
		{
			// Text hint
			char hint[255];
			sprintf( hint, "Mode %d at %5.4f", p.mode, val );
			corner->SetText( 2, hint ); // 2 == upper left

			// Update visualization
			m_optionsWidget->setModeAnimation( p.mode, val );

			if( saveToDisk ) {
				// Save screenshot
				std::string filename = p.build_filename( frame, val );
				std::cout << "Writing image " << filename << std::endl;
				saveScreenshot( QString::fromStdString( filename ) );
			}
		}
	}

	m_renderer->RemoveActor2D( corner );
}

void QTensorVisWidget::showModeAnimationDialog()
{	
	static bool firstRun = true;
	static QDialog* dialog = NULL;
	if( firstRun )
	{
		firstRun = false;
		dialog = new QDialog();
		dialog->setModal( false );
		QVBoxLayout* layout = new QVBoxLayout;
		layout->setContentsMargins( 0,0,0,0 );
		layout->addWidget( m_modeAnimationWidget );
		dialog->setLayout( layout );
	}	
	dialog->show();
}

