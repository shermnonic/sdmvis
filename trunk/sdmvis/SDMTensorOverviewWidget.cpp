#include "SDMTensorOverviewWidget.h"
#include <QTime>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QCheckBox>

#include <QVTKWidget.h>
#include <vtkRenderWindow.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkActor.h>
#include <vtkProperty.h>

SDMTensorOverviewWidget
  ::SDMTensorOverviewWidget( QWidget* parent )
  : QWidget( parent )
{
	m_renderer = VTKPTR<vtkRenderer>::New();
	m_renderer->SetBackground( 1., 1., 1. );
	
	m_volvis.setRenderer( m_renderer );
	m_volvis.setContourVisibility( false );
	m_volvis.setOutlineVisibility( false );
	m_volvis.setVolumeVisibility ( true );
	m_volvis.setVisibility( true );

	m_volvis.getVolumeActor()->GetProperty()->SetScalarOpacityUnitDistance( 15.0 );
	
	m_volvisReference.setContourValue( 5.0 );
	m_volvisReference.getContourActor()->GetProperty()->SetOpacity( .38 );
	//m_volvisReference.getContourActor()->GetProperty()->SetBackfaceCulling( 1 );

	m_volvisReference.setRenderer( m_renderer );
	m_volvisReference.setContourVisibility( true );
	m_volvisReference.setOutlineVisibility( false );
	m_volvisReference.setVolumeVisibility ( false );
	m_volvisReference.setVisibility( true );
	
	m_vtkWidget = new QVTKWidget();	
	m_vtkWidget->GetRenderWindow()->AddRenderer( m_renderer );
	m_vtkWidget->GetRenderWindow()->SetPointSmoothing  ( 1 );
	m_vtkWidget->GetRenderWindow()->SetLineSmoothing   ( 1 );
	m_vtkWidget->GetRenderWindow()->SetPolygonSmoothing( 0 );
	m_vtkWidget->GetRenderWindow()->SetMultiSamples    ( 4 );

	m_controlWidget = createControlWidget();

	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget( m_vtkWidget );
	layout->setContentsMargins( 0,0,0,0 );
	setLayout( layout );
}

SDMTensorOverviewWidget
  ::~SDMTensorOverviewWidget()
{}

QWidget* SDMTensorOverviewWidget
  ::createControlWidget()
{
	const TensorDataStatistics& ts = m_covarStat.getTensorDataStatistics();
	
	QWidget* w = new QWidget();
	
	// Widgets
	QComboBox* comboMeasure = new QComboBox();	
	for( unsigned i=0; i < ts.getNumMomentMeasures(); i++ )
		comboMeasure->addItem(QString::fromStdString( ts.getMomentMeasureName( i ) ));

	QComboBox* comboIntegrator = new QComboBox();
	for( unsigned i=0; i < m_covarStat.getNumIntegrators(); i++ )
		comboIntegrator->addItem(QString::fromStdString( m_covarStat.getIntegratorName( i ) ));
	
	QPushButton* butCompute = new QPushButton(tr("Compute overview vis."));
	QPushButton* butLoad    = new QPushButton(tr("Load overview vis."));

	QSpinBox* spinStep = new QSpinBox;
	spinStep->setMinimum( 1 );
	spinStep->setMaximum( 12 );
	spinStep->setValue( 2 );

	QDoubleSpinBox* spinOpacityScale = new QDoubleSpinBox;
	spinOpacityScale->setRange( 1.0, 100.0 );
	spinOpacityScale->setSingleStep( 5.0 );
	spinOpacityScale->setValue( m_volvis.getVolumeActor()->GetProperty()->GetScalarOpacityUnitDistance() );

	QCheckBox* chkShowContour = new QCheckBox(tr("Show reference contour"));

	QDoubleSpinBox* spinContourOpacity = new QDoubleSpinBox;
	spinContourOpacity->setRange( 0.0, 1.0 );
	spinContourOpacity->setSingleStep( 0.1 );
	spinContourOpacity->setValue( m_volvisReference.getContourActor()->GetProperty()->GetOpacity() );

	QDoubleSpinBox* spinContourIso = new QDoubleSpinBox;
	spinContourIso->setRange( -1000, 10000 );
	spinContourIso->setValue( m_volvisReference.getContourValue() );

	m_labelRange = new QLabel(tr("[-,-]")); 

	m_chkTFInvert    = new QCheckBox(tr("TF invert"));
	m_chkTFZeroBased = new QCheckBox(tr("TF zero based"));

	QPushButton* butUpdateVis = new QPushButton(tr("Update vis."));

	// Member widgets (some have to be accessed from outside this function)
	m_spinStep = spinStep;
	m_comboMeasure = comboMeasure;
	
	// UI groups
	int row = 0; // grid layout count their rows
	
	// "Moment measure"
	QGridLayout* measureLayout = new QGridLayout; 
	measureLayout->addWidget( comboMeasure, 0,0 );
	
	QGroupBox* measureGroup = new QGroupBox(tr("Moment measure"));
	measureGroup->setLayout( measureLayout );
	
	// "Moment calculation"
	QGridLayout* computeLayout = new QGridLayout; row = 0;
	computeLayout->addWidget( new QLabel(tr("Integrator")), row,0 );
	computeLayout->addWidget( comboIntegrator,              row,1 ); row++;
	computeLayout->addWidget( new QLabel(tr("Stepsize")), row,0 );
	computeLayout->addWidget( spinStep,                   row,1 ); row++;
	computeLayout->addWidget( butCompute, row,0, 1,2 ); row++;
	computeLayout->addWidget( butLoad   , row,0, 1,2 ); row++;

	QGroupBox* computeGroup = new QGroupBox(tr("Moment calculation"));
	computeGroup->setLayout( computeLayout );

	// "Vis"
	QGridLayout* visLayout = new QGridLayout; row = 0;
	visLayout->addWidget( new QLabel(tr("Opacity scale")),row,0 );
	visLayout->addWidget( spinOpacityScale,               row,1 );      row++;
	visLayout->addWidget( chkShowContour,                 row,0, 1,2 ); row++;
	visLayout->addWidget( new QLabel(tr("Contour isovalue")), row,0 );
	visLayout->addWidget( spinContourIso,                     row,1 );  row++;
	visLayout->addWidget( new QLabel(tr("Contour opacity" )), row,0 );
	visLayout->addWidget( spinContourOpacity,                 row,1 );  row++;
	visLayout->addWidget( new QLabel(tr("Scalar range")), row,0 );
	visLayout->addWidget( m_labelRange,                   row,1 );      row++;
	visLayout->addWidget( m_chkTFInvert,                  row,0, 1,2 ); row++;
	visLayout->addWidget( m_chkTFZeroBased,               row,0, 1,2 ); row++;
	visLayout->addWidget( butUpdateVis,                   row,0, 1,2 ); row++;	
	
	QGroupBox* visGroup = new QGroupBox(tr("Visualization"));
	visGroup->setLayout( visLayout );
	
	// Main layout
	
	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget( measureGroup );
	layout->addWidget( computeGroup );
	layout->addWidget( visGroup );
	layout->addStretch( 5 );
	layout->setContentsMargins(0,0,0,0); // for use as sub-widget
	
	w->setLayout( layout );	
	
	// Connections	
	connect( butCompute,   SIGNAL(clicked()), 
	         this,         SLOT(compute()) );
	connect( butLoad,      SIGNAL(clicked()),
			 this,         SLOT(load()) );
	connect( comboMeasure, SIGNAL(currentIndexChanged(int)), 
	         this,         SLOT(selectMeasure(int)) );
	connect( comboIntegrator, SIGNAL(currentIndexChanged(int)), 
	         this,         SLOT(selectIntegrator(int)) );
	connect( spinOpacityScale, SIGNAL(valueChanged(double)),
		     this,         SLOT(setOpacityScale(double)) );
	connect( chkShowContour, SIGNAL(toggled(bool)),
		     this,         SLOT(setContourVisible(bool)) );
	connect( spinContourIso, SIGNAL(valueChanged(double)),
		     this,         SLOT(setContourIsovalue(double)) );
	connect( spinContourOpacity, SIGNAL(valueChanged(double)),
		     this,         SLOT(setContourOpacity(double)) );
	connect( butUpdateVis, SIGNAL(clicked()),
		     this,         SLOT(updateTransferFunction()) );

	return w;
}
	
void SDMTensorOverviewWidget
  ::setTensorData( SDMTensorDataProvider* tdata )
{
	m_covarStat.setTensorData( tdata );
}

void SDMTensorOverviewWidget
  ::setTensorVis( TensorVisBase* tvis )
{
	m_covarStat.setTensorVis( tvis );
}

void SDMTensorOverviewWidget
  ::compute()
{
	// Always use currently set stepsize
	int step = m_spinStep->value();

	// Sanity check on user
	if( QMessageBox::warning(this,tr("SDMVis"),
		  tr("For small step size this will take a long time and can not be aborted!\n"
		     "Are you sure you want to start the computation for <b>stepsize %1</b> ?").arg(step),
		  QMessageBox::Ok | QMessageBox::Cancel) 
		!= QMessageBox::Ok )
		return;

	// Get output path
	QString basepath = QFileDialog::getExistingDirectory(this,
		tr("SDMVis - Overview vis. output path") );
	if( basepath.isEmpty() )
		return;

	emit statusMessage(tr("Setup local covariance statistics..."));		

	m_covarStat.setSamplingResolution( step, step, step );

	// Measure time
	QTime time;
	time.start();
	emit statusMessage(tr("Computing overview visualization..."));

	// Heavy computation...
	m_covarStat.compute();
	
	emit statusMessage(tr("Computing overview done, took %1 ms").arg(time.elapsed()));

	std::cout << "Saving overview results in " << basepath.toStdString() << "...\n";
	m_covarStat.save( (basepath.toStdString() + std::string("\\")).c_str() );
}

bool SDMTensorOverviewWidget
  ::load( const char* basepath )
{
	if( !m_covarStat.load( basepath ) )
		return false;	
	return true;
}

bool SDMTensorOverviewWidget
  ::load()
{
	QString basepath = QFileDialog::getExistingDirectory(this,tr("SDMVis") );
	if( basepath.isEmpty() )
		return true;
	
	bool ok = load( (basepath + "\\").toAscii() );

	if( ok )
	{
		// Update visualization
		selectMeasure( m_comboMeasure->currentIndex() );

		emit statusMessage(tr("Successfully loaded overview visualizations from %1").arg(basepath));
	}
	else
		emit statusMessage(tr("Error: Could not load overview visualizations from %1!").arg(basepath));

	return ok;
}

void SDMTensorOverviewWidget
  ::redraw()
{
	// Force redraw
	m_renderer->GetRenderWindow()->Render();
	update();
}

void SDMTensorOverviewWidget
  ::setContourIsovalue( double iso )
{
	m_volvisReference.setContourValue( iso );
	redraw();
}

void SDMTensorOverviewWidget
  ::setContourOpacity( double alpha )
{
	m_volvisReference.getContourActor()->GetProperty()->SetOpacity( alpha );
	redraw();
}

void SDMTensorOverviewWidget
  ::setContourVisible( bool b )
{
	m_volvisReference.setContourVisibility( b );
	redraw();
}

void SDMTensorOverviewWidget
  ::setOpacityScale( double scale )
{
	m_volvis.getVolumeActor()->GetProperty()->SetScalarOpacityUnitDistance( scale );
	redraw();
}

void SDMTensorOverviewWidget
  ::setReference( VTKPTR<vtkImageData> img )
{
	m_volvisReference.setup( img );

	// Adjust isosurface mesh (for context)
	m_volvisReference.setContourValue( 5.0 );

	redraw();
}

void SDMTensorOverviewWidget
  ::updateTransferFunction()
{
	int  idx    = m_comboMeasure->currentIndex();
	bool invert = m_chkTFInvert->checkState()==Qt::Checked;
	bool zerobased = m_chkTFZeroBased->checkState()==Qt::Checked;

	// Adjust tranfer function
	vtkColorTransferFunction* tf = 
		m_volvis.getVolumeActor()->GetProperty()->GetRGBTransferFunction();

	// Get *real* scalar range of moment statistics 
	// (omitting zero values where no moments are integrated).
	double range[2];
	m_covarStat.getScalarRange( idx, range );
	
	// Update UI
	m_labelRange->setText( tr("[%1,%2]").arg(range[0]).arg(range[1]) );
	
	double scalarMin = range[0];
	double scalarEps = 1e-8;
	if( zerobased ) range[0] = 0.0;
	if( invert ) std::swap( range[0], range[1] );

	// Cool-to-warm shading
	tf->RemoveAllPoints();
	tf->AddRGBPoint( range[0], 59./255., 76./255., 192./255. );
	tf->AddRGBPoint( range[0]+(range[1]-range[0])/2., 1,1,1 );
	tf->AddRGBPoint( range[1], 255./255., 85./255., 0./255. );
	tf->SetColorSpaceToDiverging();

	// Linear alpha ramp
	vtkPiecewiseFunction* opacity =
		m_volvis.getVolumeActor()->GetProperty()->GetScalarOpacity();
	opacity->RemoveAllPoints();
	opacity->AddPoint( 0.0, 0.0 );
	opacity->AddPoint( scalarMin - scalarEps, 0.0 );
	opacity->AddPoint( range[0], 0.0 );
	opacity->AddPoint( range[1], 1.0 );

	// Force update and redraw
	m_volvis.getVolumeActor()->Update();
	redraw();
}

void SDMTensorOverviewWidget
  ::selectMeasure( int idx )
{
	ImageCollection images = m_covarStat.getMomentImages();	
	m_volvis.setup( images.at(idx) );

	updateTransferFunction();

	// Force redraw
	redraw();
}

void SDMTensorOverviewWidget
  ::selectIntegrator( int i )
{
	m_covarStat.setIntegrator( i );
}
