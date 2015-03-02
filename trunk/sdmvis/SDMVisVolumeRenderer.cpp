#include "SDMVisVolumeRenderer.h"
#include <QTimer>
#include <QMessageBox>
#include <QActionGroup>
#include <QSpinBox>
#include <VolumeRendering/VolumeData.h>
#include <VolumeRendering/VolumeUtils.h>  // load_volume(), create_volume_tex()
#include <VolumeRendering/VolumeRendererRaycast.h>
#include <GL/glew.h>
#include <GL/GLConfig.h>  // CheckGLError()
#include <iostream>
#include <vector>
#include "SDMVisConfig.h"
#include "BatchProcessingDialog.h"
#include "PleaseWaitDialog.h"

#include "mat/numerics.h"
#include "mat/mattools.h"
#include "plotWidget.h"
#include "TraitSelectionWidget.h"
#include "SDMVisInteractiveEditingOptionsWidget.h"

#define DEBUG_MAKECURRENT_BUG(funcname) 
	//std::cout << "DEBUG: makeCurrent() called in " << funcname << std::endl;

// Adjust pixel transfer for downloading vector fields to GPU.
// Note: OpenGL first scales, then adds bias before clamping to [0,1]
// TODO: Deduce pixel transfer parameters from data and pass into shader.
#define SDMVISVOLUMERENDERER_ADJUST_PIXEL_TRANSFER \
	gl_adjustPixeltransfer( 0.025f, 0.5f );  // (0.025,0.5) maps [-20,20] -> [0,1]
// was: 
//	gl_adjustPixeltransfer( 0.005f, 0.5f );  // (0.005f,0.5) maps [-100,100] -> [0,1]


//==============================================================================
//	OpenGL/Qt4 Helper functions (gl_...)
//==============================================================================

bool gl_adjustPixeltransfer( float comp_scale, float comp_bias )
{
	// adjust pixel transfer to shift/scale signed vectorfield data
	// (OpenGL first scales, then adds bias before clamping to [0,1] )
	unsigned int 
			GL_c_BIAS[3] = { GL_RED_BIAS, GL_GREEN_BIAS, GL_BLUE_BIAS },
			GL_c_SCALE[3] = { GL_RED_SCALE, GL_GREEN_SCALE, GL_BLUE_SCALE };
	for( int i=0; i < 3; ++i )
	{
		glPixelTransferf( GL_c_SCALE[i], comp_scale );
		glPixelTransferf( GL_c_BIAS[i] , comp_bias );
	}

	return !GL::CheckGLError("adjust_pixeltransfer()");
}

// loadMatrix() and multMatrix() copied from Qt demo boxes/scene.cpp
static void gl_loadMatrix(const QMatrix4x4& m)
{
    // static to prevent glLoadMatrixf to fail on certain drivers
    static GLfloat mat[16];
    const qreal *data = m.constData();
    for (int index = 0; index < 16; ++index)
        mat[index] = data[index];
    glLoadMatrixf(mat);
}

static void gl_multMatrix(const QMatrix4x4& m)
{
    // static to prevent glMultMatrixf to fail on certain drivers
    static GLfloat mat[16];
    const qreal *data = m.constData();
    for (int index = 0; index < 16; ++index)
        mat[index] = data[index];
    glMultMatrixf(mat);
}

static QMatrix4x4 gl_getMatrix( GLenum mname = GL_MODELVIEW_MATRIX )
{
	assert( mname == GL_MODELVIEW_MATRIX ||
	        mname == GL_PROJECTION_MATRIX );
	QMatrix4x4 m;
	static GLdouble mat[16];
	glGetDoublev( mname, mat );
	for( int i=0; i < 16; ++i )
		m( i%4, i/4 ) = mat[i];
	return m;
}

void draw_aabb( float xmin, float xmax, float ymin, float ymax, float zmin, float zmax )
{
	float v[8][3] = { {0,0,1}, {1,0,1}, {0,1,1}, {1,1,1},
	                  {1,0,0}, {0,0,0}, {0,1,0}, {1,1,0} };
	int   f[6][4] = { {0,1,3,2}, {1,4,7,3}, {4,5,6,7}, {5,0,2,6},
	                  {2,3,7,6}, {5,4,1,0} };

	for( int i=0; i < 8; ++i )
	{
		v[i][0] = (v[i][0] * (xmax - xmin)) + xmin;
		v[i][1] = (v[i][1] * (ymax - ymin)) + ymin;
		v[i][2] = (v[i][2] * (zmax - zmin)) + zmin;
	}

	glBegin( GL_QUADS ); // GL_TRIANGLE_FAN
	for( int i=0; i < 6; ++i )
		for( int j=0; j < 4; ++j )
		{
			glVertex3fv  ( v[f[i][j]] );
		}
	glEnd();
}


//==============================================================================
//	SDMVisVolumeRendererOptionsWidget
//==============================================================================

SDMVisVolumeRendererOptionsWidget::SDMVisVolumeRendererOptionsWidget( QWidget* parent )
	: QGroupBox(parent)
{
	m_isoSlider = new QSlider( Qt::Horizontal );
	m_isoSlider->setRange( 0, 100 );
	m_alphaSlider = new QSlider( Qt::Horizontal );
	m_alphaSlider->setRange( 0, 1000 );	

	m_stepSizeSpinBox = new QDoubleSpinBox;
	m_stepSizeSpinBox->setDecimals( 4 );
	m_stepSizeSpinBox->setRange( 0.0001, 0.01 );
	m_stepSizeSpinBox->setSingleStep( 0.0005 );
	QLabel* stepSizeLabel = new QLabel(tr("Stepsize"));
	stepSizeLabel->setBuddy( m_stepSizeSpinBox );
	QHBoxLayout* stepSizeLayout = new QHBoxLayout;
	stepSizeLayout->addWidget( stepSizeLabel );
	stepSizeLayout->addWidget( m_stepSizeSpinBox );

	QWidget* isoWidget = new QWidget;
	QHBoxLayout* isoLayout = new QHBoxLayout;
	QLabel* isoLabel = new QLabel( tr("Isovalue") );
	isoLabel->setBuddy( m_isoSlider );
	isoLayout->addWidget( isoLabel );
	isoLayout->addWidget( m_isoSlider );
	isoWidget->setLayout( isoLayout );

	QWidget* dvrWidget = new QWidget;
	QHBoxLayout* dvrLayout = new QHBoxLayout;
	QLabel* alphaLabel = new QLabel( tr("Alpha") );
	alphaLabel->setBuddy( m_alphaSlider );
	dvrLayout->addWidget( alphaLabel );
	dvrLayout->addWidget( m_alphaSlider );
	dvrWidget->setLayout( dvrLayout );

	modeStack = new QStackedWidget;
	modeStack->addWidget( isoWidget );
	//modeStack->addWidget( silWidget );
	modeStack->addWidget( dvrWidget );
	//modeStack->addWidget( mipWidget );

	modeCombo = new QComboBox;
	modeCombo->addItem( tr("Isosurface") );
	modeCombo->addItem( tr("Silhouette") );
	modeCombo->addItem( tr("DVR") );
	modeCombo->addItem( tr("MIP") );
	//connect( modeCombo, SIGNAL(activated(int)), modeStack, SLOT(setCurrentIndex(int)) );
	connect( modeCombo, SIGNAL(activated(int)), this, SLOT(dispatchRenderMode(int)) );

	m_integratorCombo = new QComboBox;
	m_integratorCombo->addItem( tr("Displacement field") );
	m_integratorCombo->addItem( tr("SVF Euler") );
	m_integratorCombo->addItem( tr("SVF Midpoint") );
	m_integratorCombo->addItem( tr("SVF RK4") );

	m_integratorStepsSpinBox = new QSpinBox;
	m_integratorStepsSpinBox->setRange( 1, 64 );
	
	QHBoxLayout* integratorLayout = new QHBoxLayout;
	integratorLayout->addWidget( m_integratorCombo );
	integratorLayout->addWidget( m_integratorStepsSpinBox );

	m_screenshotButton = new QPushButton(tr("Screenshot"));

	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget( modeCombo );
	layout->addWidget( modeStack );
	layout->addLayout( stepSizeLayout );
	layout->addLayout( integratorLayout );
	layout->addWidget( m_screenshotButton );
	setLayout( layout );

	modeCombo->setCurrentIndex(0);
	modeStack->setCurrentIndex(0);
	this->setEnabled( false );
	this->setTitle( tr("Rendering") );
}

// anonymous helper functions
namespace {
int mode2idx( int i )
{
	RaycastShader::RenderMode mode = (RaycastShader::RenderMode)i;
	if( mode==RaycastShader::RenderIsosurface ) return 0;
	if( mode==RaycastShader::RenderSilhouette ) return 1;
	if( mode==RaycastShader::RenderDirect     ) return 2;
	if( mode==RaycastShader::RenderMIP        ) return 3;
	return -1;
}
int idx2mode( int i )
{
	int ret=-1;
	switch( i )
	{
	case 0: ret = (int)RaycastShader::RenderIsosurface; break;
	case 1: ret = (int)RaycastShader::RenderSilhouette; break;
	case 2: ret = (int)RaycastShader::RenderDirect;     break;
	case 3: ret = (int)RaycastShader::RenderMIP;        break;
	}
	return ret;
}
} // anonymous namespace


void SDMVisVolumeRendererOptionsWidget::dispatchRenderMode( int i )
{
	// some modes share the same control widget
	if( i==0 || i==1 ) modeStack->setCurrentIndex(0);
	if( i==2 || i==3 ) modeStack->setCurrentIndex(1);

	int mode = idx2mode(i);
	if( i>=0 && i <RaycastShader::NumRenderModes )
		emit renderModeChanged( mode );
	else
		std::cerr << "SDMVisVolumeRendererOptionsWidget::dispatchRenderMode()"
			" : Warning: Unknown rendermode selected!" << std::endl;
}

void SDMVisVolumeRendererOptionsWidget::setSliderValueRelative( QSlider* slider, double value )
{
	int range = slider->maximum() - slider->minimum();
	int i_value = (int)(value * range);
	// clamping should be obsolete since QSlider forces min <= value <= max
	//if( i_value < 0      )  i_value = 0;
	//if( i_value >= range )  i_value = range-1;
	slider->setValue( slider->minimum() + i_value );
}

void SDMVisVolumeRendererOptionsWidget::connectMaster( SDMVisVolumeRenderer* master )
{
	disconnect( m_isoSlider,   SIGNAL(valueChanged(int)), 0,0 );
	disconnect( m_alphaSlider, SIGNAL(valueChanged(int)), 0,0 );
	disconnect( m_stepSizeSpinBox, SIGNAL(valueChanged(double)), 0,0 );
	disconnect( this, SIGNAL(isoValueChanged(double))  , 0,0 );
	disconnect( this, SIGNAL(alphaValueChanged(double)), 0,0 );
	disconnect( this, SIGNAL(stepSizeChanged(double)),   0,0 );
	disconnect( this, SIGNAL(renderModeChanged(int))   , 0,0 );
	disconnect( m_integratorCombo, SIGNAL(activated(int)), 0,0 );
	disconnect( m_screenshotButton, SIGNAL(clicked()), 0,0 );
	this->setEnabled( false );
	setSliderValueRelative( m_isoSlider, 0.0 );
	setSliderValueRelative( m_alphaSlider, 0.0 );

	if( master )
	{
		setSliderValueRelative( m_isoSlider, master->getIsovalueRelative() );
		setSliderValueRelative( m_alphaSlider, master->getAlphaRelative() );
		m_stepSizeSpinBox->setValue( master->getStepSize() );
		int modeidx = mode2idx( master->getRenderMode() );
		modeCombo->setCurrentIndex(modeidx);
		modeStack->setCurrentIndex(modeidx);
		m_integratorCombo->setCurrentIndex( master->getIntegrator() );
		m_integratorStepsSpinBox->setValue( master->getIntegratorSteps() );

		this->setEnabled( true );

		connect( m_isoSlider,   SIGNAL(valueChanged(int)), this, SLOT(isoValueChanged(int)) );
		connect( m_alphaSlider, SIGNAL(valueChanged(int)), this, SLOT(alphaValueChanged(int)) );
		connect( this, SIGNAL(isoValueChanged(double))  , master, SLOT(setIsovalueRelative(double)) );
		connect( this, SIGNAL(alphaValueChanged(double)), master, SLOT(setAlphaRelative(double)) );
		connect( m_stepSizeSpinBox, SIGNAL(valueChanged(double)), master, SLOT(setStepSize(double)) );
				// ..., this, SIGNAL(stepSizeChanged(double)) );
		connect( this, SIGNAL(renderModeChanged(int))   , master, SLOT(setRenderMode(int)) );
		connect( m_integratorCombo, SIGNAL(activated(int)), master, SLOT(setIntegrator(int)) );
		connect( m_integratorStepsSpinBox, SIGNAL(valueChanged(int)), master, SLOT(setIntegratorSteps(int)) );
		connect( m_screenshotButton, SIGNAL(clicked()), master, SLOT(makeScreenshot()) );
	}
}

void SDMVisVolumeRendererOptionsWidget::isoValueChanged( int i )
{
	emit isoValueChanged( (double)(i - m_isoSlider->minimum()) 
		                  / (m_isoSlider->maximum()-m_isoSlider->minimum()) );
}

void SDMVisVolumeRendererOptionsWidget::alphaValueChanged( int i )
{
	emit alphaValueChanged( (double)(i - m_alphaSlider->minimum()) 
		                  / (m_alphaSlider->maximum()-m_alphaSlider->minimum()) );
}


//==============================================================================
//	SDMVisVolumeRendererControlWidget
//==============================================================================

SDMVisVolumeRendererControlWidget::SDMVisVolumeRendererControlWidget( QWidget* parent )
	: QWidget(parent),
	  m_master(NULL)
{
	// render options

	m_vrenOpts = new SDMVisVolumeRendererOptionsWidget();

	// mode bars

	QStringList labels;
	for( int i=0; i < 5; ++i )
		labels << QString::number(i+1);

	m_bars = new BarPlotWidget();
	m_bars->setLabels( labels );
	m_bars->setValuesChangeable( true );
	m_bars->setEnabled( false );
	m_bars->setFrameStyle( QFrame::StyledPanel );

	QLabel* modeLabel = new QLabel(tr("Warp coefficients:"));
	QVBoxLayout* modeLayout = new QVBoxLayout;
	
	modeLayout->addWidget( modeLabel );
	modeLayout->addWidget( m_bars );	
	
	// scatter plot

	m_plotter= new PlotWidget();
	m_plotter->setDisabled(true);

	// trait selector

	m_traitSelector= new TraitSelectionWidget();
	m_traitSelector->hide();
	m_traitSelector->setDisabled(true);

	// layout

	QVBoxLayout* layout = new QVBoxLayout();
	
	layout->addWidget( m_vrenOpts );
	layout->addWidget( m_plotter,15 );
	layout->addWidget( m_traitSelector,15 );
	layout->addStretch( 1 );
	layout->addLayout( modeLayout );
	layout->setContentsMargins( 2,2,2,2 );  // reduce margin a bit
	setLayout( layout );
	
}

void SDMVisVolumeRendererControlWidget::connectMaster( SDMVisVolumeRenderer* master )
{
	m_master = master;
	if( master )
	{
		QStringList labels;
		for( int i=0; i < master->getLambdas().size(); ++i )
			labels << QString::number(i+1);

		m_bars->setValues( master->getLambdas() );
		m_bars->setLimits( -3.0, +3.0 );
		m_bars->setLabels( labels );
		m_bars->setEnabled( true );
		connect( m_bars, SIGNAL(valueChanged(int,double)), master, SLOT(setLambda(int,double)) );	
		connect( master, SIGNAL(lambdasChangedByAnimation()), this, SLOT(updateLambdas()) );		

		m_vrenOpts->connectMaster( master );
	}
	else
	{
		disconnect( master, SIGNAL(lambdasChangedByAnimation()), 0,0 );
		disconnect( m_bars, SIGNAL(valueChanged(int,double)), 0,0 );		
		QVector<double> zero;
		for( int i=0; i < m_bars->getValues().size(); ++i )
			zero.push_back( 0.0 );
		m_bars->setValues( zero );
		m_bars->setEnabled( false );

		m_vrenOpts->connectMaster( NULL );
	}
}

void SDMVisVolumeRendererControlWidget::updateLambdas()
{
	if( m_master ) {
		// Update bar plot
		m_bars->setValues( m_master->getLambdas() );
		m_bars->setLimits( -3.0, +3.0 ); // override auto limits from setValues()

		// Update scatter plot
		QVector<qreal> lambdas = m_master->getLambdas();
		for( unsigned i=0; i < (unsigned)lambdas.size(); i++ )
			m_plotter->updateMovingPointPosition( i, lambdas.at(i) );
	}
}

//==============================================================================
//	SDMVisVolumeRenderer
//==============================================================================

SDMVisVolumeRenderer::SDMVisVolumeRenderer( QWidget* parent, const QGLWidget* shareWidget, Qt::WindowFlags f )
#ifdef SDMVIS_USE_OVERDRAW
	: OverdrawQGLWidget( parent, shareWidget, f ),
#else
	: QGLWidget( parent, shareWidget, f ),
#endif
	  m_firstInitialization( true ),
	  m_initialized        ( false ),
	  m_sdm                ( NULL ),
	  m_config             ( NULL ),
	  m_vol                ( NULL ),
	  m_overlay            ( true ),
	  m_roi                ( .0f, .0f, .0f, 1.f ),
	  m_mode               ( ModeTrackball ),
	  m_cameraFOV          ( 30.f ),  // VTK default view angle is 30°
	  m_renderLock         ( false ),
	  m_modeString         ( tr("Mode") ),
	  m_editValidDeform    ( false )
{
	// Setup trackball 
	// Note that Trackball2::setViewSize() has to be called with actual OpenGL
	// viewport size before the trackball can be used. This is done in
	// \a resizeGL().
	m_trackball2.setSpeed( 100.f );


	m_volName = QString("(No reference volume loaded.)");
	
	// animation timer
	m_animationTimer = new QTimer( this );
	connect( m_animationTimer, SIGNAL(timeout()), this, SLOT(updateGL()) );

	// main renderer component
	m_vren = new VolumeRendererRaycast();

	// control widget
	myControlWidget = new SDMVisVolumeRendererControlWidget();
	//myControlWidget->connectMaster( this ); // --> connect when intialized!

	m_editWidget = new SDMVisInteractiveEditingOptionsWidget();
	
	// batch processing dialog
	m_batchproc = new BatchProcessingDialog( this ); // parent = this OK ?
	{
		// HACK: HARDCODED default environment and voltools path!

		// default environment
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		
		// Plön version (sdmvis+voltools+dlls in same directory)
		/*env.insert( "PATH", 
			QString("C:/Qt/4.7.0-beta2/bin") +
			QString("C:/Libs/vtk-5.6.0_build-x64/bin/Release") +
			env.value("PATH") );
		m_batchproc->process()->setProcessEnvironment( env );*/

		// default path for voltools programs
		m_batchVoltoolsDir = ""; //"G:/BITGrad/VolumeTools/bin_vs2010_x64/Release/";
	}

	// Rendering actions

	QAction* actReinitShader = new QAction( tr("&Reinit shader"), this );
	actReinitShader->setStatusTip( tr("Force reload of raycast GLSL shader used"
		                              " in direct volume rendering.") );
	connect( actReinitShader, SIGNAL(triggered()), this, SLOT(reinitShader()) );

	QAction* actToggleOverlay = new QAction( tr("Show &info overlay"), this );
	actToggleOverlay->setCheckable( true );
	connect( actToggleOverlay, SIGNAL(toggled(bool)), this, SLOT(toggleOverlay(bool)) );
	actToggleOverlay->setChecked( true );

	QAction* actChangeGradient = new QAction( tr("Choose background gradient colors"), this );
	connect( actChangeGradient, SIGNAL(triggered()), this, SLOT(changeGradientColors()) );

	m_showBBox = false;
	QAction* actShowBBox = new QAction( tr("Show bounding box"), this );
	actShowBBox->setCheckable( true );
	actShowBBox->setChecked( m_showBBox );
	connect( actShowBBox, SIGNAL(toggled(bool)), this, SLOT(toggleBBox(bool)) );

	// Debug actions

	QAction* actOffscreen = new QAction( tr("Offscreen rendering"), this );	
	QAction* actStopAnim  = new QAction( tr("Stop render update" ), this );
	QAction* actStartAnim = new QAction( tr("Start render update"), this );	
	actOffscreen->setCheckable( true );
	actOffscreen->setChecked( m_vren->getOffscreen() );
	connect( actOffscreen, SIGNAL(toggled(bool)), this, SLOT(toggleOffscreen(bool)) );
	connect( actStopAnim , SIGNAL(triggered()), this, SLOT(stopAnimation()) );
	connect( actStartAnim, SIGNAL(triggered()), this, SLOT(startAnimation()) );

	QAction* actShowColor = new QAction( tr("Show color output (default)"), this );
	QAction* actShowHit   = new QAction( tr("Show intersection output"   ), this );
	QAction* actShowNormal= new QAction( tr("Show normal output"         ), this );
	actShowColor ->setCheckable( true );
	actShowHit   ->setCheckable( true );
	actShowNormal->setCheckable( true );
	QActionGroup* actgroupShowOutput = new QActionGroup(this);
	actgroupShowOutput->addAction( actShowColor  );
	actgroupShowOutput->addAction( actShowHit    );
	actgroupShowOutput->addAction( actShowNormal );
	actgroupShowOutput->setExclusive( true );
	actShowColor->setChecked( true );
	connect( actgroupShowOutput, SIGNAL(triggered(QAction*)), this, SLOT(setOutputType(QAction*)) );

	actShowDebugInfo = new QAction( tr("Show debug infos"), this );
	actShowDebugInfo->setCheckable( true );
	actShowDebugInfo->setChecked( false );
	connect( actShowDebugInfo, SIGNAL(toggled(bool)), this, SLOT(toggleDebug(bool)) );


	// WORKAROUND: Keyboard shortcuts can only be assigned once!
	static bool keyboardShortcutAssigned = false;
	if( !keyboardShortcutAssigned )
	{
		// Assign shortcuts for first instance calling this code here
		actReinitShader->setShortcut( tr("Ctrl+R") );
		actToggleOverlay->setShortcut( tr("Ctrl+I") );
		actStopAnim ->setShortcut( tr("Ctrl+A") );
		actStartAnim->setShortcut( tr("Ctrl+S") );

		keyboardShortcutAssigned = true;
	}

	// Mode action group

	QAction* actModeTrackball = new QAction( tr("Mode Trackball"  )  , this );
	QAction* actModeSelectROI = new QAction( tr("Mode Select ROI" ), this );	
	QAction* actModePicking   = new QAction( tr("Mode Ray Picking"), this );	
	actModeTrackball->setCheckable( true );
	actModeSelectROI->setCheckable( true );
	actModePicking  ->setCheckable( true );
	QActionGroup* actgroupMode = new QActionGroup( this );
	actgroupMode->addAction( actModeTrackball );
	actgroupMode->addAction( actModeSelectROI );
	actgroupMode->addAction( actModePicking );
	actgroupMode->setExclusive( true );
	actModeTrackball->setChecked( true );
	QAction* sep_modes = new QAction( tr("Interaction Modes"), this );
	sep_modes->setSeparator( true );

	QAction* actShowEditOptions = new QAction( tr("Show edit options dialog"), this );
	connect( actShowEditOptions, SIGNAL(triggered()), this, SLOT(showEditDialog()) );

	m_actModeTrackball = actModeTrackball;
	m_actModeRubberbandEditing = actModePicking;

	// ROI actions

	m_showROI = false;
	QAction* actShowROI = new QAction( tr("Show ROI"), this );
	actShowROI->setCheckable( true );
	actShowROI->setChecked( m_showROI );
	connect( actShowROI, SIGNAL(toggled(bool)), this, SLOT(toggleROI(bool)) );
	connect( actgroupMode, SIGNAL(triggered(QAction*)), this, SLOT(triggerMode(QAction*)) );

	QAction* actSpecifyROI = new QAction( tr("Specify ROI manually..."), this );
	actSpecifyROI->setStatusTip( tr("Show dialog to manually specify position and radius of spherical region of interest.") );
	connect( actSpecifyROI, SIGNAL(triggered()), this, SLOT(specifyROI()) );

	QAction* actExportROI = new QAction( tr("Export ROI..."), this );
	actExportROI->setStatusTip( tr("Export current region of interest as volume dataset.") );
	connect( actExportROI, SIGNAL(triggered()), this, SLOT(exportROI()) );

	// Warp animation

	QAction* actWarpAnim = new QAction( tr("Animate warp"), this );
	actWarpAnim->setStatusTip( tr("Toggle animation of warp sigma parameter.") );
	actWarpAnim->setCheckable( true );
	actWarpAnim->setChecked( false );
	connect( actWarpAnim, SIGNAL(toggled(bool)), this, SLOT(toggleAnimateWarp(bool)) );
	
	m_animTimer = new QTimer( this );
	connect( m_animTimer, SIGNAL(timeout()), this, SLOT(animateWarp()) );

	QAction* actWarpAnimSpeed = new QAction( tr("Set animation speed"), this );
	connect( actWarpAnimSpeed, SIGNAL(triggered()), this, SLOT(changeWarpAnimationSpeed()) );

	QAction* actWarpAnimWarp = new QAction( tr("Set animation warp"), this );
	connect( actWarpAnimWarp, SIGNAL(triggered()), this, SLOT(changeWarpAnimationWarp()) );

	// Camera actions

	QAction* actLoadLookmark = new QAction( tr("Load lookmark..."), this );
	QAction* actSaveLookmark = new QAction( tr("Save lookmark..."), this );
	connect( actSaveLookmark, SIGNAL(triggered()), this, SLOT(saveCamera()) );
	connect( actLoadLookmark, SIGNAL(triggered()), this, SLOT(loadCamera()) );

	// Actions list

	// WORKAROUND: Separators can neither be created on-the-fly because we have
	//             to call setSeparator() explicitly nor can a separator be
	//             reused at different locations in an action list. Therefore
	//             we have to explicitly create required separators in advance.
	QAction* sep0 = new QAction( tr(""), this ); sep0->setSeparator( true );
	QAction* sep1 = new QAction( tr(""), this ); sep1->setSeparator( true );
	QAction* sep2 = new QAction( tr(""), this ); sep2->setSeparator( true );
	QAction* sep3 = new QAction( tr(""), this ); sep3->setSeparator( true );
	QAction* sep4 = new QAction( tr(""), this ); sep4->setSeparator( true );
	QAction* sep5 = new QAction( tr(""), this ); sep5->setSeparator( true );
	QAction* sep6 = new QAction( tr(""), this ); sep6->setSeparator( true );
	QAction* sep7 = new QAction( tr(""), this ); sep7->setSeparator( true );
	QAction* sep8 = new QAction( tr(""), this ); sep8->setSeparator( true );
	QAction* sep9 = new QAction( tr(""), this ); sep9->setSeparator( true );

	m_actionsRenderer.push_back( actReinitShader );
	m_actionsRenderer.push_back( sep0 );
	m_actionsRenderer.push_back( actModeTrackball );
	m_actionsRenderer.push_back( actModeSelectROI );
	m_actionsRenderer.push_back( actModePicking );
	m_actionsRenderer.push_back( sep9 );
	m_actionsRenderer.push_back( actShowEditOptions );
	m_actionsRenderer.push_back( sep1 );
	m_actionsRenderer.push_back( actChangeGradient );
	m_actionsRenderer.push_back( actOffscreen );
	m_actionsRenderer.push_back( sep2 );
	m_actionsRenderer.push_back( actToggleOverlay );
	m_actionsRenderer.push_back( actShowBBox );
	m_actionsRenderer.push_back( actShowROI );
	m_actionsRenderer.push_back( sep3 );
	m_actionsRenderer.push_back( actWarpAnim );
	m_actionsRenderer.push_back( actWarpAnimSpeed );
	m_actionsRenderer.push_back( actWarpAnimWarp );
	m_actionsRenderer.push_back( sep5 );
	m_actionsRenderer.push_back( actLoadLookmark );
	m_actionsRenderer.push_back( actSaveLookmark );
	m_actionsRenderer.push_back( sep6 );
	m_actionsDebug.push_back( actShowDebugInfo );	
	m_actionsDebug.push_back( sep7 );
	m_actionsDebug.push_back( actStopAnim );
	m_actionsDebug.push_back( actStartAnim );
	m_actionsDebug.push_back( sep8 );
	m_actionsDebug.push_back( actShowColor );
	m_actionsDebug.push_back( actShowHit );
	m_actionsDebug.push_back( actShowNormal );
	//m_actions.push_back( sep_modes );
	m_actionsAnalysis.push_back( sep4 );
	m_actionsAnalysis.push_back( actSpecifyROI );
	m_actionsAnalysis.push_back( actExportROI );

#if 0
	// Test overlay widget without layout
	QPushButton* overlayTest = new QPushButton( "Test overlay!", this );
	overlayTest->move( 100, 100 );
	BarPlotWidget* barplotTest = new BarPlotWidget( this );
	barplotTest->resize( 500, 100 );
	barplotTest->move( 100, 500 );
	QVector<double> foo; 
	foo.push_back(1.23); foo.push_back(-1.23); foo.push_back(2.3); foo.push_back(0.1);
	barplotTest->setValues( foo );
	QStringList labels;
	labels << "One" << "Two" << "Three" << "Four";
	barplotTest->setLabels( labels );
#endif
}

SDMVisVolumeRenderer::~SDMVisVolumeRenderer()
{
	delete m_vol; m_vol=NULL;
	m_vtex.Destroy();
	delete m_vren;
}

void SDMVisVolumeRenderer::showEditDialog()
{
	static bool firstRun = true;
	static QDialog* dialog = NULL;
	if( firstRun )
	{
		firstRun = false;
		dialog = new QDialog();
		dialog->setModal( false );
		QVBoxLayout* layout = new QVBoxLayout;
		layout->addWidget( m_editWidget );
		dialog->setLayout( layout );
		m_editWidget->connectMaster( &m_edit );
	}

	dialog->show();
}

int getGroupIndex( QAction* act )
{
	QActionGroup* group = act->actionGroup();
	if( group )
		return group->actions().indexOf( act );
	return -1;
}

void SDMVisVolumeRenderer::setOutputType( QAction* act )
{
	int type = getGroupIndex(act);
	if( type >= 0 )
	{
		m_vren->setOutputType( type );
		this->update();
	}
}

void SDMVisVolumeRenderer::triggerMode( QAction* act )
{
	int mode = getGroupIndex(act);
	if( mode >= 0 && mode < NumInteractionModes )
		m_mode = static_cast<InteractionMode>(mode);

#if 0
	// Special procedure when changing into specific modes
	if( m_mode == ModePicking )
		showEditDialog();
#endif
}

void SDMVisVolumeRenderer::setInitialized( bool b )
{
	m_initialized = b;
	myControlWidget->connectMaster( b ? this : NULL );
}

void SDMVisVolumeRenderer::toggleOffscreen( bool checked )
{
	m_vren->setOffscreen( checked );
}

void SDMVisVolumeRenderer::toggleDebug( bool checked )
{
	m_vren->setDebug( checked );
	this->update();
}

void SDMVisVolumeRenderer::toggleOverlay( bool checked )
{
	m_overlay = checked;
}

void SDMVisVolumeRenderer::toggleBBox( bool checked )
{
	m_showBBox = checked;
}

void SDMVisVolumeRenderer::toggleROI( bool checked )
{
	m_showROI = checked;
}


const VolumeDataHeader* SDMVisVolumeRenderer::getAnyWarpfield()
{
	if( m_vman.size() == 0 )
		return NULL;
	return m_vman.getVolume(0).volume();
}

const VolumeDataHeader* SDMVisVolumeRenderer::getAnyReferenceVolume()
{
	if( getAnyWarpfield() )
		return getAnyWarpfield();
	return m_vol;
}


//------------------------------------------------------------------------------
//	SDMVisVolumeRenderer -	ROI stuff
//------------------------------------------------------------------------------

void SDMVisVolumeRenderer::exportROI()
{
	using namespace std;

	QString filename = QFileDialog::getSaveFileName( this,
		tr("sdmvis: Export ROI..."), m_baseDir, tr("Volume description file (*.mhd)") );

	// user cancelled?
	if( filename.isEmpty() )
		return;

	saveROI( filename );
}

void SDMVisVolumeRenderer::saveROI( QString filename )
{
	using namespace std;

	SHOW_PLEASEWAIT_DIALOG( tr("sdmvis"),
		tr("Saving selected ROI to %1").arg(filename) )

	// get selection parameters
	float cx,cy,cz,
	      radius = m_roi.get_radius();
	m_roi.get_center( cx, cy, cz );

	// HACK: Aspect ratio correction should be done in interaction/rendering
	//       components, which would aid any refactoring ROI functionality into
	//       its own module.
	float aspx, aspy, aspz;
	m_vren->getAspect( aspx, aspy, aspz );
#if 0
	// convert normalized [-asp,+asp] coordinates to unit cube [0,1] coordinates
	cx = (cx + aspx) / (2*aspx);
	cy = (cy + aspy) / (2*aspy);
	cz = (cz + aspz) / (2*aspz);
	radius /= 2.f;
#endif

	cout << "Exporting spherical selection of radius " << radius 
		<< " centered at " << cx << "," << cy << "," << cz << "..." << endl;

	// creat mask volume

	VolumeDataAllocator<float> valloc;
	VolumeData<float>* mask = NULL;

	// If available we use the resolution of the deformation fields which must 
	// not be identical to the resolution of the reference volume.
	const VolumeDataHeader* vol = getAnyReferenceVolume();
	if( !vol ) {
		QMessageBox::warning( this, tr("sdmvis: Failed exporting ROI!"),
			tr("Exporting ROI not possible since no valid reference volume found!") );
		return;
	}

	int resX =vol->resX(), 
		resY =vol->resY(), 
		resZ =vol->resZ();
	double spacingX =vol->spacingX(), 
		   spacingY =vol->spacingY(), 
		   spacingZ =vol->spacingZ();

	cout << "Setting resolution of ROI volume to " << resX << "x" << resY << "x" << resZ << endl;
	cout << "Setting spacing of ROI volume to " << spacingX << ", " << spacingY << ", " << spacingZ << endl;

	mask = valloc.allocate( resX, resY, resZ, spacingX,spacingY,spacingZ );
	if( !mask )
	{
		HIDE_PLEASEWAIT_DIALOG()

		cerr << "Error: Couldn't allocate selection mask volume!" << endl;
		QMessageBox::warning( this, tr("sdmvis: Failed exporting ROI!"),
			tr("Couldn't allocate volume for selection mask!") );
		return;
	}

	// convert selection into volumetric mask
	cout << "Converting selection into volumetric mask..." << endl;
	for( int z=0; z < resZ; ++z )
		for( int y=0; y < resY; ++y )
			for( int x=0; x < resX; ++x )
			{
				float dx = (float)x/(resX-1),
					  dy = (float)y/(resY-1),
					  dz = (float)z/(resZ-1);				

		#if 1 //  CORRECT_ASPECT_RATIO
				// unit cube [0,1] to [-asp,+asp] normalized coordinates
				dx = (2*dx-1)*aspx;
				dy = (2*dy-1)*aspy;
				dz = (2*dz-1)*aspz;
		#endif

				float l2 = (dx-cx)*(dx-cx) + (dy-cy)*(dy-cy) + (dz-cz)*(dz-cz);
				if( l2 < radius*radius )
				{
					// inside spherical selection
					(*mask)(x,y,z) = 1.f;
				}
				else
				{
					// outside selection
					(*mask)(x,y,z) = 0.f;
				}
			}


	// save mask volume to disk
	cout << "Saving selection mask to " << filename.toStdString() << "..." << endl;
	VolumeDataWriterRAW<float> writer;
	writer.write( filename.toStdString().c_str(), mask, 10 );
	cout << "Successfully saved selection mask to " << filename.toStdString() << endl;

	HIDE_PLEASEWAIT_DIALOG()

	// free memory
	delete mask;
}

void SDMVisVolumeRenderer::specifyROI()
{
	float x, y, z, radius = m_roi.get_radius();
	double x_,y_,z_, r_;
	m_roi.get_center( x, y, z );	

	// TODO: real ROI dialog (showing current values, updating live any changes)

	bool ok;
	x_ = QInputDialog::getDouble( this, tr("sdmvis: ROI center X"), tr("Specify ROI center X"), x, -2.0,2.0, 3, &ok );
	if( !ok ) return;
	y_ = QInputDialog::getDouble( this, tr("sdmvis: ROI center Y"), tr("Specify ROI center Y"), y, -2.0,2.0, 3, &ok );
	if( !ok ) return;
	z_ = QInputDialog::getDouble( this, tr("sdmvis: ROI center Z"), tr("Specify ROI center Z"), z, -2.0,2.0, 3, &ok );
	if( !ok ) return;
	r_ = QInputDialog::getDouble( this, tr("sdmvis: ROI radius"), tr("Specify ROI radius"), radius, -3.0,3.0, 3, &ok );
	if( !ok ) return;

	m_roi.set_center( (float)x_, (float)y_, (float)z_ );
	m_roi.set_radius( (float)r_ );
}


//------------------------------------------------------------------------------
//	SDMVisVolumeRenderer -	Rendering
//------------------------------------------------------------------------------

void SDMVisVolumeRenderer::startAnimation()
{
	//m_animationTimer->start( 42 );
}

void SDMVisVolumeRenderer::stopAnimation()
{
	m_animationTimer->stop();
}

namespace {
	void gl_info( std::ostream& os )
	{
		os << "Vendor  : "<< (const char*)glGetString(GL_VENDOR)   << std::endl
		   << "Renderer: "<< (const char*)glGetString(GL_RENDERER) << std::endl
		   << "Version : "<< (const char*)glGetString(GL_VERSION)  << std::endl
		   << "GLSL    : "<< (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION) 
		   << std::endl;
		GL::checkGLError("gl_info");
	}
}

void SDMVisVolumeRenderer::initializeGL()
{		
	static bool firstRun = true;

	// OpenGL setup only required once since we are using a shared context in
	// the case of multiple instances.
	if( !firstRun )
	{
		std::cout << "Skipping additional OpenGL initialization, assuming a "
			         "shared context.\n";
		return;
	}

	// since we create the GL context, we have to initialize GLEW		
	glewExperimental = GL_TRUE;	
	GLenum glew_err = glewInit();
	if( glew_err != GLEW_OK )
	{
		std::cerr << "GLEW error:" << glewGetErrorString(glew_err) << std::endl;
		QMessageBox::warning( this, tr("sdmvis: Warning"),
			tr("Could not setup GLEW OpenGL extension manager!") );
	}
	if( !glewIsSupported("GL_VERSION_1_3") )
	{
		std::cerr << "GLEW error:" << glewGetErrorString(glew_err) << std::endl;
		QMessageBox::warning( this, tr("sdmvis: Warning"),
			tr("OpenGL 1.3 not supported by current graphics hardware/driver!"));
	}
	// Show GL infos
	{
		std::cout << "OpenGL Information" << std::endl
			      << "------------------" << std::endl;
		gl_info( std::cout );
		std::cout << "Using GLEW " << glewGetString( GLEW_VERSION ) << std::endl;
	}

	// create 3D texture
	m_vtex.Create( GL_TEXTURE_3D );  // only called once?

	// raycaster defaults
	m_vren->setZNear( 0.1f );
	//m_vren->setRenderMode( RaycastShader::RenderIsosurface );
	m_vren->setIsovalue( 0.15f );

	// opengl defaults
	glClearColor(1,0,0,1);

	if( firstRun )
		firstRun = false;
}

void SDMVisVolumeRenderer::resizeGL( int w, int h )
{
	float aspect = (float)w/h,
		  fov    = (float)m_cameraFOV,
		  znear  =   0.1f,
		  zfar   =  42.0f;

	glViewport( 0,0, w,h );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluPerspective( fov, aspect, znear, zfar );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	m_trackball2.setViewSize( w, h );

	//qDebug() << "SDMVisVolumeRenderer resized to " << w << " x " << h;
}

void SDMVisVolumeRenderer::invokeRenderUpdate()
{
	if( !m_renderLock )
	{
		makeCurrent();
		update();
		doneCurrent();
	}
}

//------------------------------------------------------------------------------
//	SDMVisVolumeRenderer -	Camera
//------------------------------------------------------------------------------

void SDMVisVolumeRenderer::saveCamera( QString filename )
{
	QSettings ini( filename, QSettings::IniFormat );
	ini.beginGroup("SDMVis_camera_lookmark_trackball2");
	ini.setValue( "fov"  , m_cameraFOV );
	ini.setValue( "zoom" , (double)m_trackball2.getZoom() );
	ini.setValue( "speed", (double)m_trackball2.getSpeed() );

	glm::fquat q = m_trackball2.getQuaternion();
	ini.setValue( "q_scalar", (double)q.w );
	ini.setValue( "q_x", (double)q.x );
	ini.setValue( "q_y", (double)q.y );
	ini.setValue( "q_z", (double)q.z );

	glm::vec3 v = m_trackball2.getTranslation();
	ini.setValue( "translation_x", (double)v.x );
	ini.setValue( "translation_y", (double)v.y );
	ini.setValue( "translation_z", (double)v.z );

#if 0
	for( int i=0; i < 4; ++i )
		for( int j=0; j < 4; ++j )
		{
			ini.setValue( tr("cameraMatrix_%1_%2").arg(i).arg(j), 
				m_cameraMatrix(i,j) );
		}
#endif
	ini.endGroup();
}

void SDMVisVolumeRenderer::saveCamera()
{
	QString filename = QFileDialog::getSaveFileName( this,
		tr("sdmvis: Save camera lookmark..."), 
		m_baseDir, tr("Camera lookmark (*.cam)") );

	if( filename.isEmpty() )
		return;

	saveCamera( filename );
}

Camera SDMVisVolumeRenderer::getCamera()
{	
	return Camera( m_trackball2, m_cameraFOV );
}

void SDMVisVolumeRenderer::setCamera( Camera cam )
{
	m_trackball2 = cam.getTrackball2();
	m_cameraFOV  = cam.getFOV();
	this->invokeRenderUpdate();
}

void SDMVisVolumeRenderer::loadCamera( QString filename )
{
	// camera positions could also put as list of "lookmarks" into config file.

	QSettings ini( filename, QSettings::IniFormat );

	ini.beginGroup("SDMVis_camera_lookmark_trackball2");

	glm::fquat q;
	q.w = (float)ini.value("q_scalar").toDouble();
	q.x = (float)ini.value("q_x").toDouble();
	q.y = (float)ini.value("q_y").toDouble();
	q.z = (float)ini.value("q_z").toDouble();
	
	float zoom = (float)ini.value( "zoom"  , m_trackball2.getZoom() ).toDouble();
	float fov  = (float)ini.value( "fov"   , m_cameraFOV            ).toDouble();

	glm::vec3 t = m_trackball2.getTranslation();
	t.x = (float)ini.value( "translation_x", t.x ).toDouble();
	t.y = (float)ini.value( "translation_y", t.y ).toDouble();
	t.z = (float)ini.value( "translation_z", t.z ).toDouble();
#if 0
	QMatrix4x4 cameraMatrix;
	for( int i=0; i < 4; ++i )
		for( int j=0; j < 4; ++j )
			cameraMatrix(i,j) = ini.value( tr("cameraMatrix_%1_%2").arg(i).arg(j)
				,m_cameraMatrix(i,j) ).toReal();
	m_cameraMatrix = cameraMatrix;
#endif
	ini.endGroup();
	
	// update camera
	m_trackball2.setTranslation( t );
	m_trackball2.setQuaternion( q );
	m_trackball2.setZoom( zoom );
	m_cameraFOV = fov;
	this->invokeRenderUpdate();
}

void SDMVisVolumeRenderer::loadCamera()
{
	QString filename = QFileDialog::getOpenFileName( this,
		tr("sdmvis: Load camera lookmark..."), 
		m_baseDir, tr("Camera lookmark (*.cam)") );

	if( filename.isEmpty() )
		return;

	loadCamera( filename );
}

//------------------------------------------------------------------------------
//	SDMVisVolumeRenderer -	Draw functions
//------------------------------------------------------------------------------

void SDMVisVolumeRenderer::drawBackgroundGradient()
{
	glPushAttrib( GL_ALL_ATTRIB_BITS );	

	glDepthMask( GL_FALSE );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_LIGHTING );
	glDisable( GL_CULL_FACE );
	glShadeModel( GL_SMOOTH );
	GLint viewport[4];
	glGetIntegerv( GL_VIEWPORT, viewport );
	GLfloat W = viewport[2],
	        H = viewport[3];
		
	// 1:1 pixel to coordinates relation
	glPushMatrix();
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D( 0,viewport[2],0,viewport[3] );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	float gradientA[3],// = { 1,1,1 },
	      gradientB[3];// = { .7,.7,.7 };

	m_gradient.getrgb( 0, gradientA );
	m_gradient.getrgb( 1, gradientB );


	glBegin( GL_QUADS );
	glColor3fv( gradientB ); glVertex2f( 0,0 );	glVertex2f( W,0 );
	glColor3fv( gradientA ); glVertex2f( W,H );	glVertex2f( 0,H );
	glEnd();

	glMatrixMode( GL_PROJECTION );
	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();

	glPopAttrib();
}

void drawLinesBegin( float lineWidth=1.5f )
{
	// antialiased lines
	glPushAttrib( GL_ALL_ATTRIB_BITS );
	glLineWidth( lineWidth );
	glEnable( GL_LINE_SMOOTH );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
}

void drawLinesEnd()
{
	glPopAttrib();
}

void SDMVisVolumeRenderer::drawEditArrow( QVector3D p, QVector3D q )
{
	// convert from unit [0,1] to aspect normalized [-asp,asp]
	float aspx, aspy, aspz;
	m_vren->getAspect( aspx, aspy, aspz );

	// antialiased lines
	drawLinesBegin( 2.f );

	glBegin( GL_LINES );
	glVertex3f( -aspx+2.*aspx*p.x(), -aspy+2.*aspy*p.y(), -aspz+2.*aspz*p.z() );
	glVertex3f( -aspx+2.*aspx*q.x(), -aspy+2.*aspy*q.y(), -aspz+2.*aspz*q.z() );				
	glEnd();

	drawLinesEnd();
}

void SDMVisVolumeRenderer::drawBackground()
{
	// background
	glClearColor( 0,0,0,0 );
	/* --> We are alpha blending with background for all raycaster modes!
	if( this->m_vren->getRenderMode() == RaycastShader::RenderDirect 
		|| this->m_vren->getRenderMode() == RaycastShader::RenderMIP 
	  )
	{
		glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
	}
	else
	*/
	{
		glClear( GL_DEPTH_BUFFER_BIT ); // | GL_COLOR_BUFFER_BIT
		drawBackgroundGradient();
	}
}

void SDMVisVolumeRenderer::paintGL()
{
	if( !isValid() )
		return;

	makeCurrent();

	// do nothing if not initialized
	if( !m_initialized ) 
	{
		// gray screen
		glClearColor( .5, .5, .5, 1 );
		glClear( GL_COLOR_BUFFER_BIT );
		return;
	}

	// background
	drawBackground();
	glColor4f( 1,1,1,1 );

	resizeGL( width(), height() );

	// Camera
	glLoadIdentity( );
	glm::mat4 modelview = m_trackball2.getCameraMatrix();
	glMultMatrixf( &modelview[0][0] );
	// was: gl_multMatrix( updateCameraMatrix(false) );

#if 1
	// DEBUG BUG Windows 7 lost context 
	{
		GLenum error;
		while( (error = glGetError()) != GL_NO_ERROR ) {
			qDebug("Uncatched OpenGL Error: %s\n", (char *)gluErrorString(error));
		}
	}
	//return;
#endif

	// raycast (blend result with background)

	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	m_vren->render();

	glDisable( GL_BLEND );

	// ray-picking
	//static bool validDeform = false;
	bool validDeform = m_editValidDeform;
	if( m_mode == ModePicking )
	{
		// Pick start point (could be saved)
		QPointF        pos0  = pixelPosToUnitPos( m_pickStartPos );
		RayPickingInfo info0 = m_vren->pick( pos0.x(), pos0.y() );		

		// Pick current point (no isosurface intersection needed)
		QPointF        pos1  = pixelPosToUnitPos( m_pickCurPos );
		RayPickingInfo info1 = m_vren->pick( pos1.x(), pos1.y() );		

		validDeform = info0.hasIntersection();
		m_editValidDeform = validDeform;

		if( validDeform )
		{
			// Solve edit
			m_edit.setPickedPoints( info0, info1 );

			// Apply new coefficients

			lockRenderUpdate();

			Vector copt = m_edit.getCoeffs();
			if( copt.size() >= 1 )
			{
				//for( int i=0; i < copt.size(); i++ )
				for( int i=0; i < 5; i++ )
				{
					// Clamp to [-3,3]
					double ci = copt(i);
					if( ci >  3.0 ) ci =  3.0;
					if( ci < -3.0 ) ci = -3.0;

					this->setLambda( i, ci );
				}
				emit lambdasChangedByAnimation();

				double E1 = m_edit.getErrorData(),
					   E2 = m_edit.getErrorReg();

				Vector dedit = m_edit.getDEdit();

				m_editWidget->setCoeffs( copt(0),copt(1),copt(2),copt(3),copt(4) );
				m_editWidget->setError( E1+E2, E1, E2 );
				m_editWidget->setDEdit( dedit(0), dedit(1), dedit(2) );
				
				SDMVisInteractiveEditing::VectorType p0, p1;
				m_edit.getStartEndPoints( p0, p1 );
				m_editWidget->setPos0( p0.x(), p0.y(), p0.z() );
			}

			// raycast deformed result
			drawBackground();  // clear background first
			m_vren->render();

			unlockRenderUpdate();

			emit pickedRay( info0 );
		}
	}

	// draw user displacement vector
	if( validDeform )
	{
		// Non displaced start/end points
		QVector3D p0,q0;
		m_edit.getStartEndPoints(p0,q0,false);

		// Displaced start/end points
		QVector3D p1,q1;
		m_edit.getStartEndPoints(p1,q1,true);

		if( m_editWidget->getShowDebugRubberband() )
		{			
			glColor3f( 1.f, 1.f, 0.f );
			drawEditArrow( p0, q0 );
		}

		if( m_editWidget->getShowRubberband() )
		{
			double fudge = m_editWidget->getRubberFudgeFactor();

			// Render displaced rubber band:
			// - start point p is displaced according to current edit field
			// - start point is interpolated with non-displaced one 
			// - end point q remains the same to match mouse cursor
			glColor3f( 1.f, 0.f, 0.f );
			drawEditArrow( p0 + (p1 - p0)*fudge, q0 );
		}
	}

	// draw spherical ROI
	if( m_mode == ModeSelectROI || m_showROI )
	{
		drawLinesBegin( 1.5f );
		glColor3f( 1.f, 1.f, 0.f );
		m_roi.draw();
		drawLinesEnd();
	}

	// draw bounding box
	if( m_showBBox )
	{
		float aspx, aspy, aspz;
		m_vren->getAspect( aspx, aspy, aspz );

		drawLinesBegin( 1.2f );
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		glColor3f( 1,1,1 );
		draw_aabb( -aspx,aspx, -aspy,aspy, -aspz,aspz );

	  #if 0
		glColor3f( .5f, 1.f, 1.f );
		draw_aabb( -1.f,1.f, -1.f,1.f, -1.f,1.f );

		glColor3f( 0, 1, 0 );
		draw_aabb( 0,1, 0,1, 0,1 );
		
		glColor3f( 1, 0, 0 );
		draw_aabb( 0,aspx, 0,aspy, 0,aspz );
	  #endif

		drawLinesEnd();
	}

	// overlay additional information
	if( m_overlay )
	{
		// setup font
		static QFont labelFont("Helvetica",12);		
		static QFont smallFont("Helvetica",11);
		const int lineHeight = 16;

#ifndef SDMVIS_USE_OVERDRAW // Default overlay is now drawn in overpaint()

		labelFont.setStyleStrategy( QFont::PreferAntialias );
		labelFont.setBold( true );
		smallFont.setStyleStrategy( QFont::PreferAntialias );
		glColor3f( .42f, .42f, .42f );

		// show visualization title
		if( !m_title.isEmpty() )
		{
			// Use the given title
			renderText( 10,lineHeight, m_title, labelFont );
		}
		else
		if( m_config )
		{
			// Use SDM config name
			renderText( 10,lineHeight, m_config->getConfigName(), labelFont );
		}
		else
		{
			// Use reference volume MHD filename
			QString name = m_volName;
			name.replace(".raw","");
			renderText( 10,lineHeight,tr("Volume: %1").arg( name ), labelFont );
		}

		// mode hint
		if( m_mode == ModePicking )
		{
			QFontMetrics fm( labelFont );
			QString text = tr("(Rubberband editing active)");
			int text_width = fm.width(text);

			glPushAttrib( GL_CURRENT_BIT );
			glColor3f( 1,0,0 );
			renderText( width()/2-text_width/2,lineHeight, text, labelFont );
			glPopAttrib();
		}

		// warpfield info (including current lambda values, useful for screenshots) 
		for( int i=0; i < m_warpfields.size(); ++i )
		{
			double lambda = getLambda( i );
			QFileInfo info(m_warpfields.at(i).mhdFilename);

			QString desc;
			if( m_warpfields.size() > 1 )
				desc = tr("%1 %2: %3 %4")
						.arg( m_modeString )
						.arg( i+1 ) 
						.arg( "" ) //info.baseName() )
						.arg( lambda, 0, 'f', 2 );
			else
				desc = tr("%1: %2 %3")
						.arg( m_modeString )
						.arg( "" ) //info.baseName() )
						.arg( lambda, 0, 'f', 2 );

			renderText( 10, (i+2)*lineHeight, desc, smallFont );
		}

	 // Rendertext together with paint engine is not allowed!
		// DEBUG info
		if( actShowDebugInfo->isChecked() )
		{
			float cx,cy,cz;
			m_roi.get_center(cx,cy,cz);		
			static QFont debugFont("Helvetica",10);
			debugFont.setStyleStrategy( QFont::PreferAntialias );
			glColor3f( 1.f, .42f, .42f );

			// Debug textures labels
			renderText(  10,height()-80, tr("Front" ), debugFont );
			renderText( 110,height()-80, tr("Back"  ), debugFont );
			renderText( 210,height()-80, tr("Pick"  ), debugFont );
			renderText( 310,height()-80, tr("Normal"), debugFont );
			renderText( 410,height()-80, tr("Front" ), debugFont );
			renderText( 510,height()-80, tr("Back"  ), debugFont );

			// Debug render view
			renderText( 10, height()-lineHeight-100, 
				tr("View: animTimer=%1")						
						.arg( (m_animTimer->isActive() ? "active" : "NOT active") ), 
						debugFont );

			if( m_mode == ModePicking )
			{
				renderText( 10, height()-2*lineHeight-100,
					tr("Ray-picking from (%1,%2) to (%3,%4)")
					.arg(m_pickStartPos.x()).arg(m_pickStartPos.y())
					.arg(m_pickCurPos  .x()).arg(m_pickCurPos  .y()),
					debugFont );

				if( validDeform )
				{
					QVector3D p,q;
					m_edit.getStartEndPoints( p, q );
					renderText( 10, height()-3*lineHeight-100,
						tr("Deformation from (%1,%2,%3) to (%4,%5,%6)")
						.arg(p.x()).arg(p.y()).arg(p.z())
						.arg(q.x()).arg(q.y()).arg(q.z()),
						debugFont );
				}
				else
				{
					renderText( 10, height()-3*lineHeight-100,
						tr("Invalid deformation."),
						debugFont );
				}
			}
			else
			if( m_mode == ModeSelectROI )
			{
				renderText( 10, height()-2*lineHeight-100, 
					tr("ROI selection: center=(%1,%2,%3), scale=%5")
							.arg( cx, 0,'f',2 )
							.arg( cy, 0,'f',2 )
							.arg( cz, 0,'f',2 )
							.arg( m_roi.get_radius(), 0,'f',2 ),
							debugFont );
			}
		}
#endif  // ifndef SDMVIS_USE_OVERDRAW 
	}

#ifdef _DEBUG
	GLenum error;
	while( (error = glGetError()) != GL_NO_ERROR ) {
		qDebug("Uncatched OpenGL Error: %s\n", (char *)gluErrorString(error));
	}
#endif
}

void SDMVisVolumeRenderer::setTitle( QString title )
{
	m_title = title;
}

#ifdef SDMVIS_USE_OVERDRAW 
//------------------------------------------------------------------------------
//	SDMVisVolumeRenderer -	overdraw()
//------------------------------------------------------------------------------
void SDMVisVolumeRenderer::overdraw( QPainter& painter )
{
	//painter.fillRect( QRect(0,height()/2-50,width(),100), QColor(0,0,255,127) );
	//painter.setPen( QColor(255,255,0) );
	//painter.setFont(QFont("Arial", 30));
	//painter.drawText(rect(), Qt::AlignCenter, "Hello, OpenGL!");

	if( !m_overlay )
		return;

	// setup font
	static QFont labelFont("Helvetica",12);		
	static QFont smallFont("Helvetica",11);
	const int lineHeight = 16;
	labelFont.setStyleStrategy( QFont::PreferAntialias );
	labelFont.setBold( true );
	smallFont.setStyleStrategy( QFont::PreferAntialias );
	
	painter.setPen( QColor( 107, 107, 107 ) );

	if( m_config )
	{
		// SDM config info
		painter.setFont( labelFont );
		painter.drawText( 10,lineHeight, m_config->getConfigName() );
	}
	else
	{
		// volume info
		QString name = m_volName;
		name.replace(".raw","");
		painter.setFont( labelFont );
		painter.drawText( 10,lineHeight,tr("Volume: %1").arg( name ) );
	}

	// mode hint
	if( m_mode == ModePicking )
	{
		QRect r = QRect( rect().x(), rect().y(), rect.width(), rect().y()+lineHeight );
		painter.save();
		painter.setFont( labelFont );
		painter.setPen( QColor( 255, 0, 0 ) );
		painter.drawText( r, Qt::AlignCenter, tr("(Rubberband editing active)") );
		painter.restore();
	}

	// warpfield info (including current lambda values, useful for screenshots) 
	for( int i=0; i < m_warpfields.size(); ++i )
	{
		double lambda = getLambda( i );
		QFileInfo info(m_warpfields.at(i).mhdFilename);

		QString desc;
		if( m_warpfields.size() > 1 )
			desc = tr("%1 %2: %3 %4")
					.arg( m_modeString )
					.arg( i+1 ) 
					.arg( "" ) //info.baseName() )
					.arg( lambda, 0, 'f', 2 );
		else
			desc = tr("%1: %2 %3")
					.arg( m_modeString )
					.arg( "" ) //info.baseName() )
					.arg( lambda, 0, 'f', 2 );

		painter.setFont( smallFont );
		painter.drawText( 10, (i+2)*lineHeight, desc );
	}

}
#endif // ifdef SDMVIS_USE_OVERDRAW 

//------------------------------------------------------------------------------
//	SDMVisVolumeRenderer -	Mouse interaction
//------------------------------------------------------------------------------

QPointF SDMVisVolumeRenderer::pixelPosToViewPos(const QPointF& p)
{
    return QPointF(2.0 * float(p.x()) / width() - 1.0,
                   1.0 - 2.0 * float(p.y()) / height());
}

QPointF SDMVisVolumeRenderer::pixelPosToUnitPos(const QPointF& p)
{
    return QPointF(float(p.x()) / width(),
                   1.0 - float(p.y()) / height());
}

QVector3D SDMVisVolumeRenderer::pixelPosToSelectionVec( const QPointF& p )
{
	// NOT IMPLEMENTED!
	return QVector3D();
}

void SDMVisVolumeRenderer::mouseMoveEvent( QMouseEvent* e )
{	
	QGLWidget::mouseMoveEvent( e );
	if( e->isAccepted() )
		return;

	m_mousePos = e->pos();

	if( m_mode == ModeTrackball )
	{
		m_trackball2.update( e->pos().x(), e->pos().y() );
		e->accept();
		invokeRenderUpdate();
	}
	else
	if( m_mode == ModeSelectROI )
	{
		if( e->buttons() & Qt::LeftButton )
		{
			QVector3D w = pixelPosToSelectionVec( e->pos() );
			m_roi.set_center( w.x(), w.y(), w.z() );
			e->accept();
			invokeRenderUpdate();
		}
	}
	else
	if( m_mode == ModePicking )
	{
		if( e->buttons() & Qt::LeftButton )
		{
			m_pickCurPos = e->pos();
			e->accept();
			invokeRenderUpdate();
		}
	}
}

void SDMVisVolumeRenderer::mousePressEvent( QMouseEvent* e )
{		
	QGLWidget::mouseMoveEvent( e );
	if( e->isAccepted() )
		return;

	m_mousePos = e->pos();

	if( m_mode == ModeTrackball )
	{
		if( e->buttons() & Qt::LeftButton )
		{
			m_trackball2.start( e->pos().x(), e->pos().y(), Trackball2::Rotate );
			e->accept();
			invokeRenderUpdate();
		}
		else
		if( e->buttons() & Qt::MiddleButton )
		{
			m_trackball2.start( e->pos().x(), e->pos().y(), Trackball2::Translate );
			e->accept();
			invokeRenderUpdate();
		}
		else
		if( e->buttons() & Qt::RightButton )
		{
			int y_inverted = this->rect().height() - e->pos().y();
			m_trackball2.start( e->pos().x(), y_inverted, Trackball2::Zoom );
			e->accept();
			invokeRenderUpdate();
		}
	}
	else
	if( m_mode == ModeSelectROI )
	{
		if( e->buttons() & Qt::LeftButton )
		{
			QVector3D w = pixelPosToSelectionVec( e->pos() );
			m_roi.set_center( w.x(), w.y(), w.z() );
			e->accept();
			invokeRenderUpdate();
		}
	}
	else
	if( m_mode == ModePicking )
	{
		if( e->buttons() & Qt::LeftButton )
		{
			m_pickStartPos = e->pos();
			m_pickCurPos   = e->pos();
			e->accept();
			invokeRenderUpdate();
		}
	}
}

void SDMVisVolumeRenderer::mouseReleaseEvent( QMouseEvent* e )
{	
	QGLWidget::mouseMoveEvent( e );
	if( e->isAccepted() )
		return;

	m_mousePos = e->pos();

	if( m_mode == ModeTrackball )
	{
		if( e->button() == Qt::LeftButton )
		{
			m_trackball2.stop();
			e->accept();
			invokeRenderUpdate();			
		}
	}
	else
	if( m_mode == ModeSelectROI )
	{
		e->accept();
		invokeRenderUpdate();
	}
	else
	if( m_mode == ModePicking )
	{
		// nothing to do yet
	}
}

void SDMVisVolumeRenderer::wheelEvent( QWheelEvent* e )
{	
	QGLWidget::wheelEvent( e );
	if( e->isAccepted() )
		return;

	if( m_mode == ModeTrackball )
	{
		// Linear zoom
		float delta = (float)e->delta() / (10.f*m_trackball2.getSpeed());
		m_trackball2.setZoom( m_trackball2.getZoom() - delta );

		e->accept();
		invokeRenderUpdate();		
	}
	else
	if( m_mode == ModeSelectROI )
	{
		float r = m_roi.get_radius();
		if( e->delta() > 0 )
			r += 0.1f;
		else
			r -= 0.1f;
		m_roi.set_radius( r );

		e->accept();
		invokeRenderUpdate();
	}
}


//------------------------------------------------------------------------------
//	SDMVisVolumeRenderer -	Raycast shader config
//------------------------------------------------------------------------------

bool SDMVisVolumeRenderer::loadVolume( QString mhdFilename )
{
	using namespace std;
	int verbosity = 1;

	// load volume from disk
	void* dataptr = NULL;
	VolumeDataHeader* vol = load_volume( mhdFilename.toAscii(), verbosity, &dataptr );
	if( !vol )
	{
		cerr << "Error: Could not load volume " << mhdFilename.toStdString() << endl;
		return false;
	}

	// setup raycaster
	downloadVolume( vol, dataptr );

	// store volume (not necessarily the data, but header info is needed)
	if( m_vol )
	{
		m_vol->clear();
		delete m_vol; // delete any previously allocated volume
	}
	m_vol = vol;

	// free volume data
	m_vol->clear();
	//delete vol; // implicit in deletion of VolumeDataHeader?
	            // or call vol->clear() explicitly?
	
	return true;
}

void SDMVisVolumeRenderer::downloadVolume( VolumeDataHeader* vol, void* dataptr )
{
	int verbosity = 0;

	// activate GL context
	DEBUG_MAKECURRENT_BUG("downloadVolume")
	doneCurrent();
	makeCurrent();

	// download 3D texture
	if( !create_volume_tex( m_vtex, vol, dataptr, verbosity ) )
	{
		QMessageBox::warning( this, tr("sdmvis: Warning"),
			tr("Could not download volume data to GPU!") );
		setInitialized( false );
		return;
	}

	// (re)init raycast
	m_vren->setVolume( &m_vtex ); // FIXME: make setVolume() call implicit to init()
#if 0
	m_vren->setAspect( 1.f,                            // * vol->spacingX()
	                   vol->resY()/(float)vol->resX(), // * vol->spacingY()
	                   vol->resZ()/(float)vol->resX());// * vol->spacingZ()			
#else
	m_vren->setAspect( vol->spacingX(),
	                   vol->spacingY() * vol->resY()/(float)vol->resX(),
	                   vol->spacingZ() * vol->resZ()/(float)vol->resX());
#endif

	if( reinitShader() )
		m_volName = QString( vol->filename().c_str() );
	else
		m_volName = QString("(No reference volume loaded.)");
}

bool SDMVisVolumeRenderer::reinitShader()
{
	DEBUG_MAKECURRENT_BUG("reinitShader")
	doneCurrent();
	makeCurrent();
	stopAnimation();

	// set shader colormode
	int shader_colormode = 0;
	switch( m_colormode ) {
		default:
		case ColorPlain: shader_colormode = 0; break;
		case ColorCustom: shader_colormode = 1; break;
		case ColorTexture: break; // FIXME: not implemented yet!
	}	
	m_vren->setColorMode( shader_colormode );

try_again:
	bool success = false;	

	if( m_firstInitialization )
	{
		// suffices to call m_vren->init() only once
		success = m_vren->init( 512, 512 );
		m_firstInitialization = !success;
	}
	else
	{
		// if once initialized, only reload shader
		success = m_vren->reinit();
	}

	if( !success )    // Q: can m_vren->init() be called multiple times?
	{
		QMessageBox::StandardButton reply = 
		QMessageBox::question( this, tr("Error initializing raycast shader"),
			tr("Could not initialize raycast shader!\n"
			   "Select 'Ok' to try again and 'Cancel' to abort.\n"), 
			    QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel );
		
		if( reply == QMessageBox::Ok )
		{
			goto try_again;
		}

		// failed reloading shader
		setInitialized( false );
		return false;		
	}

	// success reloading shader
	setInitialized( true );
	startAnimation();
	// force render
	updateGL();
	return true;
}

void SDMVisVolumeRenderer::resetShader()
{
	// release current warpfields
	m_vman.clear();
	m_warpfields.clear();
	m_vren->resetWarpfields(); // reset shader		
	m_vren->setMeanwarp( NULL );
}

bool SDMVisVolumeRenderer::setMeanwarp( const Warpfield& meanwarp )
{
	if( !m_initialized )
	{
		QMessageBox::warning( this, tr("sdmvis: Error setting warpfields"),
			tr("Raycaster is not initialized!\n"
			   "Maybe you forgot to load a reference volume before "
			   "loading a set of warpfields?") );
		return false;
	}

	// Check if filename is empty
	std::string filename = meanwarp.mhdFilename.toStdString();
	if( filename.empty() ) 
		return true; // We do not consider this as failure

	// activate GL context
	makeCurrent();

	// only download volumes to GPU, do not keep in CPU memory
	int opts = VolumeManager::DownloadToGPU | VolumeManager::ReleaseCPUMemory;

	// adjust pixel transfer to shift/scale signed displacement vectorfield
	gl_adjustPixeltransfer( 0.05f, 0.5f ); // map [-10,10] to [0,1]

	// load from disk and upload to gpu
	if( !m_vman.loadVolume( filename, opts ) )
	{
		QMessageBox::warning( this, tr("sdmvis: Error settings warpfields"),
			tr("Could not load/setup warpfield %1").arg(QString::fromStdString(filename)) );

		gl_adjustPixeltransfer( 1, 0 );

		//// clear everything
		//resetShader();
		//setInitialized( false );
		m_vren->setMeanwarp( NULL );
		return false;
	}

	// reset pixel transfer
	gl_adjustPixeltransfer( 1, 0 );

	// re-init shader
	m_vren->setMeanwarp( m_vman.getVolume( filename ).texture() );

	if( !reinitShader() )
	{
		// FIXME: How to handle failure here? Keep warpfields in GPU mem?
		//        Possible to recover last working state somehow?
		resetShader();		
		setInitialized( false );
		return false;
	}

	return true;
}

bool SDMVisVolumeRenderer::setWarpfields( const QList<Warpfield>& warpfields )
{
	int size = warpfields.size();

	if( !m_initialized )
	{
		QMessageBox::warning( this, tr("sdmvis: Error setting warpfields"),
			tr("Raycaster is not initialized!\n"
			   "Maybe you forgot to load a reference volume before "
			   "loading a set of warpfields?") );
		return false;
	}
	if (size==0)
	{
		resetShader();
		return false;
	}

	// activate GL context
	DEBUG_MAKECURRENT_BUG("setWarpfields")
	doneCurrent();
	makeCurrent();

	// release current warpfields
	resetShader();

	// only download volumes to GPU, do not keep in CPU memory
	int opts = VolumeManager::DownloadToGPU | VolumeManager::ReleaseCPUMemory;

	// adjust pixel transfer to shift/scale signed displacement vectorfield
	// (OpenGL first scales, then adds bias before clamping to [0,1] )
	SDMVISVOLUMERENDERER_ADJUST_PIXEL_TRANSFER

	std::vector<GL::GLTexture*> modes( size, NULL );
	for( int i=0; i < size; ++i )
	{
		std::string filename = warpfields.at(i).mhdFilename.toStdString();

		// load from disk and upload to gpu
		if( !m_vman.loadVolume( filename, opts ) )
		{
			QMessageBox::warning( this, tr("sdmvis: Error settings warpfields"),
				tr("Could not load/setup warpfield %1").arg(warpfields.at(i).mhdFilename) );

			// clear everything
			resetShader();
			setInitialized( false );
			return false;
		}

		modes[i] = m_vman.getVolume( filename ).texture();
	}

	// reset pixel transfer
	gl_adjustPixeltransfer( 1, 0 );

	// re-init shader
	m_vren->setWarpfields( modes );

	if( !reinitShader() )
	{
		// FIXME: How to handle failure here? Keep warpfields in GPU mem?
		//        Possible to recover last working state somehow?
		resetShader();		
		setInitialized( false );
		return false;
	}

	// all warpfields successfully put on the GPU
	m_warpfields = warpfields;
	setInitialized( true );
	return true;
}

/// Get min/max scalar value in a data array.
template<typename T> void get_minmax( const T* data, size_t nelements, T& min_, T& max_ )
{
	min_ = std::numeric_limits<T>::max();
	max_ = -min_;

	for( unsigned i=0; i < nelements; i++ )	{
		const T& val = data[i];
		min_ = (val < min_) ? val : min_;
		max_ = (val > max_) ? val : max_;
	}
}

bool SDMVisVolumeRenderer::setWarpfieldsFromSDM( unsigned numModes )
{
	if( !m_sdm ) 
		return false;

	// Get eigenmodes from SDM

	// Warpfield list for elementScale variable (mhdFilename stays empty)
	QList<Warpfield> warpfields; 
	for( unsigned i=0; i < numModes; i++ )
		warpfields.append( Warpfield() );	
	
	// FIXME: Some code duplication from setWarpfields()

	int size = warpfields.size();

	if( !m_initialized )
	{
		QMessageBox::warning( this, tr("sdmvis: Error setting warpfields"),
			tr("Raycaster is not initialized!\n"
			   "Maybe you forgot to load a reference volume before "
			   "loading a set of warpfields?") );
		return false;
	}
	if (size==0)
	{
		resetShader();
		return false;
	}

	// activate GL context
	DEBUG_MAKECURRENT_BUG("setWarpfields")
	doneCurrent();
	makeCurrent();

	// release current warpfields
	resetShader();

	// only download volumes to GPU, do not keep in CPU memory
	int opts = VolumeManager::DownloadToGPU | VolumeManager::ReleaseCPUMemory;

	// adjust pixel transfer to shift/scale signed displacement vectorfield
	SDMVISVOLUMERENDERER_ADJUST_PIXEL_TRANSFER

	// Set warpfields
	std::vector<GL::GLTexture*> modes( size, NULL );
	for( int i=0; i < size; ++i )
	{
		StatisticalDeformationModel::Header header = m_sdm->getHeader();

		// Allocate raw buffer
		mattools::ValueType* rawdata = new mattools::ValueType[ m_sdm->getFieldSize() ];
		// Get i-th eigenmode
		m_sdm->getEigenmode( i, rawdata );

	#if 0 // HACK
		// Shift-scale parameters from first mode
		if( i==0 )
		{
			mattools::ValueType minval, maxval;
			get_minmax<mattools::ValueType>( rawdata, m_sdm->getFieldSize(), minval, maxval );

			// Map [minval,maxval] to [0,1]
			gl_adjustPixeltransfer( 1.f/(maxval-minval), minval/(maxval-minval) );
		}
	#endif

	#if 1 // DEBUG
		//if( i==0 )
		{
			std::stringstream ss; ss << "mode" << i << ".raw";
			std::ofstream f(ss.str(),std::ios::binary);
			if( f.is_open() )
				f.write( (char*)rawdata, m_sdm->getFieldSize()*sizeof(float) );
			f.close();
		}
	#endif

		// Put into VolumeDataHeader derived class instance
		VolumeDataSetFromRAW< mattools::ValueType > vol;
		vol.setSpacing( header.spacing[0], header.spacing[1], header.spacing[2] );
		VolumeDataHeader* volHeader = 
		  vol.setFromRaw( header.resolution[0], header.resolution[1], header.resolution[2],
			              rawdata, true );  // take ownership (of raw data)

		// Create Volume object for VolumeManager
		std::stringstream ss; ss << "SDM-Eigenmode-" << i;
		std::string name = ss.str();
		if( !m_vman.setVolume( name, volHeader, true, opts ) )
		{
			QMessageBox::warning( this, tr("sdmvis: Error settings warpfields"),
				tr("Could not load/setup warpfield %1").arg(warpfields.at(i).mhdFilename) );

			// clear everything
			resetShader();
			setInitialized( false );
			return false;
		}

		modes[i] = m_vman.getVolume( name ).texture();
	}

	// Set mean warp
	GL::GLTexture* meanwarp( NULL );
	if( m_sdm->getMeanPtr() )
	{
		qDebug() << "SDMVisVolumeRenderer::setWarpfieldsFromSDM() : Found non-zero mean warp!";

		StatisticalDeformationModel::Header header = m_sdm->getHeader();

		// Allocate raw buffer
		mattools::ValueType* rawdata = new mattools::ValueType[ m_sdm->getFieldSize() ];

		// Copy mean warp
		memcpy( (void*)rawdata, (void*)m_sdm->getMeanPtr(), m_sdm->getFieldSize()*sizeof(mattools::ValueType) );

		// Put into VolumeDataHeader derived class instance
		VolumeDataSetFromRAW< mattools::ValueType > vol;
		vol.setSpacing( header.spacing[0], header.spacing[1], header.spacing[2] );
		VolumeDataHeader* volHeader = 
		  vol.setFromRaw( header.resolution[0], header.resolution[1], header.resolution[2],
			              rawdata, true );  // take ownership (of raw data)

		// Create Volume object for VolumeManager
		std::string name = "SDM-Meanwarp";
		if( !m_vman.setVolume( name, volHeader, true, opts ) )
		{
			QMessageBox::warning( this, tr("sdmvis: Error setting mean warpfield"),
				tr("Could not setup mean warpfield from SDM!") );

			// clear everything
			resetShader();
			setInitialized( false );
			return false;
		}

		meanwarp = m_vman.getVolume( name ).texture();
	}

	// reset pixel transfer
	gl_adjustPixeltransfer( 1, 0 );

	// re-init shader
	m_vren->setWarpfields( modes );
	m_vren->setMeanwarp( meanwarp );

	if( !reinitShader() )
	{
		// FIXME: How to handle failure here? Keep warpfields in GPU mem?
		//        Possible to recover last working state somehow?
		resetShader();		
		setInitialized( false );
		return false;
	}

	// all warpfields successfully put on the GPU
	m_warpfields = warpfields;
	setInitialized( true );
	return true;	
}

//------------------------------------------------------------------------------
//	SDMVisVolumeRenderer -	Config
//------------------------------------------------------------------------------

void SDMVisVolumeRenderer::setSDM( StatisticalDeformationModel* sdm )
{
	m_sdm = sdm;
	m_edit.setSDM( sdm );
}

void SDMVisVolumeRenderer::setConfig( SDMVisConfig* config, bool update )
{
	if( config && update )
	{
		this->setWarpfields ( config->getWarpfields()  );		
	}

	this->setMeanwarp ( config->getMeanwarp() );

	m_config = config;
}

void SDMVisVolumeRenderer::setScale( int idx, double value )
{
	// TODO: Redundant to SDMVisMainWindow::setScale()!
	if( idx >=0 && idx < m_warpfields.size() )
	{
		// get relative lambda parameter (in [-3,3])
		double lambda = getLambda(idx);

		m_warpfields[idx].elementScale = value;

		// update lambda wrt to new elementScale
		setLambda( idx, lambda );
	}
}

QVector<double> SDMVisVolumeRenderer::getLambdas() const
{
	QVector<double> l;
	for( int i=0; i < m_warpfields.size(); ++i )
		l.push_back( getLambda(i) );
	return l;
}

void SDMVisVolumeRenderer::setLambdas( QVector<double> lambdas )
{
	if( !m_initialized )
		return;

	for( int i=0; i < lambdas.size(); i++ )
	{
		// consider warpfield elementScale (e.g. as specified in config file)
		double scale = m_warpfields.at(i).elementScale;
		m_vren->setLambda( i, (float)scale*lambdas.at(i) );
	}

	invokeRenderUpdate();
}

void SDMVisVolumeRenderer::setLambda( int i, double lambda )
{
	if( !m_initialized )
		return;
	
	// consider warpfield elementScale (e.g. as specified in config file)
	double scale = m_warpfields.at(i).elementScale;

	m_vren->setLambda( i, (float)scale*lambda );	

	invokeRenderUpdate();
}

double SDMVisVolumeRenderer::getLambda( int i ) const
{
	// sanity
	if( !m_initialized || i<0 || i>=m_warpfields.size() )
		return 0.0;

	// consider warpfield elementScale (e.g. as specified in config file)
	// (inverse scaling here)
	double inv_scale = 1.0 / m_warpfields.at(i).elementScale;

	return inv_scale * m_vren->getLambda( i );
}

double SDMVisVolumeRenderer::getIsovalueRelative() const
{
	if( !m_initialized ) 
		return 0.0;
	return m_vren->getIsovalue();
}

void SDMVisVolumeRenderer::setIsovalueRelative( double iso )
{
	if( !m_initialized )
		return;
	m_vren->setIsovalue( iso );
	invokeRenderUpdate();
}

double SDMVisVolumeRenderer::getAlphaRelative() const
{
	if( !m_initialized ) 
		return 0.0;
	return m_vren->getAlphaScale();
}

void SDMVisVolumeRenderer::setAlphaRelative( double alpha )
{
	if( !m_initialized )
		return;
	m_vren->setAlphaScale( alpha );
	invokeRenderUpdate();
}

double SDMVisVolumeRenderer::getStepSize() const
{
	if( !m_initialized ) 
		return 0.0;
	return m_vren->getStepsize();
}

void SDMVisVolumeRenderer::setStepSize( double step ) 
{
	if( !m_initialized )
		return;
	m_vren->setStepsize( step );
	invokeRenderUpdate();
}

void SDMVisVolumeRenderer::setRenderMode( int mode )
{
	if( !m_initialized )
		return;
	DEBUG_MAKECURRENT_BUG("setRenderMode")
	doneCurrent();
	makeCurrent();
	m_vren->setRenderMode( (RaycastShader::RenderMode) mode );
	invokeRenderUpdate();
	//doneCurrent();
}

int SDMVisVolumeRenderer::getRenderMode() const
{
	if( !m_initialized )
		return 0;
	return m_vren->getRenderMode();
}

int SDMVisVolumeRenderer::getIntegrator() const 
{
	return m_vren->getIntegrator();
}

void SDMVisVolumeRenderer::setIntegrator( int type )
{
	m_vren->setIntegrator( type );
}

int SDMVisVolumeRenderer::getIntegratorSteps() const 
{
	return m_vren->getIntegratorSteps();
}

void SDMVisVolumeRenderer::setIntegratorSteps( int steps )
{
	m_vren->setIntegratorSteps( steps );
}

void SDMVisVolumeRenderer::changeGradientColors()
{
	QColor color1 = QColorDialog::getColor( m_gradient.colors[0], this );
	if( color1.isValid() )
	{
		QColor color2 = QColorDialog::getColor( m_gradient.colors[1], this );
		if( color2.isValid() )
		{
			m_gradient.colors[0] = color1;
			m_gradient.colors[1] = color2;
		}
	}
	invokeRenderUpdate();
}

void SDMVisVolumeRenderer::makeScreenshot( QString filename )
{
	// OpenGL context is required
	makeCurrent();

	if( m_vren->getOffscreen() )
	{
		// Make screenshot from offscreen texture
		// Note that the aspect ratio is not considered here and the raw
		// offscreen texture will be stored to disk.

		int channels = 3; // 3 for RGB and 4 for RGBA
		int width  = m_vren->getTextureWidth(),
			height = m_vren->getTextureHeight();
		if( width<=0 || height<=0 )
		{
			qDebug() << "SDMVisVolumeRenderer::makeScreenshot() : "
				"Encountered empty offscreen buffer!";
			return;
		}
		unsigned char* pixels = new unsigned char[ width*height*channels ];
		int texid = m_vren->getRenderTexture();

		// Get texture data
		glBindTexture( GL_TEXTURE_2D, texid );
		glGetTexImage( GL_TEXTURE_2D, 0, channels==3 ? GL_RGB : GL_RGBA, 
		               GL_UNSIGNED_BYTE, (void*)pixels );
		glBindTexture( GL_TEXTURE_2D, 0 );

		// Process data
		unsigned char* processed = new unsigned char[ width*height*channels ];
		// 1) Flip upside down
		for( int y=0; y < height; y++ )
		{
			// Row-by-row copy
			memcpy( (void*)&processed[y*width*channels],          // dst
				    (void*)&pixels[(height-y-1)*width*channels],  // src
					width*channels );
		}
		// 2) Convert RGBA to ARGB (if alpha considered)
		if( channels==4 )
		{
			unsigned char* ptr = processed;
			for( int i=0; i < width*height; i++ )
			{
				unsigned char r = ptr[0], g = ptr[1], b = ptr[2], a = ptr[3];

				ptr[0] = a; 
				ptr[1] = r; 
				ptr[2] = g;
				ptr[3] = b;
				
				ptr += 4;
			}
		}

		// Create and save image
		{ 
			QImage img( processed, width, height, 
				  channels==3 ? QImage::Format_RGB888 : QImage::Format_ARGB32 );
			if( !img.save( filename ) )
			{
				QMessageBox::warning( this, APP_NAME,
					tr("An error occured on saving the image!") );
			}
			// Destruct QImage img before freeing pixels memory
		}

		// Free memory
		delete [] pixels;
		delete [] processed;
	}
	else
	{
		// Make screenshot from backbuffer
		QImage img = grabFrameBuffer();
		if( !img.save( filename ) )
		{
			QMessageBox::warning( this, APP_NAME,
				tr("An error occured on saving the image!") );
		}
	}
}

void SDMVisVolumeRenderer::makeScreenshot()
{
	// File dialog
	QString filename = QFileDialog::getSaveFileName( this, APP_NAME, QString(), 
		tr("Portable Network Graphics (*.png);;JPEG (*.jpg)") );
	if( filename.isEmpty() ) // User cancelled
		return;

	makeScreenshot( filename );
}

//------------------------------------------------------------------------------
//	SDMVisVolumeRenderer -	Warp animation
//------------------------------------------------------------------------------

void SDMVisVolumeRenderer::animateWarp()
{
	// sanity
	if( m_animWarp<0 || m_animWarp>=m_warpfields.size() )
		return;

	const  double step=0.1;
	static double sign=+1;
	double lambda = getLambda( m_animWarp );
	
	lambda += sign*step;
	if( lambda > +3. ) { lambda = +3.; sign*=-1; }
	if( lambda < -3. ) { lambda = -3.; sign*=-1; }

	setLambda( m_animWarp, lambda );

	emit lambdasChangedByAnimation();
}

void SDMVisVolumeRenderer::toggleAnimateWarp( bool enable )
{
	if( enable )
		m_animTimer->start( m_animSpeed );
	else
		m_animTimer->stop();
}

void SDMVisVolumeRenderer::changeWarpAnimationSpeed()
{
	bool ok;
	int i = QInputDialog::getInteger( this, tr("Change warp animation speed"),
				tr("Interval time in ms:"),
				m_animSpeed, 10, 1000, 100, &ok );
	if( ok )
	{
		m_animSpeed = i;
		m_animTimer->setInterval( m_animSpeed );
	}
}

void SDMVisVolumeRenderer::changeWarpAnimationWarp()
{
	bool ok;
	int i = QInputDialog::getInteger( this, tr("Change warp to animate"),
				tr("Warp index:"),
				m_animWarp, 0, 4, 1, &ok );
	if( ok )
		m_animWarp = i;
}