#include "RoiControls.h"	

#include <QTextBrowser>
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
RoiControls::RoiControls()
{
	maskCalculated=false;
	m_expertMode=false;
	// generate Visualisation Controls
	QGroupBox *visBox= new QGroupBox("Visualization");

	QGroupBox *roiBox= new QGroupBox("Properties");

	QPushButton * but_ScreenShot= new QPushButton("Screenshot");
	QVBoxLayout * roiBoxLayout= new QVBoxLayout();
	QVBoxLayout * visBoxLayout= new QVBoxLayout();
	QVBoxLayout * roiControlsLayout= new QVBoxLayout();

	QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum,
	   QSizePolicy::Expanding );
 
	m_chbx_ShowRoi		= new QCheckBox("Show ROI");
	m_chbx_ShowRoi->setChecked(true);
	m_chbx_ShowRoi->hide();
	m_selectionBox		= new QComboBox();
	
	m_but_AddRoi		= new QPushButton("Add ROI");
	m_but_CalculateRoi	= new QPushButton("Calculate ROI");
	m_but_Clear			= new QPushButton("Clear ROI");
	m_but_SaveMask		= new QPushButton("Save ROI");

	QHBoxLayout *addClearLayout = new QHBoxLayout();

	addClearLayout->addWidget(m_but_AddRoi);
	addClearLayout->addWidget(m_but_Clear);

	roiBoxLayout->addLayout(addClearLayout);
	roiBoxLayout->addWidget(m_selectionBox);
	roiBoxLayout->addWidget(m_but_SaveMask);
	roiBoxLayout->addWidget(m_but_CalculateRoi);	

	visBoxLayout->addWidget(m_chbx_ShowRoi);
	visBoxLayout->addWidget(but_ScreenShot);

	roiBox->setLayout(roiBoxLayout);
	visBox->setLayout(visBoxLayout);
	// create Data for properties box 

	QLabel * description= new QLabel();
	QString textdescription=tr("Description : \n"
		"Move : Control+Left Mousebutton\n"
		"Size : Control+Mousewheel Up / Down\n\n"
		"Generate a selection and press [Save ROI]\n"
		"[Save Roi] generates the volumetric mask.\n"
		"Now press [Calculate ROI] to generate a new config for this ROI\n"

		);

		description->setText(textdescription);




	roiControlsLayout->addWidget(visBox);
	roiControlsLayout->addWidget(roiBox);
	roiControlsLayout->addWidget(description);
	roiControlsLayout->addItem(spacer);
	setLayout(roiControlsLayout);

	connect(m_but_AddRoi,SIGNAL(clicked()),this,SLOT(do_AddRoi()));
	connect(m_but_Clear,SIGNAL(clicked()),this,SLOT(do_ClearRoi()));
	connect(m_chbx_ShowRoi,SIGNAL(clicked()),this,SLOT(do_ShowRoi()));
	connect(m_but_SaveMask,SIGNAL(clicked()),this,SLOT(do_SaveRoi()));
	connect(m_but_CalculateRoi,SIGNAL(clicked()),this,SLOT(do_Calculate()));
	connect(but_ScreenShot,SIGNAL(clicked()),this,SLOT(do_Screenshot()));

	connect(m_selectionBox,SIGNAL(activated(int)),this,SLOT(roiSelectionChanged(int)));

}
void RoiControls::setExpertMode(bool expert)
{
	m_expertMode=expert;
	if (expert)
		m_chbx_ShowRoi->show();
	else
		m_chbx_ShowRoi->hide();
}
void RoiControls::do_Screenshot()
{
QString filename;
	filename = QFileDialog::getSaveFileName( this,
		tr("Save Screenshot"),"", tr("(*.PNG)") );

	if( filename.isEmpty() )
		return;

	m_varVis->makeScreenShot(filename);
}

void RoiControls::do_ShowRoi()
{
	if(m_varVis->getROI().empty())
		return;

	m_varVis->setRoiVisibility(m_chbx_ShowRoi->isChecked());
	sceneUpdate();
}

void RoiControls::roiSelectionChanged(int id)
{
	m_varVis->specifyROI(id);
}

void RoiControls::do_AddRoi()
{
	if (m_varVis->getVolumePresent())
	{
		m_varVis->generateROI();
		m_selectionBox->addItem("Roi - "+QString::number(m_selectionBox->count()));
		m_varVis->specifyROI(m_selectionBox->count()-1);
		m_selectionBox->setCurrentIndex(m_selectionBox->count()-1);
	}
	else
		emit statusMessage("No volume present - cancel");
}

void RoiControls::do_Calculate()
{
	emit calculateROI();
}
void RoiControls::do_SaveRoi()
{
	if(m_varVis->getROI().empty())
		return;
	bool ok;
	QString nameOfROI= QInputDialog::getText( this,
		tr("Config options"), tr("please set the name of ROI"),
		QLineEdit::Normal, tr(""), &ok );
	if( !ok )
		return;	
	m_roiName=nameOfROI;
	if (m_roiName.contains(" "))
	{
		QMessageBox::warning(this,tr("Warning"),tr("Warning : Mask name contains spaces. \n This is not allowed. \n CANCEL!"));
		return;
	}
	if (!QDir(m_pathToMask).exists())
		QDir().mkdir(m_pathToMask);

	QString savingFileName =m_pathToMask+"/MASK_"+nameOfROI+".mhd";
	m_fullRoiName=savingFileName;
	if (m_expertMode)
	{
		if( QMessageBox::question( this, tr("VarVis: Save the Local Reference Model"),
			tr("Do you want to save the local Reference Model?"),
			QMessageBox::Yes | QMessageBox::No,  QMessageBox::Yes )
			== QMessageBox::Yes )
		{
			m_varVis->setUseLocalReference(true);
		}
	}
	else
		m_varVis->setUseLocalReference(false);

	m_varVis->saveROI(savingFileName);

}
void RoiControls::do_ClearRoi()
{
	m_varVis->clearRoi();
	m_selectionBox->clear();
	sceneUpdate();
}
void RoiControls::initRoiContols()
{
	
}


void RoiControls::sceneUpdate()
{
	m_varVis->getRenderWindow()->Render();
}
