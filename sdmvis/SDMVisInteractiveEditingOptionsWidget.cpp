#include "SDMVisInteractiveEditingOptionsWidget.h"
#include <QGridLayout>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QCheckBox>

SDMVisInteractiveEditingOptionsWidget::
SDMVisInteractiveEditingOptionsWidget( QWidget* parent )
: QWidget(parent),
  m_master(NULL)
{
	QDoubleSpinBox* sbGamma = new QDoubleSpinBox();
	QLabel* laGamma = new QLabel(tr("Gamma"));
	laGamma->setBuddy( sbGamma );
	sbGamma->setRange( 0.0, 1e15 );
	sbGamma->setDecimals( 5 );

	QDoubleSpinBox* sbEdit = new QDoubleSpinBox();
	QLabel* laEdit = new QLabel(tr("Edit scale"));
	laEdit->setBuddy( sbEdit );
	sbEdit->setRange( 0.0, 1e15 );

	QDoubleSpinBox* sbResult = new QDoubleSpinBox();
	QLabel* laResult = new QLabel(tr("Result scale"));
	laResult->setBuddy( sbResult );
	sbResult->setRange( 0.0, 1e15 );
	
	QComboBox* cbMode = new QComboBox();
	cbMode->addItem( tr("Plain") );
	cbMode->addItem( tr("Normalized") );
	cbMode->addItem( tr("Sqrt") );
	cbMode->addItem( tr("Inverse") );
	QLabel* laMode = new QLabel(tr("Eigenmode scaling"));


	m_showDebugRubberband = new QCheckBox();
	m_showDebugRubberband->setChecked( false );
	QLabel* laShowDebugRubberband = new QLabel(tr("Show non-displaced rubberband"));
	laShowDebugRubberband->setBuddy( m_showDebugRubberband );

	m_showRubberband = new QCheckBox();
	m_showRubberband->setChecked( false );
	QLabel* laShowRubberband = new QLabel(tr("Show rubberband"));
	laShowRubberband->setBuddy( m_showRubberband );

	m_rubberFudgeFactor = new QDoubleSpinBox();
	QLabel* laRubberFudgeFactor = new QLabel(tr("Rubberband fudge factor"));
	laRubberFudgeFactor->setBuddy( m_rubberFudgeFactor );
	m_rubberFudgeFactor->setRange( 0.0, 1.0 );
	m_rubberFudgeFactor->setDecimals( 2 );

	// Some hardcoded rubberband defaults (those are not members of master but
	// are only queried from these controls from master)
	m_showRubberband     ->setChecked( true );
	m_showDebugRubberband->setChecked( false );
	m_rubberFudgeFactor  ->setValue  ( 0.13 );

	QLabel* laCoeffs = new QLabel();
	QLabel* lalaCoeffs = new QLabel(tr("Coefficients:"));

	QLabel* laError = new QLabel();
	QLabel* lalaError = new QLabel(tr("Error:"));

	QLabel* laDedit = new QLabel();
	QLabel* lalaDedit = new QLabel(tr("DEdit:"));

	QLabel* laPos0 = new QLabel();
	QLabel* lalaPos0 = new QLabel(tr("Pos0:"));

	QGridLayout* layout = new QGridLayout();
	unsigned row = 0;
	layout->addWidget( laGamma, row,0 );
	layout->addWidget( sbGamma, row,1 );
	row++;
	layout->addWidget( laEdit, row,0 );
	layout->addWidget( sbEdit, row,1 );
	row++;
	layout->addWidget( laResult, row,0 );
	layout->addWidget( sbResult, row,1 );
	row++;
	layout->addWidget( laMode, row,0 );
	layout->addWidget( cbMode, row,1 );
	row++;
	layout->addWidget( laShowRubberband, row,0 );
	layout->addWidget( m_showRubberband, row,1 );
	row++;
	layout->addWidget( laShowDebugRubberband, row,0 );
	layout->addWidget( m_showDebugRubberband, row,1 );
	row++;
	layout->addWidget( laRubberFudgeFactor, row,0 );
	layout->addWidget( m_rubberFudgeFactor, row,1 );
	row++;
	layout->addWidget( lalaError, row,0 );
	layout->addWidget( laError  , row,1 );
	row++;
	layout->addWidget( lalaDedit, row,0 );
	layout->addWidget( laDedit  , row,1 );
	row++;
	layout->addWidget( lalaCoeffs, row,0 );
	layout->addWidget( laCoeffs  , row,1 );
	row++;
	layout->addWidget( lalaPos0, row,0 );
	layout->addWidget( laPos0  , row,1 );
	row++;
	
	m_gamma      = sbGamma;
	m_editScale  = sbEdit;
	m_resultScale= sbResult;
	m_mode       = cbMode;

	m_coeffs = laCoeffs;
	m_error  = laError;
	m_dedit  = laDedit;
	m_pos0   = laPos0;

	setLayout( layout );

	connect( sbGamma , SIGNAL(valueChanged(double)), this, SLOT(onChangedGamma(double)) );
	connect( sbEdit  , SIGNAL(valueChanged(double)), this, SLOT(onChangedEditScale(double)) );
	connect( sbResult, SIGNAL(valueChanged(double)), this, SLOT(onChangedResultScale(double)) );
	connect( cbMode  , SIGNAL(currentIndexChanged(int)), this, SLOT(onChangedMode(int)) );
}

void SDMVisInteractiveEditingOptionsWidget::
connectMaster( SDMVisInteractiveEditing* master )
{	
	m_master = NULL; // prohibit direct passing of signals
	if( master )
	{
		// Connect
		m_gamma      ->setValue( master->getGamma() );
		m_editScale  ->setValue( master->getEditScale() );
		m_resultScale->setValue( master->getResultScale() );
		m_mode       ->setCurrentIndex( master->getModeScaling() );
	}
	else
	{
		// Disconnect		
	}
	m_master = master;
}

double SDMVisInteractiveEditingOptionsWidget::
getRubberFudgeFactor() const
{
	return m_rubberFudgeFactor->value();
}

bool SDMVisInteractiveEditingOptionsWidget::
getShowRubberband() const
{
	return m_showRubberband->isChecked();
}

bool SDMVisInteractiveEditingOptionsWidget::
getShowDebugRubberband() const
{
	return m_showDebugRubberband->isChecked();
}

void SDMVisInteractiveEditingOptionsWidget::
onChangedGamma( double value )
{
	if( m_master )
		m_master->setGamma( value );
}

void SDMVisInteractiveEditingOptionsWidget::
onChangedEditScale( double value )
{
	if( m_master )
		m_master->setEditScale( value );
}

void SDMVisInteractiveEditingOptionsWidget::
onChangedResultScale( double value )
{
	if( m_master )
		m_master->setResultScale( value );
}

void SDMVisInteractiveEditingOptionsWidget::
onChangedMode( int index )
{
	if( m_master )
		m_master->setModeScaling( index );
}

void SDMVisInteractiveEditingOptionsWidget::
	setCoeffs( double c0, double c1, double c2, double c3, double c4 )
{
	m_coeffs->setText( tr("%0, %1, %2, %3, %5").arg(c0,4).arg(c1,4).arg(c2,4).arg(c3,4).arg(c4,4) );
}

void SDMVisInteractiveEditingOptionsWidget::
	setError( double Etotal, double Esim, double Ereg )
{
	m_error->setText( tr("%0, %1, %2").arg(Etotal,4).arg(Esim,4).arg(Ereg,4) );
}

void SDMVisInteractiveEditingOptionsWidget::
	setDEdit( double x, double y, double z )
{
	m_dedit->setText( tr("%0, %1, %2").arg(x,4,'f').arg(y,4,'f').arg(z,4,'f') );
}

void SDMVisInteractiveEditingOptionsWidget::
	setPos0( double x, double y, double z )
{
	m_pos0->setText( tr("%0, %1, %2").arg(x,1,'f').arg(y,1,'f').arg(z,1,'f') );
}
