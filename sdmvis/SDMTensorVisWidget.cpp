#include "SDMTensorVisWidget.h"
#include "QTensorVisOptionsWidget.h"
#include "SDMTensorProbeWidget.h"
#include "LocalCovarianceStatistics.h"
#include <QWidget>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QMessageBox>
#include <QTime>
#include <QTimer>
#include <QFileDialog>
#include <QPainter>
#include <QPaintEngine>
#include <QTextItem>
#include <iostream>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkLight.h>
#ifndef VTKPTR
#include <vtkSmartPointer.h>
#define VTKPTR vtkSmartPointer
#endif
#include <QVTKWidget2.h>


SDMTensorVisWidget::~SDMTensorVisWidget()
{}

SDMTensorVisWidget::SDMTensorVisWidget( QWidget* parent )
	: QTensorVisWidget(parent),
	  m_sdm(NULL),
	  m_superTensorDataProvider(NULL)
{
	// Set some default values for our datasets
	getTensorVis()->setGlyphScaleFactor( 10.0 );
	getTensorVis()->setThreshold( 23.0 );
	getTensorVis()->setGlyphType( TensorVisBase::Cylinder );
	getTensorVis()->setExtractEigenvalues( true );
	
	// Eigenvalue offset for halo
	m_halo.getTensorGlyph()->SetEigenvalueOffset( 1 );
	m_halo.getTensorGlyph()->SetEigenvalueOffsetValue( .2 ); // exaggerate :->
	
	// Setup local covariance tensor
	m_editLocalCovariance.setSphericalSamplingRadius( 5.0 );
	m_linearLocalCovariance.setGamma( 50.0 );
	m_linearLocalCovariance.setSphericalSamplingRadius( 5.0 );
		// For LinearLocalCovariance sampling radius has no effect but it only
		// used as radius for the visualization of the probing sphere.
	m_linearLocalCovariance.setTensorType( LinearLocalCovariance::CovarianceTensor );

	// Setup statistics
	m_tdataStatistics.setTensorVisBase( getTensorVis() );

	// Create/extend user interface
	setupUI();

	setTensorMethod( 0 );

	// VTK scene and interaction setup
	setupScene();
	setupInteraction();

	// Some more hard coded defaults	
	resetMarker();
	onChangedMarkerRadius( 5.0 );
}

//QPaintEngine* SDMTensorVisWidget::
//  paintEngine() const
//{
//	return QVTKWidget2::paintEngine();
//}
//
//void SDMTensorVisWidget::
//  paintEvent( QPaintEvent* e )
//{
//	//getVTKWidget()->paintEngine()->drawTextItem( QPointF(100,100), QTextItem(
//	//QPainter paint(this);
//	//paint.drawText( 100,100, tr("Hello, world!") );
//
//	//QVTKWidget::paintEvent( e );
//
//	//QPainter painter(this);
//	//painter.setPen(Qt::blue);
//	//painter.setFont(QFont("Arial", 30));
//	//painter.drawText(rect(), Qt::AlignCenter, "Hello, world!");
//}

void SDMTensorVisWidget::
  onAppliedProbe( int idx )
{
	// Set sphere marker
	
	// Currently, the tensor probes stores normalized coordinates as required
	// by our data provider, so we can not directly use the point coordinates.
	//double p[3];
	//p[0] = m_probeWidget->getProbe(idx).point[0];
	//p[1] = m_probeWidget->getProbe(idx).point[1];
	//p[2] = m_probeWidget->getProbe(idx).point[2];
	//m_sphereMarker.getActor()->SetPosition( p );

	double p[3];
#if 1
	// Restore exact position of user selection
	m_curTensorDataProvider->space().getPointFromNormalized( 
		m_probeWidget->getProbe(idx).point, p );
#else
	// Set to closest voxel position (real sample pos.!)
	int ijk[3];
	ijk[0] = m_probeWidget->getProbe(idx).ijk[0];
	ijk[1] = m_probeWidget->getProbe(idx).ijk[1];
	ijk[2] = m_probeWidget->getProbe(idx).ijk[2];
	m_curTensorDataProvider->space().getPoint( ijk, p );
#endif
	m_sphereMarker.getActor()->SetPosition( p );

	// Force UI update on control widget
	QTensorVisOptionsWidget* ctrl = dynamic_cast<QTensorVisOptionsWidget*>
		                              ( QTensorVisWidget::getControlWidget() );
	if( ctrl )
	{
		ctrl->setTensorVis( getTensorVis() );
	}

	// Update our own UI widgets
	updateUI();
	updateMarkerLabel();

	// Generate new tensor vis
	updateTensorVis();
}

void SDMTensorVisWidget::
  onCapturedProbe( int idx )
{
#if 0
	// Correct position and set to *current* marker position, not the last
	// one a computation was performed for. This allows to add new probe
	// settings without doing the tensor computation.
	double p[3];
	m_sphereMarker.getActor()->GetPosition( p );

	m_probeWidget->getProbe(idx).point[0] = p[0];
	m_probeWidget->getProbe(idx).point[1] = p[1];
	m_probeWidget->getProbe(idx).point[2] = p[2];

	// Communicate changes back to probe list widget
	m_probeWidget->updateListView();
#endif
}


void SDMTensorVisWidget::
  setupUI()
{
	// Probe list
	m_probeWidget = new SDMTensorProbeWidget;
	//m_probeWidget->setMasters( getTensorVis(), m_curTensorDataProvider );

	connect( m_probeWidget, SIGNAL(appliedProbe(int)), this, SLOT(onAppliedProbe(int)) );
	connect( m_probeWidget, SIGNAL(capturedProbe(int)), this,SLOT(onCapturedProbe(int)) );

	// Extended options widget
	m_extendedOptionsWidget = new QWidget();

	// Original controls
	QWidget* plainControls = QTensorVisWidget::getControlWidget();
	plainControls->setContentsMargins( 0,0,0,0 );
	dynamic_cast<QTensorVisOptionsWidget*>(plainControls)
		->setTensorVis( getTensorVis() ); // update UI to changed values

	// Group "Local covariance tensor"
	QPushButton* butUpdate = new QPushButton(tr("Update TensorVis"));
	QPushButton* butEditMarker = new QPushButton(tr("Set marker to current edit"));	
	QPushButton* butDragMarker = new QPushButton(tr("Drag marker"));
	butDragMarker->setCheckable( true );
	butDragMarker->setChecked( false );

	m_spinGamma = new QDoubleSpinBox();
	m_spinGamma->setRange( 0.0001, 1000.0 );
	m_spinGamma->setDecimals( 4 );

	m_spinRadius = new QDoubleSpinBox();
	m_spinRadius->setRange( 0.0001, 100.0 );
	m_spinRadius->setDecimals( 4 );
	m_spinRadius->setSingleStep( 0.0005 );

	m_labelPosition = new QLabel("(-,-,-)\n[-,-,-]");

	QComboBox* comboMethod = new QComboBox();
	comboMethod->addItem(tr("Covariance"));
	comboMethod->addItem(tr("Sample based"));
	comboMethod->addItem(tr("Preloaded field"));

	QComboBox* comboType = new QComboBox();
	comboType->addItem(tr("Covariance (Zpq*Zpq')"));
	comboType->addItem(tr("Interaction (Zpq)"));
	comboType->addItem(tr("Integral (Zpq'*Zpq)"));
	comboType->addItem(tr("Self covariance (Zqq*Zqq')"));
	comboType->addItem(tr("Self interaction (Zqq)"));
	comboType->setToolTip(tr("Applies only for linear local covariance method and not sample-based approach"));
	comboType->setCurrentIndex( m_linearLocalCovariance.getTensorType() );

	QCheckBox* chkHalo = new QCheckBox(tr("Show halo"));
	chkHalo->setChecked( false );

	QCheckBox* chkMarker = new QCheckBox(tr("Show marker"));
	chkMarker->setChecked( true );

	QPushButton* butResetMarker = new QPushButton(tr("Reset"));

	QDoubleSpinBox* spinEigenvalueOfs = new QDoubleSpinBox;
	spinEigenvalueOfs->setRange( 0.0, 3.0 );
	spinEigenvalueOfs->setValue( m_halo.getTensorGlyph()->GetEigenvalueOffsetValue() );

	QHBoxLayout* showLayout = new QHBoxLayout();
	showLayout->addWidget( chkMarker );
	showLayout->addWidget( chkHalo );
	showLayout->addWidget( butResetMarker );

	QVBoxLayout* ctrlButtonLayout = new QVBoxLayout();
	ctrlButtonLayout->addWidget( butDragMarker );
	ctrlButtonLayout->addWidget( butEditMarker );
	ctrlButtonLayout->addWidget( butUpdate );

	int row=0;
	QGridLayout* ctrlLayout = new QGridLayout();
	ctrlLayout->addWidget( new QLabel(tr("Probe position")), row,0      );
	ctrlLayout->addWidget( m_labelPosition,                  row,1      ); row++;
	ctrlLayout->addWidget( new QLabel(tr("Gamma")),          row,0      );
	ctrlLayout->addWidget( m_spinGamma,                      row,1      ); row++;
	ctrlLayout->addWidget( new QLabel(tr("Radius")),         row,0      );
	ctrlLayout->addWidget( m_spinRadius,                     row,1      ); row++;
	ctrlLayout->addWidget( new QLabel(tr("Method")),         row,0      );
	ctrlLayout->addWidget( comboMethod,                      row,1      ); row++;
	ctrlLayout->addWidget( new QLabel(tr("Tensor type")),    row,0      );
	ctrlLayout->addWidget( comboType,                        row,1      ); row++;	
	ctrlLayout->addLayout( showLayout,                       row,0, 1,2 ); row++;
	ctrlLayout->addWidget( new QLabel(tr("Halo size")),      row,0      );
	ctrlLayout->addWidget( spinEigenvalueOfs,                row,1      ); row++;

	// sub widgets to distinguish advanced and basic controls
	ctrlLayout->setContentsMargins( 0,0,0,0 );
	ctrlButtonLayout->setContentsMargins( 0,0,0,0 );

	QWidget* ctrlWidgetAdvanced = new QWidget;
	ctrlWidgetAdvanced->setLayout( ctrlLayout );

	QWidget* ctrlWidgetBasic = new QWidget;
	ctrlWidgetBasic->setLayout( ctrlButtonLayout );

	QVBoxLayout* ctrlCombinedLayout = new QVBoxLayout();
	ctrlCombinedLayout->addWidget( ctrlWidgetBasic );
	ctrlCombinedLayout->addWidget( ctrlWidgetAdvanced );
	QGroupBox* ctrlGroup = new QGroupBox(tr("Local covariance tensor"));
	ctrlGroup->setLayout( ctrlCombinedLayout );

	// Group "Tensor statistics"
	m_labelStatistics = new QLabel("");

	QPushButton* butComputeOverviewVis = new QPushButton(tr("Compute overview vis."));

	row=0;
	QGridLayout* statLayout = new QGridLayout();
	statLayout->addWidget( new QLabel( tr("Max.eigenvalue\n"
		                                  "Mean diffusivity\n"
		                                  "Fractional anisotropy")), row,0 );	
	statLayout->addWidget( m_labelStatistics,                        row,1 );      row++;
	statLayout->addWidget( butComputeOverviewVis,                    row,0, 1,2 ); row++;
	QGroupBox* statGroup = new QGroupBox(tr("Tensor statistics"));
	statGroup->setLayout( statLayout );
	
	// Layout
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget( ctrlGroup );
	layout->addWidget( m_probeWidget );
	layout->addWidget( statGroup );
	layout->addWidget( plainControls );
	layout->addStretch( 10 );

	m_extendedOptionsWidget->setLayout( layout );

	// Mark some options as advanced
	m_advancedOptions.clear();
	m_advancedOptions.push_back( ctrlWidgetAdvanced );
	m_advancedOptions.push_back( plainControls );

	// Update timer
	m_interactorUpdateTimer = new QTimer(this);
	connect( m_interactorUpdateTimer, SIGNAL(timeout()), this, SLOT(updateMarkerLabel()) );

	// Connections
	connect( butUpdate, SIGNAL(clicked()), this, SLOT(updateTensorVis()) );
	connect( butEditMarker, SIGNAL(clicked()), this, SLOT(setMarkerToEdit()) );
	connect( butDragMarker,SIGNAL(toggled(bool)), this, SLOT(toggleDragMarker(bool)) );
	connect( comboMethod, SIGNAL(currentIndexChanged(int)), this, SLOT(setTensorMethod(int)) );
	connect( comboType,   SIGNAL(currentIndexChanged(int)), this, SLOT(setTensorType(int)) );	
	connect( m_spinRadius, SIGNAL(valueChanged(double)), this, SLOT(onChangedMarkerRadius(double)) );
	connect( butComputeOverviewVis, SIGNAL(clicked()), this, SLOT(computeOverviewVis()) );
	connect( chkHalo,   SIGNAL(toggled(bool)), this, SLOT(toggleHalo(bool)) );
	connect( chkMarker, SIGNAL(toggled(bool)), this, SLOT(toggleMarker(bool)) );
	connect( butResetMarker, SIGNAL(clicked()), this, SLOT(resetMarker()) );
	connect( spinEigenvalueOfs, SIGNAL(valueChanged(double)), this, SLOT(setHaloSize(double)) );
}

void SDMTensorVisWidget::
  setExpertMode( bool enable )
{
	for( int i=0; i < m_advancedOptions.size(); i++ )
		m_advancedOptions[i]->setVisible( enable );

	getControlWidget()->update();
}

void SDMTensorVisWidget::
  computeOverviewVis()
{
	if( QMessageBox::warning(this,tr("SDMVis"),
		  tr("This will take a long time and can not be aborted!\n"
		   "Are you sure you want to start the computation?"),
		  QMessageBox::Ok | QMessageBox::Cancel) 
		!= QMessageBox::Ok )
		return;

	bool doSampleCovarianceWeighting =
		m_superTensorDataProvider ?
		( QMessageBox::question(this,tr("SDMVis"),
			tr("Use currently loaded tensor data as sample covariance?"),
			QMessageBox::Yes | QMessageBox::No )
			== QMessageBox::Yes ) : false;

	QString basepath = QFileDialog::getExistingDirectory(this,tr("SDMVis") );
	if( basepath.isEmpty() )
		return;

	emit statusMessage(tr("Setup local covariance statistics..."));

	// WORKAROUND: Temporarily override current settings:
	// - use IntegralTensor
	// - use sample covariance (if specified)
	int tensorType = m_linearLocalCovariance.getTensorType();
	TensorDataProvider* sampleCovariance = m_linearLocalCovariance.getSampleCovariance();
	m_linearLocalCovariance.setTensorType( LinearLocalCovariance::IntegralTensor );	
	if( doSampleCovarianceWeighting )
		m_linearLocalCovariance.setSampleCovariance( m_superTensorDataProvider );

	// Setup statistics module
	LocalCovarianceStatistics cvstat;
	cvstat.setIntegrator( LocalCovarianceStatistics::IntegrateTensors );
	cvstat.setTensorDataStatistics( m_tdataStatistics );
	cvstat.setTensorData( &m_linearLocalCovariance ); // was: m_curTensorDataProvider
	cvstat.setTensorVis( getTensorVis() );

	int step = getTensorVis()->getGridStepSize();
	cvstat.setSamplingResolution( step, step, step );
		// Note: We could also use the last valid stepsize from data provider.

	// Measure time
	QTime time;
	time.start();
	emit statusMessage(tr("Computing overview visualization..."));

	// Heavy computation...
	cvstat.compute();

	// Restore LinearLocalCovariance settings
	m_linearLocalCovariance.setTensorType( tensorType );
	if( doSampleCovarianceWeighting )
		m_linearLocalCovariance.setSampleCovariance( sampleCovariance );
	
	emit statusMessage(tr("Computing overview done, took %1 ms").arg(time.elapsed()));

	std::cout << "Saving overview results in " << basepath.toStdString() << "...\n";
	cvstat.save( (basepath.toStdString() + std::string("\\")).c_str() );
}

void SDMTensorVisWidget::
  setTensorType( int type )
{
	m_linearLocalCovariance.setTensorType( type );
}

void SDMTensorVisWidget::
  setTensorMethod( int method )
{
	switch( method )
	{
	case 0: 
		m_curTensorDataProvider = &m_linearLocalCovariance; 
		break;
	default:
	case 1: 
		m_curTensorDataProvider = &m_editLocalCovariance; 
		break;
	case 2:
		if( m_superTensorDataProvider )
			setTensorDataProvider( m_superTensorDataProvider );
		else
			QMessageBox::warning( this, tr("SDMVis Warning"), 
				tr("No valid tensor data available!") );
		break;
	}

	// Set custom data provider
	setTensorDataProvider( m_curTensorDataProvider );

	// Activate probe list
	m_probeWidget->setMasters( getTensorVis(), m_curTensorDataProvider );

	// Set UI values according to current TensorDataProvider
	updateUI();
}

void SDMTensorVisWidget::
  setTensorDataProvider( TensorDataProvider* ptr )
{
	// Call super implementation
	QTensorVisWidget::setTensorDataProvider( ptr );

	// If not pointing to one of our SDM data provider it is obviously set
	// by the super class
	if( ptr != m_curTensorDataProvider )
	{
		m_superTensorDataProvider = ptr;
	}
}

void SDMTensorVisWidget::
  setupScene()
{
	// --- VTK scene setup ---
	// The tensor visualization actors are already added to the renderer.
	// We add some custom stuff here.

	// Halo visualization
	m_halo.setRenderer( getRenderer() );
	m_halo.setVisibility( false );

	// Marker visualization
	m_sphereMarker.setup( 0,0,0, 1.0 );
	m_sphereMarker.getActor()->GetProperty()->SetColor( 1,0,0 );
	m_sphereMarker.getActor()->GetProperty()->SetOpacity( .5 );
	m_sphereMarker.getActor()->GetProperty()->SetLighting( 0 );
	m_sphereMarker.getActor()->SetVisibility( 1 );
	getRenderer()->AddActor( m_sphereMarker.getActor() );

	getRenderer()->SetBackground( 1,1,1 );

	// Custom lighting

	// Light from right, down
	VTKPTR<vtkLight> light0 = VTKPTR<vtkLight>::New();
	light0->SetLightTypeToCameraLight();
	light0->SetPosition( 1,-1, 0 );
	light0->SetFocalPoint( 0,0,0 );
	light0->SetDiffuseColor( .7,.7,.7 );
	light0->SetAmbientColor( .2,.2,.2 );
	light0->SetSpecularColor( 1,1,1 );

	// Headlight
	VTKPTR<vtkLight> light1 = VTKPTR<vtkLight>::New();
	light1->SetLightTypeToHeadlight();
	light1->SetDiffuseColor( .7,.7,.7 );
	light1->SetAmbientColor( .2,.2,.2 );
	light1->SetSpecularColor( 1,1,1 );

	// Light from left, up, front
	VTKPTR<vtkLight> light2 = VTKPTR<vtkLight>::New();
	light2->SetLightTypeToCameraLight();
	light2->SetPosition( -1,1,1 );
	light2->SetFocalPoint( 0,0,0 );
	light2->SetDiffuseColor( .7,.7,.7 );
	light2->SetAmbientColor( .2,.2,.2 );
	light2->SetSpecularColor( 1,1,1 );

	getRenderer()->AddLight( light0 );
	getRenderer()->AddLight( light1 );
	getRenderer()->AddLight( light2 );
}

void SDMTensorVisWidget::
  setupInteraction()
{
	// Create interactor and styles
	m_interactor = VTKPTR<vtkRenderWindowInteractor>::New();
	m_interactorCamera     = VTKPTR<vtkInteractorStyleTrackballCamera>::New();
	m_interactorManipulate = VTKPTR<vtkInteractorStyleTrackballActor >::New();

	// Set camera style as default	
	m_interactor->SetInteractorStyle( m_interactorCamera );

	// Attach to window
	vtkRenderWindow* window = getRenderer()->GetRenderWindow();
	assert( window ); // We definitely need the render window!
	m_interactor->SetRenderWindow( window );

	// Make all actors not pickable despite the sphere marker
	vtkPropCollection* props = getRenderer()->GetViewProps();
	props->InitTraversal();
	while( vtkProp* prop = props->GetNextProp() )
	{
		prop->SetPickable( 0 );
		prop->SetDragable( 0 );
	}
	m_sphereMarker.getActor()->SetPickable( 1 );
	m_sphereMarker.getActor()->SetDragable( 1 );
}

void SDMTensorVisWidget::
  toggleDragMarker( bool picking )
{
	if( picking )
	{
		m_interactor->SetInteractorStyle( m_interactorManipulate );
		m_interactorUpdateTimer->start( 100 ); // 1/10th of a second
	}
	else
	{
		m_interactor->SetInteractorStyle( m_interactorCamera );
		m_interactorUpdateTimer->stop();
	}

	// WORKAROUND: As long as we do not have signals on dragging the marker
	//             we simply update the marker position label each time the
	//             interactor mode is changed.
	updateMarkerLabel();
}

void SDMTensorVisWidget::
  toggleMarker( bool show )
{
	m_sphereMarker.getActor()->SetVisibility( show );
}

void SDMTensorVisWidget::
  resetMarker()
{
	// FIXME: Hardcoded default marker position
	m_sphereMarker.getActor()->SetPosition( 102., 122., 69. );
}

void SDMTensorVisWidget::
  setMarkerToEdit()
{
	// Ray intersection is already given in normalized coordinates [0,1]
	float v[3];
	m_rayPickingInfo.getIntersection( v );

	// Convert normalized coordinates to physical ones
	double px=v[0], py=v[1], pz=v[2];
	m_sdm->getHeader().physicalizeCoordinates( px, py, pz );

	// Set marker to new position
	m_sphereMarker.getActor()->SetPosition( px, py, pz );

	updateMarkerLabel();
}

void SDMTensorVisWidget::
  onChangedMarkerRadius( double radius )
{
	double p[3];
	m_sphereMarker.getActor()->GetPosition( p );
	m_sphereMarker.setup( p[0],p[1],p[2], radius );
}

void SDMTensorVisWidget::
  setSDM( StatisticalDeformationModel* sdm )
{
	m_sdm = sdm;
	m_editLocalCovariance  .setSDM( sdm );
	m_linearLocalCovariance.setSDM( sdm );
}

void SDMTensorVisWidget::
  updateUI()
{
	m_spinGamma ->setValue( m_curTensorDataProvider->getGamma() );
	m_spinRadius->setValue( m_curTensorDataProvider->getSphericalSamplingRadius() );
}

void SDMTensorVisWidget::
  updateMarkerLabel()
{
	double p[3];
	m_sphereMarker.getActor()->GetPosition( p );

	static int last_ijk[3] = { 0,0,0 };
	int ijk[3];
	m_curTensorDataProvider->getImageDataSpace().getIJK( p, ijk );

	m_labelPosition->setText( tr("(%1,%2,%3)\n[%4,%5,%6]")
		.arg(p[0],5,'f',2).arg(p[1],5,'f',2).arg(p[2],5,'f',2)
		.arg(ijk[0]).arg(ijk[1]).arg(ijk[2]) );

	// Update halo rendering (slightly expensive!) 
	if( ijk[0]==last_ijk[0] && ijk[1]==last_ijk[1] && ijk[2]==last_ijk[2] )
	{
		// Still the same sample position, do nothing
	}
	else
	{
		// Changed sample position
		last_ijk[0] = ijk[0];
		last_ijk[1] = ijk[1];
		last_ijk[2] = ijk[2];

		// Update halo
		synchronizeHalo();
	}
}

void SDMTensorVisWidget::
  updateTensorVis()
{	
	static double last_ijk[3] = { -1,-1,-1 };	

	// Marker position in physical and normalized coordinates
	double p[3],  // p = physical coordinates
		   v[3];  // v = normalized coordinates
	m_sphereMarker.getActor()->GetPosition( p );
	m_sphereMarker.getActor()->GetPosition( v );
	m_sdm->getHeader().normalizeCoordinates( v[0], v[1], v[2] );

	// Sanity check
	int ijk[3];
	m_curTensorDataProvider->getImageDataSpace().getIJK( p, ijk );
#if 0
	if( last_ijk[0]==ijk[0] && last_ijk[1]==ijk[1] && last_ijk[2]==ijk[2] )
	{
		emit statusMessage(tr("Tensor update skipped: New position does not "
			"differ from old one (at vectorfield resolution)!"));
		return;
	}
#endif
	last_ijk[0] = ijk[0];
	last_ijk[1] = ijk[1];
	last_ijk[2] = ijk[2];

	emit statusMessage(tr("Updating tensor visualization..."));

	// Set gamma
	m_curTensorDataProvider->setGamma( m_spinGamma->value() );
	// Set probe radius
	m_curTensorDataProvider->setSphericalSamplingRadius( m_spinRadius->value() );
	// Invoke linear solve
	m_curTensorDataProvider->setReferencePoint( v[0],v[1],v[2] );

	// Invoke tensor visualization update
	QTensorVisWidget::updateTensorVis();

	// Compute tensor statistics
	emit statusMessage(tr("Updating tensor statistics..."));
	m_tdataStatistics.setTensorData( m_curTensorDataProvider );
	m_tdataStatistics.compute();

	m_labelStatistics->setText(
		tr("%1 (±%2)\n"
		   "%3 (±%4)\n"
		   "%5 (±%6)")
		   .arg( m_tdataStatistics.getMoment(0,0), 5,'f',4 )
		   .arg( m_tdataStatistics.getMoment(1,0), 5,'f',4 )
		   .arg( m_tdataStatistics.getMoment(0,1), 5,'f',4 )
		   .arg( m_tdataStatistics.getMoment(1,1), 5,'f',4 )
		   .arg( m_tdataStatistics.getMoment(0,2), 5,'f',4 )
		   .arg( m_tdataStatistics.getMoment(1,2), 5,'f',4 ) );

	// Show marker
#if 1
	// PROBE RADIUS IN PHYSICAL COORDINATES
	// VTK works in the correctly scaled physical coordinates, so we do not have
	// to convert the radius.
	m_sphereMarker.setup( p[0], p[1], p[2], 
		 m_spinRadius->value() );
		//was: m_curTensorDataProvider->getSphericalSamplingRadius() );
#else
	// PROBE RADIUS IN NORMALIZED COORDINATES
	double scale = m_sdm->getHeader().resolution[0] * m_sdm->getHeader().spacing[0];
	m_sphereMarker.setup( px, py, pz, 
		scale * m_editLocalCovariance.getSphericalSamplingRadius() );
#endif

	// Redraw
	forceRedraw();

	emit statusMessage(tr("Tensor visualization update finished."));
}

void SDMTensorVisWidget::
  setPickedRay( RayPickingInfo info )
{
	m_rayPickingInfo = info;
}

void SDMTensorVisWidget::
  toggleHalo( bool show )
{
	m_halo.setVisibility( show );
	// Enable only tensor glyphs
	m_halo.setVectorfieldVisibility( false );
	m_halo.setLegendVisibility( false );
	m_halo.setTensorVisibility( show );
}

void SDMTensorVisWidget::
  setHaloSize( double eigenvalueOfs )
{
	m_halo.getTensorGlyph()->SetEigenvalueOffsetValue( eigenvalueOfs );
	forceRedraw();
}

void SDMTensorVisWidget::
  synchronizeHalo()
{
	// Make sure the halo uses the exact same tensor data as the tensor vis.
	m_haloAdaptor.setTensorProvider( m_curTensorDataProvider );
	m_halo.setDataProvider( &m_haloAdaptor ); 

	// Get selected point
	double pt[3];  // Everything should work in physical coordinates
	m_sphereMarker.getActor()->GetPosition( pt );

	// Round position to grid
	int ijk[3];
	m_curTensorDataProvider->space().getIJK( pt, ijk );
	
	int step[3];
	m_curTensorDataProvider->getLastGridStep( step );
	ijk[0] = (ijk[0] / step[0])*step[0];
	ijk[1] = (ijk[1] / step[1])*step[1];
	ijk[2] = (ijk[2] / step[2])*step[2];

	m_curTensorDataProvider->space().getPoint( ijk, pt );

	// Set selected point as only sample point
	VTKPTR<vtkPoints> samplePoint = VTKPTR<vtkPoints>::New();
	samplePoint->SetNumberOfPoints( 1 );
	samplePoint->InsertPoint( 0, pt );
	m_haloAdaptor.setPoints( samplePoint, true ); // threshold = true
	//m_haloAdaptor.updateTensorData(); // Compute tensor at sample point
	// -> updatTensorData() implicitly called on call to updateGlyphs() below

	// Duplicate TensorVisBase settings for halo renderer
	m_halo.setColorGlyphs ( getTensorVis()->getColorGlyphs()  );
	m_halo.setColorMode   ( getTensorVis()->getColorMode()    );
	m_halo.setExtractEigenvalues( getTensorVis()->getExtractEigenvalues() );
	m_halo.setGlyphScaleFactor( getTensorVis()->getGlyphScaleFactor() );
	m_halo.setGlyphType( getTensorVis()->getGlyphType() );
	m_halo.setSuperquadricGamma( getTensorVis()->getSuperquadricGamma() );
	m_halo.setThreshold( getTensorVis()->getThreshold() );

	// Disable lighting
	m_halo.getTensorActor()->GetProperty()->SetAmbient ( 1. );
	m_halo.getTensorActor()->GetProperty()->SetDiffuse ( 0. );
	m_halo.getTensorActor()->GetProperty()->SetSpecular( 0. );
	m_halo.getTensorActor()->GetProperty()->SetLighting( 0 );
	// Halo color
	m_halo.getTensorActor()->GetProperty()->SetColor( 1.,0.,0. );
	m_halo.getTensorActor()->GetProperty()->SetAmbientColor( 1., 0., 0. );	
	m_halo.getTensorActor()->GetProperty()->SetInterpolationToFlat();
	// Draw backfaces
#if 1 // <-- Frontface culling is working only sporadically, so we turn it off!
	m_halo.getTensorActor()->GetProperty()->SetFrontfaceCulling( 1 );
	//m_halo.getTensorActor()->GetProperty()->SetBackfaceCulling( 0 );
#endif

	// Update visualization using the set sample point
	m_halo.updateGlyphs( TensorVisBase::ReUseExistingSampling );

	// Disable lookup table (which is explicitly enabled in updateGylphs()!)
	vtkMapper* mapper = m_halo.getTensorActor()->GetMapper();
	if( mapper )  // Can happen that mapper is not set yet
	{
		mapper->SetColorModeToDefault();		
		mapper->SetScalarVisibility( 0 );
	}
}

void SDMTensorVisWidget::
  setProbes( QList<SDMTensorProbe> probes )
{
	m_probeWidget->setProbes( probes );
}

QList<SDMTensorProbe> SDMTensorVisWidget::
  getProbes() const
{
	return m_probeWidget->getProbes();
}
