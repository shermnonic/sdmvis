#include "ModeAnimationWidget.h"

#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>

ModeAnimationWidget
  ::ModeAnimationWidget( QWidget* parent )
  : QWidget( parent )
{
	createUI();
}

void ModeAnimationWidget
  ::createUI()
{
	// -- Widgets

	sbMode = new QSpinBox;
	sbMode->setRange(0,5);
	sbMode->setValue(0);

	sbRangeMin = new QDoubleSpinBox;
	sbRangeMin->setRange(-10.0,0.0);
	sbRangeMin->setValue( -1.0 );
	sbRangeMin->setDecimals( 4 );

	sbRangeMax = new QDoubleSpinBox;
	sbRangeMax->setRange(0.0,+10.0);
	sbRangeMax->setValue( +1.0 );
	sbRangeMax->setDecimals( 4 );

	sbRangeStep = new QDoubleSpinBox;
	sbRangeStep->setRange(0.0,10.0);
	sbRangeStep->setValue( 0.1 );
	sbRangeStep->setDecimals( 4 );

	leNameRule = new QLineEdit;
	leNameRule->setText( QString::fromStdString( ModeAnimationParameters::default_filename_pattern() ) );

	lePath = new QLineEdit;
	lePath->setReadOnly( true );	
	QPushButton* btBrowse = new QPushButton("...");
	QHBoxLayout* pathLayout = new QHBoxLayout;
	pathLayout->addWidget( lePath );
	pathLayout->addWidget( btBrowse );
	pathLayout->setContentsMargins( 0,0,0,0 );

	QPushButton* btBake   = new QPushButton(tr("Bake animation"));
	QPushButton* btRender = new QPushButton(tr("Render animation"));
	
	// -- Layout

	QGridLayout* grid = new QGridLayout;
	int row=0;
	grid->addWidget( new QLabel(tr("Mode")), row,0 );
	grid->addWidget( sbMode,                 row,1 ); row++;
	grid->addWidget( new QLabel(tr("Min.")), row,0 );
	grid->addWidget( sbRangeMin,             row,1 ); row++;
	grid->addWidget( new QLabel(tr("Max.")), row,0 );
	grid->addWidget( sbRangeMax,             row,1 ); row++;
	grid->addWidget( new QLabel(tr("Step")), row,0 );
	grid->addWidget( sbRangeStep,            row,1 ); row++;
	grid->addWidget( new QLabel(tr("Filenames")), row,0 );
	grid->addWidget( leNameRule,             row,1 ); row++;
	grid->addWidget( new QLabel(tr("Output path")), row,0 );
	grid->addLayout( pathLayout,             row, 1 ); row++;	
	grid->addWidget( btBake,   row, 0, 1,2 ); row++;
	grid->addWidget( btRender, row, 0, 1,2 ); row++;

	this->setLayout( grid );

	// -- Connections (internal)

	connect( btBrowse, SIGNAL(clicked()), this, SLOT(browseOutputPath()) );
	connect( btBake,   SIGNAL(clicked()), this, SIGNAL(bakeAnimation()) );
	connect( btRender, SIGNAL(clicked()), this, SIGNAL(renderAnimation()) );
}

void ModeAnimationWidget
  ::browseOutputPath()
{
	// File path dialog
	QString path = QFileDialog::getExistingDirectory( this, 
		tr("Animation output path") );

	// User cancelled?
	if( path.isEmpty() )
		return;
	
	// Update UI
	lePath->setText( path );
}

ModeAnimationParameters ModeAnimationWidget
  ::getModeAnimationParameters() const
{
	ModeAnimationParameters p;
	
	p.filename_pattern = leNameRule->text().toStdString();
	p.output_path      = lePath    ->text().toStdString();
	
	p.mode             = sbMode     ->value();	
	p.range_min        = sbRangeMin ->value();
	p.range_max        = sbRangeMax ->value();
	p.range_stepsize   = sbRangeStep->value();

	return p;
}
