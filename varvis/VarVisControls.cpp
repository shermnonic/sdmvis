#include "varVisControls.h"	

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

// Constructor
VarVisControls::VarVisControls(QWidget *parent)
{
	// generate and Initialisate Vars
	QGroupBox *volumeBox  = new QGroupBox("Volume");
	QGroupBox *warpBox    = new QGroupBox("Warp");
	QGroupBox *startBox   = new QGroupBox("Setup (reload required)");
	QGroupBox *animationBox   = new QGroupBox("Animation");

	QVBoxLayout *volumeLayout = new QVBoxLayout;
	QVBoxLayout *warpLayout   = new QVBoxLayout;
	QVBoxLayout *startLayout  = new QVBoxLayout;
	QVBoxLayout *chbx_Layout  = new QVBoxLayout;

	QHBoxLayout *radiusLayout        = new QHBoxLayout;
	QHBoxLayout *sampleRadiusLayout  = new QHBoxLayout;
	QHBoxLayout *sampleLayout        = new QHBoxLayout;
	QVBoxLayout *animationLayout     = new QVBoxLayout;

	m_but_StartAnimation	= new QPushButton("Start");
	m_but_ScreenShot		= new QPushButton("Screenshot");
	m_but_PauseAnimation	= new QPushButton("Pause");
	m_but_StopAnimation		= new QPushButton("Stop");
	m_but_ROI			    = new QPushButton("Add ROI");
	m_but_SaveRoi			= new QPushButton("Save ROI");
	m_but_ClearROI		    = new QPushButton("Clear ROI");
	m_but_Cluster			= new QPushButton("Cluster");
	m_but_VolumeCluster     = new QPushButton("Cluster volumetric");
	m_comb_RoiSelection     = new QComboBox();

	// checkBoxes

	m_chbx_Glyph			= new QCheckBox("Glyph visble");
	m_chbx_Volume			= new QCheckBox("Volume visble");
	m_chbx_Contour			= new QCheckBox("Contour visble");
	m_chbx_isoContour		= new QCheckBox("Show reference");
	m_chbx_Lut				= new QCheckBox("Show scalar bar");
	m_chbx_backwardsTr		= new QCheckBox("Backwards transformation");
	m_chbx_samples			= new QCheckBox("Show samples");
	m_chbx_silhouette		= new QCheckBox("Show silhouette");
	m_chbx_gaussion		    = new QCheckBox("Gaussian filter");
	m_chbx_Roi  		    = new QCheckBox("Show ROI");
	m_chbx_Cluster		    = new QCheckBox("Show clusters");
	m_chbx_getTraits		= new QCheckBox("Get traits");
	m_frameSlider			= new QSlider(Qt::Horizontal);
	// random stuff
	animationLayout->addWidget(m_but_StartAnimation);
	animationLayout->addWidget(m_but_PauseAnimation);
	animationLayout->addWidget(m_but_StopAnimation);
	animationLayout->addWidget(m_frameSlider);

	m_frameSlider->setRange(0, 20);
    m_frameSlider->setSingleStep(1);
	m_frameSlider->setPageStep(1);


	QHBoxLayout * ROILayout= new QHBoxLayout;
	ROILayout->addWidget(m_but_ROI);
	ROILayout->addWidget(m_but_ClearROI);
	
	animationLayout->addWidget(m_but_ScreenShot);
	animationLayout->addLayout(ROILayout);
	animationLayout->addWidget(m_comb_RoiSelection);
	animationLayout->addWidget(m_but_SaveRoi);

	QLabel *radiusLabel	    = new QLabel("Radius");
	m_gaussionRadius		= new QDoubleSpinBox();

	QLabel *sampleLabel	    = new QLabel("Sample size");
	m_sampleRange			= new QSpinBox();
	m_but_UpdateSampleRange = new QPushButton("Update");
	
	QLabel *scaleLabel	    =new QLabel("Warp scale");
	m_warpScale				=new QDoubleSpinBox();
	m_but_UpdateWarpScale   =new QPushButton("Update");

	QHBoxLayout *scaleLayout = new QHBoxLayout;
	scaleLayout->addWidget(scaleLabel);
	scaleLayout->addWidget(m_warpScale);
	scaleLayout->addWidget(m_but_UpdateWarpScale);

	m_warpScale->setRange(0.1,3.0);
	m_warpScale->setSingleStep( 0.1 );
	m_warpScale->setValue(0.2);

	QLabel *sampleRadiusLabel	= new QLabel("Sampling radius");
	m_sampleRadius		        = new QDoubleSpinBox();
	m_but_UpdatePointRadius     = new QPushButton("Update");
	
	QLabel *isoValueLabel      = new QLabel("Isovalue");
	m_isoSurfaceValue          = new QDoubleSpinBox();
	QHBoxLayout *surfaceLayout = new QHBoxLayout;
	m_but_UpdateIsoValue       = new QPushButton("Update");

	m_glyphAutoScaling           = new QCheckBox("Auto scaling");

	QHBoxLayout *glyphSizeLayout = new QHBoxLayout;
	QLabel *glyphSizeLabel	     = new QLabel("Glyph size");
	m_glyphSize				 	 = new QDoubleSpinBox();
	m_but_UpdateGlyphSize	 	 = new QPushButton("Update");

	glyphSizeLayout->addWidget(glyphSizeLabel);
	glyphSizeLayout->addWidget(m_glyphSize);
	glyphSizeLayout->addWidget(m_but_UpdateGlyphSize);

	m_glyphSize->setRange( 0.1, 20.0 );
	m_glyphSize->setSingleStep( 0.5 );
	m_glyphSize->setValue(5);

	surfaceLayout->addWidget(isoValueLabel);
	surfaceLayout->addWidget(m_isoSurfaceValue);
	surfaceLayout->addWidget(m_but_UpdateIsoValue);
	m_isoSurfaceValue->setRange( 0.1, 500.0 );
	m_isoSurfaceValue->setSingleStep( 0.5 );
	m_isoSurfaceValue->setValue(16);

	m_but_UpdateGaussion= new QPushButton("Update");
	radiusLayout->addWidget(radiusLabel);
	radiusLayout->addWidget(m_gaussionRadius);
	radiusLayout->addWidget(m_but_UpdateGaussion);

	sampleRadiusLayout->addWidget(sampleRadiusLabel);
	sampleRadiusLayout->addWidget(m_sampleRadius);
	sampleRadiusLayout->addWidget(m_but_UpdatePointRadius);

	sampleLayout->addWidget(sampleLabel);
	sampleLayout->addWidget(m_sampleRange);
	sampleLayout->addWidget(m_but_UpdateSampleRange);

	m_gaussionRadius->setRange( 0.1, 4.0 );
	m_gaussionRadius->setSingleStep( 0.1 );
	m_gaussionRadius->setValue(1.5);	
	
	m_sampleRange->setRange(10,10000) ;// FIXME: hier min und max anhand der Vertex anzahl setzten oO 
	m_sampleRange->setValue(5000);
	m_sampleRange->setSingleStep(100);

	m_sampleRadius->setRange(1.0,20.0);
	m_sampleRadius->setSingleStep( 0.1 );
	m_sampleRadius->setValue(2.5);

	//chbx_Layout->addWidget(m_but_ResetCamera);
	
	volumeLayout->addWidget(m_chbx_Volume);
	volumeLayout->addWidget(m_chbx_isoContour);
	volumeLayout->addWidget(m_chbx_Contour);
	volumeLayout->addWidget(m_chbx_samples);
	volumeLayout->addWidget(m_chbx_silhouette);
	volumeLayout->addWidget(m_chbx_Roi);

	volumeLayout->addLayout(surfaceLayout);
	
	
	QLabel *centroidsLabel = new QLabel("Percentage centroids");
	m_clusterNumber= new QDoubleSpinBox();

	m_clusterNumber->setValue(0.05);
	m_clusterNumber->setMaximum(0.9);
	m_clusterNumber->setMinimum(0.01);
	m_clusterNumber->setSingleStep(0.05);

	m_selectionBox= new QComboBox();
	warpLayout->addWidget(m_selectionBox);
	warpLayout->addWidget(m_chbx_getTraits);
	warpLayout->addWidget(m_chbx_Glyph);
	warpLayout->addWidget(m_chbx_Lut);
	warpLayout->addWidget(m_glyphAutoScaling);
	warpLayout->addLayout(glyphSizeLayout);
	warpLayout->addLayout(scaleLayout);
	warpLayout->addWidget(m_but_Cluster);
	warpLayout->addWidget(m_but_VolumeCluster);
	warpLayout->addWidget(m_chbx_Cluster);
	warpLayout->addWidget(centroidsLabel);
	warpLayout->addWidget(m_clusterNumber);
	
	
    m_selectionBox->setDisabled(true);
	m_chbx_getTraits->setDisabled(true);
	
	startLayout->addWidget(m_chbx_backwardsTr);
	startLayout->addWidget(m_chbx_gaussion);
	startLayout->addLayout(radiusLayout);
	startLayout->addLayout(sampleLayout);
	startLayout->addLayout(sampleRadiusLayout);
	
	volumeBox->setLayout(volumeLayout);
	warpBox->setLayout(warpLayout);
	startBox->setLayout(startLayout);
	animationBox->setLayout(animationLayout);

	chbx_Layout->addWidget(volumeBox);
	chbx_Layout->addWidget(warpBox);
	chbx_Layout->addWidget(startBox);
	chbx_Layout->addWidget(animationBox);

	// set startup values
	m_chbx_Contour->setChecked(true);
	m_chbx_isoContour->setChecked(false);
	m_chbx_Lut->setChecked(true);
	m_chbx_Glyph->setChecked(true);	
	m_chbx_samples->setChecked(false);
	m_chbx_silhouette->setChecked(false);
	m_chbx_gaussion->setChecked(false);
	m_chbx_Roi->setChecked(true);
	m_chbx_Cluster->setChecked(true);
	setLayout(chbx_Layout);

	// connections for checkBoxes
	connect(m_chbx_Cluster,SIGNAL(clicked()),this,SLOT(show_cluster()));
	connect(m_chbx_Glyph,SIGNAL(clicked()),this,SLOT(do_Glyph()));
	connect(m_chbx_Volume,SIGNAL(clicked()),this,SLOT(do_Volume()));
	connect(m_chbx_Contour,SIGNAL(clicked()),this,SLOT(do_Contour()));
	connect(m_chbx_isoContour,SIGNAL(clicked()),this,SLOT(do_isoContour()));
	connect(m_chbx_Lut,SIGNAL(clicked()),this,SLOT(do_Lut()));
	connect(m_chbx_samples,SIGNAL(clicked()),this,SLOT(do_Samples()));
	connect(m_chbx_silhouette,SIGNAL(clicked()),this,SLOT(do_Silhouette()));
	connect(m_chbx_gaussion,SIGNAL(clicked()),this,SLOT(do_Gaussion()));
	connect(m_chbx_Roi,SIGNAL(clicked()),this,SLOT(do_ShowRoi()));
	connect(m_chbx_getTraits,SIGNAL(clicked()),this,SLOT(getTraits()));

	connect( m_glyphAutoScaling, SIGNAL(toggled(bool)), this, SLOT(setGlyphAutoScaling(bool)) );
	
	// connections for Buttons
	connect(m_but_UpdateIsoValue,SIGNAL(clicked()),this,SLOT(do_IsoUpdate()));
	connect(m_but_UpdateSampleRange,SIGNAL(clicked()),this,SLOT(do_SampleUpdate()));
	connect(m_but_UpdateGlyphSize,SIGNAL(clicked()),this,SLOT(do_GlyphUpdate()));
	connect(m_but_UpdateWarpScale,SIGNAL(clicked()),this,SLOT(do_ScaleUpdate()));
	connect(m_but_StartAnimation,SIGNAL(clicked()),this,SLOT(do_Animation()));

	connect(m_but_StopAnimation,SIGNAL(clicked()),this,SLOT(do_StopAnimation()));
	connect(m_but_PauseAnimation,SIGNAL(clicked()),this,SLOT(do_PauseAnimation()));

	connect(m_but_ScreenShot,SIGNAL(clicked()),this,SLOT(do_ScreenShot()));
	connect(m_but_ROI,SIGNAL(clicked()),this,SLOT(do_ROI()));
	connect(m_but_ClearROI,SIGNAL(clicked()),this,SLOT(do_ClearROI()));
	connect(m_but_SaveRoi,SIGNAL(clicked()),this,SLOT(do_SaveROI()));
	connect(m_but_Cluster,SIGNAL(clicked()),this,SLOT(do_Cluster()));
	connect(m_but_VolumeCluster,SIGNAL(clicked()),this,SLOT(do_VolumeCluster()));

	connect(m_comb_RoiSelection,SIGNAL(activated(int)),this,SLOT(selectROI(int)));
	connect(m_but_UpdateGaussion,SIGNAL(clicked()),this,SLOT(do_updateGaussion()));
	connect(m_but_UpdatePointRadius,SIGNAL(clicked()),this,SLOT(do_updatePointRadius()));
	connect(m_selectionBox,SIGNAL(activated(int)),this,SLOT(warpSelectionChanged(int)));
}
void VarVisControls::do_VolumeCluster()
{
	m_vren->clusterVolumeGlyphs();
	m_vren->getRenderWindow()->Render();
}
void VarVisControls::setGetTraitsFromSDMVIS(bool yes)
{
	m_chbx_getTraits->blockSignals(true);
	m_chbx_getTraits->setChecked(yes);
	m_chbx_getTraits->blockSignals(false);

}

void VarVisControls::warpSelectionChanged(int id)
{
emit setwarpId(id);

}

void VarVisControls::getTraits()
{
	emit getTraitsfromSDMVIS(m_chbx_getTraits->isChecked());
}
void VarVisControls::addTraitSelection(QComboBox * box)
{
	m_selectionBox->clear();
	m_selectionBox->setEnabled(true);
	m_chbx_getTraits->setEnabled(true);
	for (int iA=0;iA<box->count();iA++)
	{
		m_selectionBox->addItem(box->itemText(iA));
	}
	m_selectionBox->blockSignals(true);
	m_selectionBox->setCurrentIndex(box->currentIndex());
	m_selectionBox->blockSignals(false);
}	
void VarVisControls::clearTraitSelection()
{
	m_selectionBox->clear();
	m_selectionBox->setDisabled(true);
}	

void VarVisControls::do_Cluster()
{
	m_vren->setCentroidNumber(m_clusterNumber->value());
	m_vren->clusterGlyphs();
}
void VarVisControls::do_updatePointRadius()
{
	m_vren->setPointRadius(m_sampleRadius->value());
	sceneUpdate();

}
void VarVisControls::show_cluster()
{
	m_vren->showCluster(m_chbx_Cluster->isChecked());


}
void VarVisControls::do_updateGaussion()
{
	m_vren->setGaussionRadius(m_gaussionRadius->value());
	if (!m_vren->getLoadedReference().isEmpty())
		m_vren->load_reference(m_vren->getLoadedReference().toAscii());
}
void VarVisControls::do_ClearROI()
{
m_vren->clearRoi();
m_comb_RoiSelection->clear();
sceneUpdate();
}
void VarVisControls::do_ShowRoi()
{
if(m_vren->getROI().empty())
		return;

	m_vren->setRoiVisibility(m_chbx_Roi->isChecked());
	sceneUpdate();

}
void VarVisControls::do_SaveROI()
{
	if(m_vren->getROI().empty())
		return;
	bool ok;
	QString nameOfROI= QInputDialog::getText( this,
		tr("Config options"), tr("please set the name of ROI"),
		QLineEdit::Normal, tr(""), &ok );
	if( !ok )
		return;	
	m_roiName=nameOfROI;
	if (!QDir(m_pathToMask).exists())
		QDir().mkdir(m_pathToMask);

	QString savingFileName =m_pathToMask+"/MASK_"+nameOfROI+".mhd";
	m_fullRoiName=savingFileName;
	if( QMessageBox::question( this, tr("VarVis: Save the Local Reference Model"),
		tr("Do you want to save the local Reference Model?"),
		QMessageBox::Yes | QMessageBox::No,  QMessageBox::Yes )
		== QMessageBox::Yes )
	{
		m_vren->setUseLocalReference(true);
	}
	m_vren->saveROI(savingFileName);

}
void VarVisControls::do_ROI()
{
	m_vren->generateROI();
	m_comb_RoiSelection->addItem("Roi - "+QString::number(m_comb_RoiSelection->count()));
	m_vren->specifyROI(m_comb_RoiSelection->count()-1);
	m_comb_RoiSelection->setCurrentIndex(m_comb_RoiSelection->count()-1);
}


void VarVisControls::selectROI(int index)
{
	m_vren->specifyROI(index);
}

void VarVisControls::sceneUpdate()
{
m_vren->getRenderWindow()->Render();
}
void VarVisControls::do_StopAnimation()
{
m_vren->stopAnimation();

}
void VarVisControls::do_PauseAnimation()
{
m_vren->pauseAnimation();
}

void VarVisControls::do_ScreenShot()
{
	QString filename;
	filename = QFileDialog::getSaveFileName( this,
		tr("Save Screenshot"),"", tr("(*.PNG)") );

	if( filename.isEmpty() )
		return;

	m_vren->makeScreenShot(filename);
}
void VarVisControls::do_Animation()   {m_vren->startAnimation();sceneUpdate();}
void VarVisControls::do_GlyphUpdate() {m_vren->setGlyphSize(getGlyphSize());sceneUpdate();}
void VarVisControls::do_IsoUpdate()   {m_vren->setIsovalue(getIsoValue());sceneUpdate();}
void VarVisControls::do_ScaleUpdate() {m_vren->setScaleValue(getWarpScaleValue());sceneUpdate();}
void VarVisControls::do_SampleUpdate()
{
	m_vren->setPointSize(getSampleRadius());
	
	m_vren->updateSamplePoints(getSampleRange());
	sceneUpdate();
}


void VarVisControls::do_Glyph()
{
	m_vren->setGlyphVisible(m_chbx_Glyph->isChecked());
	sceneUpdate();
}

void VarVisControls::do_Volume()
{
	m_vren->setVolumeVisibility (m_chbx_Volume->isChecked());
	sceneUpdate();
}
void VarVisControls::do_Contour()     {m_vren->setMeshVisibility(m_chbx_Contour->isChecked());sceneUpdate();}
void VarVisControls::do_isoContour()  {m_vren->setRefMeshVisibility  (m_chbx_isoContour->isChecked());sceneUpdate();}
void VarVisControls::do_Lut()         {m_vren->setBarsVisibility(m_chbx_Lut->isChecked());sceneUpdate();}
void VarVisControls::do_Samples()     {m_vren->setSamplesVisibility(m_chbx_samples->isChecked());sceneUpdate();}
void VarVisControls::do_Silhouette()  {m_vren->setSilhouetteVisible(m_chbx_silhouette->isChecked());sceneUpdate();}
void VarVisControls::do_Gaussion()    {m_vren->useGaussion(m_chbx_gaussion->isChecked());sceneUpdate();}



double VarVisControls::getGlyphSize()     {return m_glyphSize->value();}
double VarVisControls::getGaussionRadius(){return m_gaussionRadius->value();}	
double VarVisControls::getWarpScaleValue(){return m_warpScale->value();}
double VarVisControls::getSampleRadius()  {return m_sampleRadius->value();}
double VarVisControls::getIsoValue()      {return m_isoSurfaceValue->value();}
int VarVisControls::getSampleRange()	  {return m_sampleRange->value();}


void VarVisControls::setGlyphAutoScaling( bool b )
{
	m_vren->setGlyphAutoScaling( b );
	sceneUpdate();
}