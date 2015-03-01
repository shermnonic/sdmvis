#include "QTensorVisOptionsWidget.h"
#include "TensorVisRenderBase.h"
#include "TensorVisBase.h"
#include "TensorVis.h"
#include "TensorVis2.h"
#include "TensorNormalDistributionProvider.h"
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QPushButton>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QSlider>

//------------------------------------------------------------------------------
//	C'tor()
//------------------------------------------------------------------------------
QTensorVisOptionsWidget::QTensorVisOptionsWidget( QWidget* parent )
: QWidget(parent),
  m_master(NULL)
{
	createWidgets();
}

//------------------------------------------------------------------------------
//	createWidgets()
//------------------------------------------------------------------------------
void QTensorVisOptionsWidget::createWidgets()
{
	int row;

	// --- Sampling options ----
	QPushButton*    butResample    = new QPushButton(tr("Resample"));
	QDoubleSpinBox* sbThreshold    = new QDoubleSpinBox;
	QSpinBox*       sbSampleSize   = new QSpinBox;
	QSpinBox*       sbGridStepSize = new QSpinBox;
	
	sbSampleSize->setRange(100,2000000);
	sbSampleSize->setSingleStep(1000);
	sbGridStepSize->setRange(1,10);
	
	QComboBox* comboStrategy = new QComboBox;
	comboStrategy->addItem(tr("Regular grid sampling"));
	comboStrategy->addItem(tr("Random sampling"));

	QGridLayout* resampleLayout = new QGridLayout; row=0;
	resampleLayout->addWidget( new QLabel(tr("Sampling strategy")), row,0 );
	resampleLayout->addWidget( comboStrategy, row++,1 );
	resampleLayout->addWidget( new QLabel(tr("Threshold")), row,0 );
	resampleLayout->addWidget( sbThreshold, row++,1 );
	resampleLayout->addWidget( new QLabel(tr("Number of random samples")), row,0 );
	resampleLayout->addWidget( sbSampleSize, row++,1 );
	resampleLayout->addWidget( new QLabel(tr("Sampling grid stepsize")), row,0 );
	resampleLayout->addWidget( sbGridStepSize, row++,1 );
	resampleLayout->addWidget( butResample, row,0, 1,2 );

	// --- Slicing options ---
	QCheckBox* slicingEnabled = new QCheckBox();

	QRadioButton* sliceX = new QRadioButton( tr("X") );
	QRadioButton* sliceY = new QRadioButton( tr("Y") );
	QRadioButton* sliceZ = new QRadioButton( tr("Z") );
	QButtonGroup* sliceDir = new QButtonGroup();
	sliceDir->addButton( sliceX, 0 );
	sliceDir->addButton( sliceY, 1 );
	sliceDir->addButton( sliceZ, 2 );
	sliceDir->setExclusive( true );

	QSlider* sliceSlider = new QSlider( Qt::Horizontal );
	sliceSlider->setRange( 0, 100 );
	QLabel* sliceLabel = new QLabel(tr("-"));

	QHBoxLayout* sliceLayout = new QHBoxLayout;
	sliceLayout->addWidget( slicingEnabled );
	sliceLayout->addWidget( sliceX );
	sliceLayout->addWidget( sliceY );
	sliceLayout->addWidget( sliceZ );
	sliceLayout->addWidget( sliceSlider );
	sliceLayout->addWidget( sliceLabel );


	// --- Visbility options ---
	QCheckBox* cbShowVectorfield  = new QCheckBox;
	QCheckBox* cbShowTensorGlyphs = new QCheckBox;
	QCheckBox* cbShowLegend       = new QCheckBox;

	QGridLayout* showLayout = new QGridLayout; row=0;
	showLayout->addWidget( new QLabel(tr("Show vectorfield")), row,0 );
	showLayout->addWidget( cbShowVectorfield, row++,1 );
	showLayout->addWidget( new QLabel(tr("Show tensor glyphs")), row,0 );
	showLayout->addWidget( cbShowTensorGlyphs, row++,1 );
	showLayout->addWidget( new QLabel(tr("Show color legend")), row,0 );
	showLayout->addWidget( cbShowLegend, row++,1 );


	// --- Tensor Visualization options ---
	QCheckBox*      cbExtractEigenvalues = new QCheckBox;
	QCheckBox*      cbSqrtScaling = new QCheckBox;
	QCheckBox*      cbColorGlyphs = new QCheckBox;
	QDoubleSpinBox* sbGlyphScaling = new QDoubleSpinBox;
	sbGlyphScaling->setDecimals( 1 );
	sbGlyphScaling->setSingleStep( 1.0 );
	sbGlyphScaling->setMinimum( 0.1 );
	sbGlyphScaling->setMaximum( 1000 );

	QDoubleSpinBox* sbSuperquadricGamma = new QDoubleSpinBox;
	sbSuperquadricGamma->setDecimals( 1 );
	sbSuperquadricGamma->setSingleStep( 0.1 );
	sbSuperquadricGamma->setRange( 0.0, 10.0 );

	QComboBox* comboGlyphType = new QComboBox;
	comboGlyphType->addItem(tr("Sphere"));
	comboGlyphType->addItem(tr("Cylinder"));
	comboGlyphType->addItem(tr("Cube"));
	comboGlyphType->addItem(tr("Superquadric"));

	QComboBox* comboColorBy = new QComboBox;
	comboColorBy->addItem(tr("Scalar"));
	comboColorBy->addItem(tr("Maximum Eigenvalue"));
	comboColorBy->addItem(tr("Fractional anisotropy"));

	QGridLayout* visLayout = new QGridLayout; row=0;
	visLayout->addWidget( new QLabel(tr("Glyph type")), row,0 );
	visLayout->addWidget( comboGlyphType, row++,1 );
	visLayout->addWidget( new QLabel(tr("Glyph scale factor")), row,0 );
	visLayout->addWidget( sbGlyphScaling, row++,1 );
	visLayout->addWidget( new QLabel(tr("Superquadric gamma")), row, 0 );
	visLayout->addWidget( sbSuperquadricGamma, row++,1 );
	visLayout->addWidget( new QLabel(tr("Extract eigenvalues")), row,0 );
	visLayout->addWidget( cbExtractEigenvalues, row++,1 );
	visLayout->addWidget( new QLabel(tr("Sqrt scaling")), row,0 );
	visLayout->addWidget( cbSqrtScaling, row++,1 );
	visLayout->addWidget( new QLabel(tr("Color glyphs")), row,0 );
	visLayout->addWidget( cbColorGlyphs, row++,1 );
	visLayout->addWidget( new QLabel(tr("Color by")), row,0 );
	visLayout->addWidget( comboColorBy, row++,1 );


	// --- Tensor Distribution options ---
	QPushButton* butComputeSpectrum = new QPushButton(tr("Compute spectrum"));
	
	QSpinBox* sbSpectMode = new QSpinBox;
	sbSpectMode->setRange(0,5);
	sbSpectMode->setValue(0);

	QDoubleSpinBox* sbSpectModeScale = new QDoubleSpinBox;
	sbSpectModeScale->setRange(-10.0,+10.0);
	sbSpectModeScale->setValue( 1.0 );
	sbSpectModeScale->setDecimals( 4 );

	QCheckBox* cbSpectModeApply = new QCheckBox;
	cbSpectModeApply->setChecked( false );

	QHBoxLayout* spectModeLayout = new QHBoxLayout;
	spectModeLayout->addWidget( cbSpectModeApply );
	spectModeLayout->addWidget( new QLabel(tr("Mode")) );
	spectModeLayout->addWidget( sbSpectMode );
	spectModeLayout->addWidget( new QLabel(tr("Scale")) );
	spectModeLayout->addWidget( sbSpectModeScale );

	QVBoxLayout* distLayout = new QVBoxLayout;
	distLayout->addWidget( butComputeSpectrum );
	distLayout->addLayout( spectModeLayout );


	// --- Assemble options widget ---
	QGroupBox* showGroup = new QGroupBox(tr("Visibility"));
	showGroup->setLayout( showLayout );

	QGroupBox* resampleGroup = new QGroupBox(tr("Sampling"));
	resampleGroup->setLayout( resampleLayout );

	QGroupBox* sliceGroup = new QGroupBox(tr("Slicing"));
	sliceGroup->setLayout( sliceLayout );

	QGroupBox* visGroup = new QGroupBox(tr("Tensor glyph"));
	visGroup->setLayout( visLayout );

	QGroupBox* distGroup =  new QGroupBox(tr("Tensor distribution"));
	distGroup->setLayout( distLayout );

	//QGroupBox* vectorGroup = new QGroupBox(tr("Vectorfield"));
	//vectorGroup->setLayout( vectorLayout );

	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget( showGroup );
	layout->addWidget( resampleGroup );
	layout->addWidget( sliceGroup );
	layout->addWidget( visGroup );
	layout->addWidget( distGroup );
	layout->addStretch( 5 );
	layout->setContentsMargins(0,0,0,0); // for use as sub-widget
	this->setLayout( layout );
	
	// Assign global control widgets
	m_comboStrategy      = comboStrategy;
	m_sbThreshold        = sbThreshold;
	m_sbSampleSize       = sbSampleSize;
	m_sbGridStepSize     = sbGridStepSize;
	m_sbGlyphScaleFactor = sbGlyphScaling;
	m_cbExtractEigenvalues = cbExtractEigenvalues;
	m_cbSqrtScaling      = cbSqrtScaling;
	m_cbColorGlyphs      = cbColorGlyphs;
	m_comboGlyphType     = comboGlyphType;
	m_cbShowVectorfield  = cbShowVectorfield;
	m_cbShowTensorGlyphs = cbShowTensorGlyphs;
	m_cbShowLegend       = cbShowLegend;
	m_comboColorBy       = comboColorBy;
	m_sbSuperquadricGamma = sbSuperquadricGamma;
	m_slicingEnabled     = slicingEnabled;
	m_sliceDir           = sliceDir;
	m_sliceSlider        = sliceSlider;
	m_sliceGroup         = sliceGroup;
	m_distGroup          = distGroup;
	m_sbSpectMode        = sbSpectMode;
	m_sbSpectModeScale   = sbSpectModeScale;
	m_cbSpectModeApply   = cbSpectModeApply;
	
	// Connections
	connect( butResample, SIGNAL(clicked()), this, SLOT(resample()) );
	connect( m_sbGlyphScaleFactor,   SIGNAL(valueChanged(double)), this, SLOT(updateVis()) );
	connect( m_sbSuperquadricGamma,  SIGNAL(valueChanged(double)), this, SLOT(updateVis()) );
	connect( m_cbExtractEigenvalues, SIGNAL(stateChanged(int)),    this, SLOT(updateVis()) );
	connect( m_cbSqrtScaling,        SIGNAL(stateChanged(int)),    this, SLOT(updateVis()) );
	connect( m_cbColorGlyphs,        SIGNAL(stateChanged(int)),    this, SLOT(updateVis()) );
	connect( m_comboGlyphType,       SIGNAL(currentIndexChanged(int)),this,SLOT(updateVis()) );
	connect( m_cbShowVectorfield,    SIGNAL(stateChanged(int)),    this, SLOT(updateVis()) );
	connect( m_cbShowTensorGlyphs,   SIGNAL(stateChanged(int)),    this, SLOT(updateVis()) );
	connect( m_cbShowLegend,         SIGNAL(stateChanged(int)),    this, SLOT(updateVis()) );
	connect( m_comboColorBy,         SIGNAL(currentIndexChanged(int)),this,SLOT(updateVis()) );

	// Slicing connection are established in setTensorVis()
	//connect( slicingEnabled, SIGNAL(stateChanged(int)), this, SLOT(setSlicing(int)) );
	//connect( sliceDir, SIGNAL(buttonClicked(int)), this, SLOT(setSliceDir(int)) );
	//connect( sliceSlider, SIGNAL(valueChanged(int)), this, SLOT(setSlice(int)) );

	// ... with exception of the slider value label 
	connect( sliceSlider, SIGNAL(valueChanged(int)), sliceLabel, SLOT(setNum(int)) );

	// Tensor distribution connections
	connect( butComputeSpectrum, SIGNAL(clicked()), this, SLOT(computeSpectrum()) );
	connect( cbSpectModeApply, SIGNAL(stateChanged(int)   ), this, SLOT(updateSpectrumModes()) );
	connect( sbSpectMode,      SIGNAL(valueChanged(int)   ), this, SLOT(updateSpectrumModes()) );
	connect( sbSpectModeScale, SIGNAL(valueChanged(double)), this, SLOT(updateSpectrumModes()) );
}

//------------------------------------------------------------------------------
//	setTensorVis()
//------------------------------------------------------------------------------
void QTensorVisOptionsWidget::setTensorVis( TensorVisRenderBase* master )
{
	if( !master ) 
	{
		// If called with NULL disable widget
		this->setEnabled( false );
		return;
	}
	
	// WORKAROUND to avoid execution of ::updateVis() on changing UI values.
	// Usually blockSignals() or disconnect() should do the trick, but somehow
	// the SLOT(updateVis()) called in either approach. Therefore we set m_master
	// temporarily to NULL which is checked in ::updateVis() and prohibits
	// its execution.
	m_master = NULL;

	m_sbGlyphScaleFactor  ->setValue  ( master->getGlyphScaleFactor() );
	m_sbSuperquadricGamma ->setValue  ( master->getSuperquadricGamma() );
	m_cbExtractEigenvalues->setChecked( master->getExtractEigenvalues() );
	m_cbSqrtScaling       ->setChecked( master->getTensorGlyph()->GetEigenvalueSqrtScaling() );
	m_cbColorGlyphs       ->setChecked( master->getColorGlyphs() );
	m_comboGlyphType      ->setCurrentIndex( master->getGlyphType() );
	m_cbShowVectorfield   ->setChecked( master->getVectorfieldVisibility() );
	m_cbShowTensorGlyphs  ->setChecked( master->getTensorVisibility() );
	m_cbShowLegend        ->setChecked( master->getLegendVisibility() );
	m_comboColorBy        ->setCurrentIndex( master->getColorMode() );

	TensorVisBase* master2 = dynamic_cast<TensorVisBase*>(master);
	if( master2 )
	{
		m_sbThreshold         ->setValue  ( master2->getThreshold() );
		m_sbSampleSize        ->setValue  ( master2->getSampleSize() );
		m_sbGridStepSize      ->setValue  ( master2->getGridStepSize() );
		m_comboStrategy       ->setCurrentIndex( master2->getSamplingStrategy() );
	}
	bool enableSamplingControls = master2 != NULL;
	{
		m_sbThreshold   ->setEnabled( enableSamplingControls );
		m_sbSampleSize  ->setEnabled( enableSamplingControls );
		m_sbGridStepSize->setEnabled( enableSamplingControls );
		m_comboStrategy ->setEnabled( enableSamplingControls );
	}

	// Slicing UI
	disconnect( m_slicingEnabled );
	disconnect( m_sliceDir );
	disconnect( m_sliceSlider );
	TensorVis2* tvis2 = dynamic_cast<TensorVis2*>( master2 );
	if( tvis2 )
	{
		m_sliceGroup->setVisible( true );

		TensorDataProvider* tdp = tvis2->getDataProvider();

		if( tdp )
		{
			m_slicingEnabled->setChecked( tdp->getSlicing() );
			m_sliceDir->button( tdp->getSliceDirection() )->setChecked( true );
			m_sliceSlider->setRange( 0, (tdp->getNumSlices()) );
			m_sliceSlider->setValue( tdp->getSlice() );

			connect( m_slicingEnabled, SIGNAL(stateChanged (int)), this, SLOT(setSlicing (int)) );
			connect( m_sliceDir,       SIGNAL(buttonClicked(int)), this, SLOT(setSliceDir(int)) );
			connect( m_sliceSlider,    SIGNAL(valueChanged (int)), this, SLOT(setSlice   (int)) );

			m_sliceGroup->setEnabled( true );
		}
		else
			m_sliceGroup->setEnabled( false );
	}
	else
	{
		m_sliceGroup->setVisible( false );
		m_sliceGroup->setEnabled( false );
	}

	// Distribution UI
	if( tvis2 )
	{
		TensorNormalDistributionProvider* tdist = 
			dynamic_cast<TensorNormalDistributionProvider*>( tvis2->getDataProvider() );

		if( tdist )
		{
			m_distGroup->setEnabled( true );
			m_distGroup->setVisible( true );
		}
		else
		{
			m_distGroup->setEnabled( false );
			m_distGroup->setVisible( false );
		}
	}


	// Set members
	m_master = master;
	this->setEnabled( true );
}

//------------------------------------------------------------------------------
//	resample()
//------------------------------------------------------------------------------
void QTensorVisOptionsWidget::resample()
{
	if( !getVisBase() ) return;

	getVisBase()->setThreshold   ( m_sbThreshold   ->value() );
	getVisBase()->setSampleSize  ( m_sbSampleSize  ->value() );
	getVisBase()->setGridStepSize( m_sbGridStepSize->value() );
	getVisBase()->setSamplingStrategy( m_comboStrategy->currentIndex() );

	emit statusMessage(tr("Resampling tensor visualization..."));
	getVisBase()->updateGlyphs();
	emit visChanged();
	emit statusMessage(tr(""));
}

//------------------------------------------------------------------------------
//	updateVis()
//------------------------------------------------------------------------------
void QTensorVisOptionsWidget::updateVis()
{
	if( !m_master ) return;
	m_master->setGlyphScaleFactor     ( m_sbGlyphScaleFactor  ->value() );
	m_master->setSuperquadricGamma    ( m_sbSuperquadricGamma ->value() );
	m_master->setExtractEigenvalues   ( m_cbExtractEigenvalues->isChecked() );
	m_master->getTensorGlyph()->SetEigenvalueSqrtScaling( m_cbSqrtScaling->isChecked() );
	m_master->setColorGlyphs          ( m_cbColorGlyphs       ->isChecked() );
	m_master->setGlyphType            ( m_comboGlyphType->currentIndex() );
	m_master->setVectorfieldVisibility( m_cbShowVectorfield   ->isChecked() );
	m_master->setTensorVisibility     ( m_cbShowTensorGlyphs  ->isChecked() );
	m_master->setLegendVisibility     ( m_cbShowLegend        ->isChecked() );
	m_master->setColorMode            ( m_comboColorBy  ->currentIndex() );

	if( getVisBase() )
	getVisBase()->setSamplingStrategy ( m_comboStrategy ->currentIndex() );
	
	emit visChanged();
}

//------------------------------------------------------------------------------
//	Slicing
//------------------------------------------------------------------------------
void QTensorVisOptionsWidget::
  setSlicing( int enable )
{
	TensorVis2* tvis2 = dynamic_cast<TensorVis2*>( m_master );
	if( !tvis2 )
		return;

	tvis2->getDataProvider()->setSlicing( enable );
}

//------------------------------------------------------------------------------
void QTensorVisOptionsWidget::
  setSliceDir( int dir )
{
	TensorVis2* tvis2 = dynamic_cast<TensorVis2*>( m_master );
	if( !tvis2 )
		return;

	tvis2->getDataProvider()->setSliceDirection( dir );
}

//------------------------------------------------------------------------------
void QTensorVisOptionsWidget::
  setSlice( int slice )
{
	TensorVis2* tvis2 = dynamic_cast<TensorVis2*>( m_master );
	if( !tvis2 )
		return;

	tvis2->getDataProvider()->setSlice( slice );
}

//------------------------------------------------------------------------------
//	Spectrum
//------------------------------------------------------------------------------
void QTensorVisOptionsWidget::
  setModeAnimation( int mode, double val )
{
	m_cbSpectModeApply->setChecked( true );
	m_sbSpectMode     ->setValue( mode );
	m_sbSpectModeScale->setValue( val );

	resample();
}

void QTensorVisOptionsWidget::
  computeSpectrum()
{
	TensorVis2* tvis2 = dynamic_cast<TensorVis2*>( m_master );
	if( !tvis2 )
		return;

	TensorNormalDistributionProvider* tdist = 
		dynamic_cast<TensorNormalDistributionProvider*>( tvis2->getDataProvider() );
	if( !tdist )
		return;

	tdist->computeModes();	
}

void QTensorVisOptionsWidget::
  updateSpectrumModes()
{
	TensorVis2* tvis2 = dynamic_cast<TensorVis2*>( m_master );
	if( !tvis2 )
		return;

	TensorNormalDistributionProvider* tdist = 
		dynamic_cast<TensorNormalDistributionProvider*>( tvis2->getDataProvider() );
	if( !tdist )
		return;

	float coeffs[6] = { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f };
	if( m_cbSpectModeApply->isChecked() )
		for( int i=0; i < 6; i++ )
			coeffs[i] = (i == m_sbSpectMode->value()) ? (float)m_sbSpectModeScale->value() : 0.f;
	tdist->setCoeffs( coeffs );
}


//------------------------------------------------------------------------------
TensorVisBase*       QTensorVisOptionsWidget::
	getVisBase() 
{ 
	return dynamic_cast<TensorVisBase*>(m_master); 
}

TensorVisRenderBase* QTensorVisOptionsWidget::
	getVisRenderBase() 
{ 
	return dynamic_cast<TensorVisRenderBase*>(m_master);
}
