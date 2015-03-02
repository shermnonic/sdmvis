#include "GlyphControls.h"	

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QDir>
#include <QListWidget>
#include <QProcess>

GlyphControls::GlyphControls()
	: m_varVis(0)
{
	// generate Visualisation Controls

	QGroupBox *visualisationBox     = new QGroupBox("Visualization");
	QGroupBox *propertiesBox        = new QGroupBox("Properties");
	clusterBox          = new QGroupBox("Cluster");

	QVBoxLayout * visualisationBoxLayout = new QVBoxLayout();
	QVBoxLayout * propertiesBoxLayout	 = new QVBoxLayout();
	QVBoxLayout * clusterBoxLayout	 = new QVBoxLayout();

	QVBoxLayout * glyphControlsLayout= new QVBoxLayout();	
	
	QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum,
	   QSizePolicy::Expanding );		

	m_chbx_Glyph			= new QCheckBox("Glyph");
	m_chbx_Volume			= new QCheckBox("Volume");
	m_chbx_Contour			= new QCheckBox("Contour");
	m_chbx_Lut				= new QCheckBox("Legend");
	m_chbx_samples			= new QCheckBox("Samples");
	m_chbx_Cluster		    = new QCheckBox("Cluster");
	m_chbx_UseGauss			= new QCheckBox("Blur radius");
	m_radio_getTraits		= new QRadioButton("Traits");
	m_radio_getEigentWarps  = new QRadioButton("Eigenwarps");
	m_but_ScreenShot		= new QPushButton("Screenshot");
	m_but_loadColormap      = new QPushButton("Load colormap");

	m_selectionBox			 = new QComboBox();
	m_selectionBoxEigenWarps = new QComboBox();

	m_selectionBox->setDisabled(true);
	m_selectionBoxEigenWarps->setDisabled(true);

	visualisationBoxLayout->addWidget(m_but_ScreenShot);
	visualisationBoxLayout->addWidget(m_but_loadColormap);

	visualisationBoxLayout->addWidget(m_chbx_Volume);
	visualisationBoxLayout->addWidget(m_chbx_Contour);
	visualisationBoxLayout->addWidget(m_chbx_Glyph);
	visualisationBoxLayout->addWidget(m_chbx_Cluster);
	visualisationBoxLayout->addWidget(m_chbx_samples);	

	visualisationBoxLayout->addWidget(m_chbx_Lut);
	visualisationBox->setLayout(visualisationBoxLayout);

	// create Data for properties box

	m_gausRadius				= new QDoubleSpinBox();
	m_but_UpdateGaussionRadius	= new QPushButton("Update");
	m_gausRadius ->setRange( 0.1, 20.0 );
	m_gausRadius ->setSingleStep( 0.5 );
	m_gausRadius ->setValue(1.5);	
	
	m_glyphSizeLabel			 = new QLabel("Glyph size");
	m_glyphSize				 	 = new QDoubleSpinBox();
	m_but_UpdateGlyphSize	 	 = new QPushButton("Update");
	m_glyphSize->setRange( 0.1, 2000.0 );
	m_glyphSize->setSingleStep( 0.5 );
	m_glyphSize->setValue(1.0);

	m_chbx_glyphScaleByVector = new QCheckBox("Scale glyphs by vector length");
	m_chbx_glyphAutoScaling   = new QCheckBox("Glyph autoscale");
	//m_glyphAutoScaleFactor    = new QDoubleSpinBox();
	//m_glyphAutoScaleFactor->setRange( 0.01, 10.0 );
	//m_glyphAutoScaleFactor->setSingleStep( 0.1 );
	m_infoLabel = new QLabel(tr("-"));

	m_sampleRadiusLabel			= new QLabel("Sampling radius");
	m_sampleRadius		        = new QDoubleSpinBox();
	m_but_UpdatePointRadius     = new QPushButton("Update");
	m_sampleRadius->setRange(1.0,20.0);
	m_sampleRadius->setSingleStep( 0.1 );
	m_sampleRadius->setValue(2.5);
	
	m_isoValueLabel			   = new QLabel("Isovalue");
	m_isoSurfaceValue          = new QDoubleSpinBox();
	m_but_UpdateIsoValue       = new QPushButton("Update");
	m_isoSurfaceValue->setRange( -1000.0, 16000.0 );
	m_isoSurfaceValue->setSingleStep( 0.5 );
	m_isoSurfaceValue->setValue(16);

	m_sampleLabel			= new QLabel("Number of samples");
	m_sampleRange			= new QSpinBox();
	m_but_UpdateSampleRange = new QPushButton("Update");
	m_sampleRange->setRange(10,50000) ;// hier min und max anhand der Vertex anzahl setzten oO 
	m_sampleRange->setValue(2500);
	m_sampleRange->setSingleStep(100);

	m_centroidsLabel	    = new QLabel("Number of clusters(%)");
	m_clusterNumber			= new QDoubleSpinBox();
	m_clusterNumber->setValue(0.05);
	m_clusterNumber->setMaximum(0.9);
	m_clusterNumber->setMinimum(0.01);
	m_clusterNumber->setSingleStep(0.05);

	QGridLayout *glyphPropertiesGrid = new QGridLayout();

	unsigned row=0;

	glyphPropertiesGrid->addWidget(m_isoValueLabel     ,row,0);
	glyphPropertiesGrid->addWidget(m_isoSurfaceValue   ,row,1);
	glyphPropertiesGrid->addWidget(m_but_UpdateIsoValue,row,2); row++;

	glyphPropertiesGrid->addWidget(m_glyphSizeLabel     ,row,0);
	glyphPropertiesGrid->addWidget(m_glyphSize          ,row,1);
	glyphPropertiesGrid->addWidget(m_but_UpdateGlyphSize,row,2); row++;
	glyphPropertiesGrid->addWidget(m_chbx_glyphScaleByVector,row,0,1,2); row++;
	glyphPropertiesGrid->addWidget(m_chbx_glyphAutoScaling  ,row,0,1,2); row++;
	
	glyphPropertiesGrid->addWidget(m_sampleRadiusLabel    ,row,0);
	glyphPropertiesGrid->addWidget(m_sampleRadius         ,row,1);
	glyphPropertiesGrid->addWidget(m_but_UpdatePointRadius,row,2); row++;

	glyphPropertiesGrid->addWidget(m_sampleLabel          ,row,0);
	glyphPropertiesGrid->addWidget(m_sampleRange          ,row,1);
	glyphPropertiesGrid->addWidget(m_but_UpdateSampleRange,row,2); row++;

	glyphPropertiesGrid->addWidget(m_chbx_UseGauss           ,row,0); row++;
	glyphPropertiesGrid->addWidget(m_gausRadius              ,row,1);
	glyphPropertiesGrid->addWidget(m_but_UpdateGaussionRadius,row,2);


	m_but_Cluster= new QPushButton("Cluster");
	m_but_ClusterVolume= new QPushButton("Cluster volume");

	QHBoxLayout * getTraitsLayout= new QHBoxLayout();
	QHBoxLayout * getEigenWarpsLayout= new QHBoxLayout();

	getTraitsLayout->addWidget(m_radio_getTraits,0);
	getTraitsLayout->addWidget(m_selectionBox,2);

	getEigenWarpsLayout->addWidget(m_radio_getEigentWarps,0);
	getEigenWarpsLayout->addWidget(m_selectionBoxEigenWarps,2);

	propertiesBoxLayout->addLayout(getTraitsLayout);
	propertiesBoxLayout->addLayout(getEigenWarpsLayout);
	
	propertiesBoxLayout->addLayout(glyphPropertiesGrid);

//	propertiesBoxLayout->addWidget(m_centroidsLabel);
//	propertiesBoxLayout->addWidget(m_clusterNumber);
//	propertiesBoxLayout->addWidget(m_but_Cluster);
//	propertiesBoxLayout->addWidget(m_but_ClusterVolume);

	clusterBoxLayout->addWidget(m_centroidsLabel);
	clusterBoxLayout->addWidget(m_clusterNumber);
	clusterBoxLayout->addWidget(m_but_Cluster);
	clusterBoxLayout->addWidget(m_but_ClusterVolume);

	clusterBox->setLayout(clusterBoxLayout);
	propertiesBox->setLayout(propertiesBoxLayout);

	QGridLayout* infoLayout = new QGridLayout;
	infoLayout->addWidget( new QLabel(tr("Max. vector length")), 0,0 );
	infoLayout->addWidget( m_infoLabel, 0,1 );

	QGroupBox* infoBox = new QGroupBox(tr("Vectorfield statistics"));
	infoBox->setLayout( infoLayout );

	glyphControlsLayout->addWidget(visualisationBox);
	glyphControlsLayout->addWidget(propertiesBox);
	glyphControlsLayout->addWidget(infoBox);
	glyphControlsLayout->addWidget(clusterBox);
	glyphControlsLayout->addItem(spacer);
	setLayout(glyphControlsLayout);

	// mark advanced UI elements
	m_advancedOptions.clear();
	m_advancedOptions.push_back( m_clusterNumber );
	m_advancedOptions.push_back( m_chbx_UseGauss );
	m_advancedOptions.push_back( m_but_Cluster );
	m_advancedOptions.push_back( m_but_ClusterVolume );
	m_advancedOptions.push_back( m_isoSurfaceValue );
	m_advancedOptions.push_back( m_but_UpdateIsoValue );
	m_advancedOptions.push_back( m_glyphSize );
	m_advancedOptions.push_back( m_chbx_glyphScaleByVector );
	m_advancedOptions.push_back( m_chbx_glyphAutoScaling );
	m_advancedOptions.push_back( m_but_UpdateGlyphSize );
	m_advancedOptions.push_back( m_sampleRadius );
	m_advancedOptions.push_back( m_but_UpdatePointRadius );
	m_advancedOptions.push_back( m_sampleRange );
	m_advancedOptions.push_back( m_but_UpdateSampleRange );
	m_advancedOptions.push_back( m_gausRadius );
	m_advancedOptions.push_back( m_but_UpdateGaussionRadius );
	m_advancedOptions.push_back( m_chbx_Volume );
	m_advancedOptions.push_back( m_chbx_Contour );
	m_advancedOptions.push_back( m_chbx_Glyph );
	m_advancedOptions.push_back( m_chbx_Cluster );
	m_advancedOptions.push_back( m_chbx_samples );
	m_advancedOptions.push_back( m_glyphSizeLabel );
	m_advancedOptions.push_back( m_sampleRadiusLabel );
	m_advancedOptions.push_back( m_isoValueLabel );
	m_advancedOptions.push_back( m_sampleLabel );
	m_advancedOptions.push_back( m_centroidsLabel );
	m_advancedOptions.push_back( clusterBox );
	m_advancedOptions.push_back( m_but_loadColormap );
	m_advancedOptions.push_back( m_chbx_Glyph );
	m_advancedOptions.push_back( m_chbx_Lut );


	// connections for checkBoxes
	connect(m_chbx_Cluster ,SIGNAL(toggled(bool)),this,SLOT(show_cluster(bool)));
	connect(m_chbx_Glyph   ,SIGNAL(toggled(bool)),this,SLOT(do_Glyph    (bool)));
	connect(m_chbx_Volume  ,SIGNAL(toggled(bool)),this,SLOT(do_Volume   (bool)));
	connect(m_chbx_Contour ,SIGNAL(toggled(bool)),this,SLOT(do_Contour  (bool)));
	connect(m_chbx_Lut     ,SIGNAL(toggled(bool)),this,SLOT(do_Lut      (bool)));
	connect(m_chbx_samples ,SIGNAL(toggled(bool)),this,SLOT(do_Samples  (bool)));
	connect(m_chbx_UseGauss,SIGNAL(toggled(bool)),this,SLOT(do_Gaussian (bool)));
	connect(m_radio_getTraits     ,SIGNAL(clicked()),this,SLOT(getTraits()));
	connect(m_radio_getEigentWarps,SIGNAL(clicked()),this,SLOT(getEigenWarps()));	
	
	// connections for Buttons
	connect(m_but_UpdateIsoValue,SIGNAL(clicked()),this,SLOT(do_IsoUpdate()));
	connect(m_but_UpdateSampleRange,SIGNAL(clicked()),this,SLOT(do_SampleUpdate()));
	connect(m_but_UpdatePointRadius,SIGNAL(clicked()),this,SLOT(do_updatePointRadius()));
	connect(m_but_UpdateGlyphSize,SIGNAL(clicked()),this,SLOT(do_GlyphUpdate()));
	connect(m_but_UpdateGaussionRadius,SIGNAL(clicked()),this,SLOT(do_updateGaussion()));
	connect(m_but_Cluster,SIGNAL(clicked()),this,SLOT(do_Cluster()));
	connect(m_but_ClusterVolume,SIGNAL(clicked()),this,SLOT(do_VolumeCluster()));
	connect(m_selectionBox,SIGNAL(activated(int)),this,SLOT(warpSelectionChanged(int)));
	connect(m_selectionBoxEigenWarps,SIGNAL(activated(int)),this,SLOT(loadEigenWarp(int)));
	connect(m_but_ScreenShot,SIGNAL(clicked()),this,SLOT(do_ScreenShot()));
	connect(m_but_loadColormap,SIGNAL(clicked()),this,SLOT(loadColormap()));

	// set some defaults
	m_chbx_glyphScaleByVector->setChecked(true);
	m_chbx_glyphAutoScaling->setChecked(true);
	m_chbx_Contour->setChecked(true);
	m_chbx_Lut->setChecked(true);
	m_chbx_Glyph->setChecked(true);
	m_chbx_samples->setChecked(false);
	m_chbx_UseGauss->setChecked(false);
	m_chbx_Cluster->setChecked(true);
	m_radio_getEigentWarps->setChecked(true);
	getTrait_oldState=false;
	getEigenWarpd_oldState=true;
	setExpertMode(false);
}


void GlyphControls::setVarVisRenderer( VarVisRender *varvisRen )
{
	m_varVis=varvisRen;
	if( !m_varVis )
		return;
#if 1
	// synchronize VarVis with control settings
	// design decision: either get VarVis state and set controls accordingly
	//                  or set VarVis states according to current controls
	// since some VarVis getters are missing, we apply current controls

	m_varVis->setGlyphSize(getGlyphSize());
	m_varVis->setIsovalue(getIsoValue());

	m_varVis->setGlyphVisible    ( m_chbx_Glyph  ->isChecked() );
	m_varVis->setVolumeVisibility( m_chbx_Volume ->isChecked() );
	m_varVis->setMeshVisibility  ( m_chbx_Contour->isChecked() );
	m_varVis->setBarsVisibility  ( m_chbx_Lut    ->isChecked() );
	m_varVis->setSamplesVisibility(m_chbx_samples->isChecked() );
	m_varVis->showCluster        ( m_chbx_Cluster->isChecked() );

	// TODO: check if all relevant settings are included

	sceneUpdate();
#endif
}


void GlyphControls::setExpertMode( bool enable )
{
	for( int i=0; i < m_advancedOptions.size(); i++ )
		m_advancedOptions[i]->setVisible( enable );
}

void GlyphControls::do_ScreenShot()
{
	if(!m_varVis) return;

	QString filename;
	filename = QFileDialog::getSaveFileName( this,
		tr("Save Screenshot"),"", tr("(*.png)") );

	if( filename.isEmpty() )
		return;

	m_varVis->makeScreenShot(filename);
}


void GlyphControls::warpSelectionChanged(int id)
{
	emit setwarpId(id);
}

void GlyphControls::do_VolumeCluster()
{
	if(!m_varVis) return;
	m_varVis->clusterVolumeGlyphs();
	m_varVis->getRenderWindow()->Render();
}

void GlyphControls::do_Cluster()
{
	if(!m_varVis) return;
	m_varVis->setCentroidNumber(m_clusterNumber->value());
	m_varVis->clusterGlyphs();
}

void GlyphControls::do_updatePointRadius()
{
	if(m_varVis) m_varVis->setPointRadius(m_sampleRadius->value());
	sceneUpdate();
}

void GlyphControls::show_cluster( bool b )
{
	if(m_varVis) m_varVis->showCluster(b);
}

void GlyphControls::do_updateGaussion()
{
	m_varVis->setGaussionRadius(m_gausRadius->value());
	if (!m_varVis->getLoadedReference().isEmpty()){
		m_radio_getEigentWarps->setChecked(false);
		m_radio_getTraits->setChecked(false);
	
		m_selectionBox->setEnabled(false);
		m_selectionBoxEigenWarps->setEnabled(false);

		m_varVis->load_reference(m_varVis->getLoadedReference().toAscii());
	}
}

void GlyphControls::updateInfo()
{
	// Update info
	if( m_varVis->getWarpVis() )
		m_infoLabel->setText(tr("%1")
			.arg( m_varVis->getWarpVis()->getMaxOrthVector() ));
}

void GlyphControls::sceneUpdate()
{
	if(m_varVis)
	{
		m_varVis->getRenderWindow()->Render();

		updateInfo();
	}
}

void GlyphControls::do_GlyphUpdate() 
{
	if(!m_varVis) return;

	m_varVis->setGlyphSize(getGlyphSize());

	m_varVis->setGlyphAutoScaling( getGlyphAutoScaling() );

	if( m_chbx_glyphScaleByVector->isChecked() )
		m_varVis->getWarpVis()->getGlyph3D()->SetScaleModeToScaleByVector();
	else
		m_varVis->getWarpVis()->getGlyph3D()->SetScaleModeToDataScalingOff();	

	sceneUpdate();
}

void GlyphControls::do_IsoUpdate()   
{
	if(!m_varVis) return;
	m_varVis->setIsovalue(getIsoValue());
	sceneUpdate();
}

void GlyphControls::do_SampleUpdate()
{
	m_varVis->setPointSize(getSampleRadius());
	m_varVis->updateSamplePoints(getSampleRange());
	sceneUpdate();
}

void GlyphControls::do_Glyph   (bool b) { if(m_varVis) m_varVis->setGlyphVisible(b);     sceneUpdate();}
void GlyphControls::do_Volume  (bool b) { if(m_varVis) m_varVis->setVolumeVisibility(b);	sceneUpdate();}

void GlyphControls::do_Contour (bool b) { if(m_varVis) m_varVis->setMeshVisibility(b);   sceneUpdate();}
void GlyphControls::do_Lut     (bool b) { if(m_varVis) m_varVis->setBarsVisibility(b);   sceneUpdate();}
void GlyphControls::do_Samples (bool b) { if(m_varVis) m_varVis->setSamplesVisibility(b);sceneUpdate();}
void GlyphControls::do_Gaussian(bool b) { if(m_varVis) m_varVis->useGaussion(b);         sceneUpdate();}

double GlyphControls::getGlyphSize     () const { return m_glyphSize      ->value(); }
double GlyphControls::getGaussionRadius() const { return m_gausRadius     ->value(); }	
double GlyphControls::getWarpScaleValue() const { return m_warpScale      ->value(); }
double GlyphControls::getSampleRadius  () const { return m_sampleRadius   ->value(); }
double GlyphControls::getIsoValue      () const { return m_isoSurfaceValue->value(); }
int    GlyphControls::getSampleRange   () const { return m_sampleRange    ->value(); }

void GlyphControls::getTraits()
{
	// reduce reload when it is not needed
//	if (m_radio_getTraits->isChecked()!=getTrait_oldState)
//	{
		m_selectionBox->setEnabled(true);
		m_selectionBoxEigenWarps->setEnabled(false);
		emit SIG_getTraitsfromSDMVIS(m_radio_getTraits->isChecked());
		getTrait_oldState=m_radio_getTraits->isChecked();
		getEigenWarpd_oldState=m_radio_getEigentWarps->isChecked();

//	}
}

void GlyphControls::setTraitsFromSDMVIS(bool yes)
{
	m_radio_getTraits->blockSignals(true);
	m_radio_getTraits->setChecked(yes);
	m_radio_getTraits->blockSignals(false);
}

bool GlyphControls::setWarp( QString mhdFilename )
{
	m_varVis->setWarp(mhdFilename, false);
	return true;
}


void GlyphControls::loadEigenWarp(int item)
{
	// sanity checks
	if( m_varVis ) 
		if( (item>=0) && (item<m_eigenWarpsList.size()) )
		{
			m_varVis->setWarp(m_eigenWarpsList.at(item),false);
			updateInfo();
		}
}

void GlyphControls::clearEigenwarps()
{
	m_eigenWarpsList.clear();
	m_selectionBoxEigenWarps->clear();
	m_selectionBoxEigenWarps->setEnabled(false);
}

void GlyphControls::addEigenwarp( QString filename )
{
	QFileInfo info( filename );
	m_eigenWarpsList.push_back( filename );
	m_selectionBoxEigenWarps->addItem( info.fileName() );
}

void GlyphControls::setEigenwarps( const QStringList& mhdFilenames )
{
	clearEigenwarps();

	for( int i=0; i < mhdFilenames.size(); i++ )
	{
		addEigenwarp( mhdFilenames.at(i) );
	}

	if( m_eigenWarpsList.size() > 0 )
	{
		loadEigenWarp( 0 );
		m_selectionBoxEigenWarps->setEnabled( true );
	}
}

void GlyphControls::getEigenWarps()
{	
	// Enable eigenwarp dialog box, if eigenwarps are already set.	
	if( m_eigenWarpsList.size() > 0 )
	{
		loadEigenWarp( 0 );
		m_selectionBoxEigenWarps->setEnabled( true );
	}
}

void GlyphControls::addTraitSelection(QComboBox * box)
{
	m_selectionBox->clear();
	m_selectionBox->setEnabled(true);
	m_radio_getTraits->setEnabled(true);
	for (int iA=0;iA<box->count();iA++)
	{
		m_selectionBox->addItem(box->itemText(iA));
	}
	m_selectionBox->blockSignals(true);
	m_selectionBox->setCurrentIndex(box->currentIndex());
	m_selectionBox->blockSignals(false);
}

void GlyphControls::clearTraitSelection()
{
	m_selectionBox->clear();
	m_selectionBox->setDisabled(true);
	m_radio_getTraits->setChecked(false);
}	

void GlyphControls::loadColormap()
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

	// load colormap from disk
	ColorMapRGB colormap;
	if( colormap.read( filename.toStdString().c_str() ) )
	{
		// on success, set colormap
		setColormap( colormap );
		emit StatusMessage( tr("Set colormap to %1").arg(info.fileName()) );
	}
	else
	{
		emit StatusMessage( tr("Failed to load %1").arg(info.fileName()) );
	}

	// remember directory
	colorBaseDir = info.absolutePath();
}

void GlyphControls::setColormap( const ColorMapRGB& colormap )
{
	m_colormap = colormap;	
	m_colormap.applyTo( m_varVis->getWarpVis()->getGlyphMapper()->GetLookupTable() ); //m_varVis->getWarpVis()->getLutTangentialComponent() );
	m_varVis->getWarpVis()->getGlyph3D()->Update();
	//m_varVis->getWarpVis()->updateColorBars();	
}
