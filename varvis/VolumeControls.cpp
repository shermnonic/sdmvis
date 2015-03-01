#include "VolumeControls.h"	

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
VolumeControls::VolumeControls()
{

	QGroupBox *propertiesBox       = new QGroupBox("Properties");
	QGroupBox *visualisationBox    = new QGroupBox("Visualization");
	QGroupBox *datasetBox		   = new QGroupBox("Dataset (aligned and registered)");
	QGroupBox *calculationBox      = new QGroupBox("Calculations");
	
	QGridLayout * propertiesGrid  = new QGridLayout();


	QVBoxLayout *propertiesLayout		= new QVBoxLayout();
	QVBoxLayout *visualisationLayout	= new QVBoxLayout();
	QVBoxLayout *datasetLayout			= new QVBoxLayout();
	QVBoxLayout *calculationLayout		= new QVBoxLayout();
	

	m_itemList = new QListWidget();
	m_useDifferenceVol=false;

	// generate layouts and widgets 
	QVBoxLayout *volumeLayout= new QVBoxLayout();
	m_chbx_useDiffVol=new QCheckBox("Use difference volume");


	m_radio_showVolumen=new QRadioButton("Volume");
	m_radio_showVolumen->setChecked(true);
	m_radio_showMeshes=new QRadioButton("Contours");
	m_chbx_showBar=new QCheckBox("Legend");
	m_chbx_showBar->setChecked(true);
	m_show_original			= new QCheckBox("Show Original (Left-Arrow)");
	m_show_registered		= new QCheckBox("Show Registered (Right-Arrow)");
	m_show_difference		= new QCheckBox("Show Difference (Down-Arrow)");

	QPushButton *but_calculateAllDiffVolumes=new QPushButton("Calculate all difference volumes");
	QPushButton *but_setPath=new QPushButton("Set Path");

	m_chbx_AdvancedMesh0=new QCheckBox("Show error 25");
	m_chbx_AdvancedMesh1=new QCheckBox("Show error 50");
	m_chbx_AdvancedMesh2=new QCheckBox("Show error 75");
	m_chbx_AdvancedMesh3=new QCheckBox("Show error 100");
	m_chbx_AdvancedMesh4=new QCheckBox("Show error 125");
	m_chbx_AdvancedMesh5=new QCheckBox("Show error 150");
	m_chbx_AdvancedMesh6=new QCheckBox("Show error 175");
	m_chbx_AdvancedMesh7=new QCheckBox("Show error 200");
	m_chbx_AdvancedMesh8=new QCheckBox("Show error 225");
	m_chbx_AdvancedMesh9=new QCheckBox("Show error 255");

	m_generateAdvancedDifference= new QPushButton("Generate advanced difference");
	m_advancedBox= new QGroupBox("Advanced difference");

	QVBoxLayout * advancedLayout= new QVBoxLayout();
	advancedLayout->addWidget(m_chbx_AdvancedMesh0);
	advancedLayout->addWidget(m_chbx_AdvancedMesh1);
	advancedLayout->addWidget(m_chbx_AdvancedMesh2);
	advancedLayout->addWidget(m_chbx_AdvancedMesh3);
	advancedLayout->addWidget(m_chbx_AdvancedMesh4);
	advancedLayout->addWidget(m_chbx_AdvancedMesh5);
	advancedLayout->addWidget(m_chbx_AdvancedMesh6);
	advancedLayout->addWidget(m_chbx_AdvancedMesh7);
	advancedLayout->addWidget(m_chbx_AdvancedMesh8);
	advancedLayout->addWidget(m_chbx_AdvancedMesh9);
	
	m_radio_showAllDifferenceErrors= new QRadioButton("All");
	m_radio_showNoneDifferenceErrors= new QRadioButton("None");
	m_radio_showNoneDifferenceErrors->setChecked(true);


	
	QHBoxLayout *allNoneLayout= new QHBoxLayout ();

	allNoneLayout->addWidget(m_radio_showAllDifferenceErrors);
	allNoneLayout->addWidget(m_radio_showNoneDifferenceErrors);

	advancedLayout->addLayout(allNoneLayout);

	m_advancedBox->setLayout(advancedLayout);

	QLabel *meshLabel= new QLabel("Isovalue (original / rigistered)");
	QLabel *errorLabel = new QLabel("Isovalue (difference)");
	
	QPushButton *but_error=new QPushButton("Update");
	QPushButton *but_meshIso=new QPushButton("Update");

	m_isoValueMeshes	= new QDoubleSpinBox();
	m_isoValueErrorMesh	= new QDoubleSpinBox();
	m_opacity_Original	= new QDoubleSpinBox();
	m_opacity_Registered= new QDoubleSpinBox();
	m_opacity_Difference= new QDoubleSpinBox();
	



	QLabel *opacityLabel = new QLabel("Original countour opacity");
	QPushButton * updateOriginalOpacity= new QPushButton("Update");

	QLabel *diff_opacityLabel = new QLabel("Difference countour opacity");
	QPushButton * updateDifferenceOpacity= new QPushButton("Update");

	QLabel *reg_opacityLabel = new QLabel("Registered countour opacity");
	QPushButton * updateRegisteredOpacity= new QPushButton("Update");

	
	propertiesGrid->addWidget(meshLabel,0,0);
	propertiesGrid->addWidget(m_isoValueMeshes,0,1);
	propertiesGrid->addWidget(but_meshIso,0,2);
	propertiesGrid->addWidget(errorLabel,1,0);
	propertiesGrid->addWidget(m_isoValueErrorMesh,1,1);
	propertiesGrid->addWidget(but_error,1,2);
	propertiesGrid->addWidget(opacityLabel,2,0);
	propertiesGrid->addWidget(m_opacity_Original,2,1);
	propertiesGrid->addWidget(updateOriginalOpacity,2,2);
	propertiesGrid->addWidget(diff_opacityLabel,3,0);
	propertiesGrid->addWidget(m_opacity_Difference,3,1);
	propertiesGrid->addWidget(updateDifferenceOpacity,3,2);
	propertiesGrid->addWidget(reg_opacityLabel,4,0);
	propertiesGrid->addWidget(m_opacity_Registered,4,1);
	propertiesGrid->addWidget(updateRegisteredOpacity,4,2);
	
	
	m_opacity_Original->setValue(1);
	m_opacity_Original->setMaximum(1);
	m_opacity_Original->setMinimum(0.1);
	m_opacity_Original->setSingleStep(0.1);

	
	m_opacity_Difference->setValue(1);
	m_opacity_Difference->setMaximum(1);
	m_opacity_Difference->setMinimum(0.1);
	m_opacity_Difference->setSingleStep(0.1);

	
	m_opacity_Registered->setValue(1);
	m_opacity_Registered->setMaximum(1);
	m_opacity_Registered->setMinimum(0.1);
	m_opacity_Registered->setSingleStep(0.1);
	
	m_isoValueMeshes->setValue(16);
	m_isoValueMeshes->setMaximum(255);
	m_isoValueMeshes->setMinimum(1);
	m_isoValueMeshes->setSingleStep(10);

	m_isoValueErrorMesh->setValue(16);
	m_isoValueErrorMesh->setMaximum(255);
	m_isoValueErrorMesh->setMinimum(1);
	m_isoValueErrorMesh->setSingleStep(10);


	// fill visualisation with Data 
	visualisationLayout->addWidget(m_radio_showMeshes);
	visualisationLayout->addWidget(m_radio_showVolumen);
	// todo space line 0o
	visualisationLayout->addWidget(m_show_original);
	visualisationLayout->addWidget(m_show_registered);
	visualisationLayout->addWidget(m_show_difference);
	// todo space line 0o
	visualisationLayout->addWidget(m_chbx_showBar);
	visualisationLayout->addWidget(m_advancedBox);
	visualisationBox->setLayout(visualisationLayout);
	
//	propertiesLayout->addWidget(but_setPath);
//  propertiesLayout->addWidget(m_chbx_useDiffVol);
	propertiesLayout->addLayout(propertiesGrid);
	
/*	propertiesLayout->addLayout(errorLayout);
	propertiesLayout->addLayout(opacityLayout);
	propertiesLayout->addLayout(registedLayout);
	propertiesLayout->addLayout(diffLayout);
*/


	propertiesBox->setLayout(propertiesLayout);

	// fill Calculation with Data
	calculationLayout->addWidget(m_chbx_useDiffVol);
	calculationLayout->addWidget(but_calculateAllDiffVolumes);
	calculationLayout->addWidget(m_generateAdvancedDifference);
	calculationBox->setLayout(calculationLayout);

	datasetLayout->addWidget(but_setPath);
	datasetLayout->addWidget(m_itemList);

	datasetBox->setLayout(datasetLayout);

	volumeLayout->addWidget(visualisationBox);
	volumeLayout->addWidget(datasetBox);
	volumeLayout->addWidget(propertiesBox);
	
	volumeLayout->addWidget(calculationBox);
	

	m_advancedBox->setEnabled(false);


	// generate the main layout (put it all together)
/*	volumeLayout->addWidget(but_setPath);
	volumeLayout->addWidget(m_chbx_useDiffVol);
	volumeLayout->addWidget(m_radio_showMeshes);
	volumeLayout->addWidget(m_radio_showVolumen);
	volumeLayout->addWidget(m_chbx_showBar);
	volumeLayout->addWidget(opacityLabel);
	volumeLayout->addLayout(opacityLayout);
	volumeLayout->addWidget(reg_opacityLabel);
	volumeLayout->addLayout(registedLayout);
	volumeLayout->addWidget(diff_opacityLabel);
	volumeLayout->addLayout(diffLayout);
	volumeLayout->addWidget(m_itemList);
	volumeLayout->addWidget(but_calculateAllDiffVolumes);
	volumeLayout->addLayout(meshLayout);
	volumeLayout->addLayout(errorLayout);
	volumeLayout->addWidget(advancedBox);
*/
	setLayout(volumeLayout);
	connect(m_itemList,SIGNAL(itemDoubleClicked(QListWidgetItem *)),this,SLOT(loadItem(QListWidgetItem *)));

	// checkbox connections
	connect(m_chbx_useDiffVol,SIGNAL(clicked()),this,SLOT(setUsedifferenceVol()));
	connect(m_radio_showMeshes,SIGNAL(clicked()),this,SLOT(setShowMeshes()));
	connect(m_radio_showVolumen,SIGNAL(clicked()),this,SLOT(setShowVolume()));
	connect(m_chbx_AdvancedMesh0,SIGNAL(clicked()),this,SLOT(showErrorMesh()));
	connect(m_chbx_AdvancedMesh1,SIGNAL(clicked()),this,SLOT(showErrorMesh()));
	connect(m_chbx_AdvancedMesh2,SIGNAL(clicked()),this,SLOT(showErrorMesh()));
	connect(m_chbx_AdvancedMesh3,SIGNAL(clicked()),this,SLOT(showErrorMesh()));
	connect(m_chbx_AdvancedMesh4,SIGNAL(clicked()),this,SLOT(showErrorMesh()));
	connect(m_chbx_AdvancedMesh5,SIGNAL(clicked()),this,SLOT(showErrorMesh()));
	connect(m_chbx_AdvancedMesh6,SIGNAL(clicked()),this,SLOT(showErrorMesh()));
	connect(m_chbx_AdvancedMesh7,SIGNAL(clicked()),this,SLOT(showErrorMesh()));
	connect(m_chbx_AdvancedMesh8,SIGNAL(clicked()),this,SLOT(showErrorMesh()));
	connect(m_chbx_AdvancedMesh9,SIGNAL(clicked()),this,SLOT(showErrorMesh()));
	connect(m_chbx_showBar,SIGNAL(clicked()),this,SLOT(showBar()));

	connect(m_show_difference,SIGNAL(clicked()),this,SLOT(showDifferenceMesh()));
	connect(m_show_original,SIGNAL(clicked()),this,SLOT(showOriginalMesh()));
	connect(m_show_registered,SIGNAL(clicked()),this,SLOT(showRegisteredMesh()));

	connect(m_radio_showNoneDifferenceErrors,SIGNAL(clicked()),this,SLOT(showNoneError()));
	connect(m_radio_showAllDifferenceErrors,SIGNAL(clicked()),this,SLOT(showAllError()));

	// buttons connection
	connect(but_setPath,SIGNAL(clicked()),this,SLOT(setPath()));
	connect(but_error,SIGNAL(clicked()),this,SLOT(setIsoValues()));
	connect(but_meshIso,SIGNAL(clicked()),this,SLOT(setIsoValues()));
	connect(updateOriginalOpacity,SIGNAL(clicked()),this,SLOT(showOriginalMesh()));
	connect(updateDifferenceOpacity,SIGNAL(clicked()),this,SLOT(showDifferenceMesh()));
	connect(updateRegisteredOpacity,SIGNAL(clicked()),this,SLOT(showRegisteredMesh()));
	connect(m_generateAdvancedDifference,SIGNAL(clicked()),this,SLOT(generateAdvancedDifference()));
	connect(but_calculateAllDiffVolumes,SIGNAL(clicked()),this,SLOT(calculateAllDifferenceVolumes()));

	connect(updateDifferenceOpacity,SIGNAL(clicked()),this,SLOT(showDifferenceMesh()));
	connect(updateOriginalOpacity,SIGNAL(clicked()),this,SLOT(showOriginalMesh()));
	connect(updateRegisteredOpacity,SIGNAL(clicked()),this,SLOT(showRegisteredMesh()));

	setExpertMode(false);

}
void VolumeControls::setCheckBoxes(bool o,bool r,bool d)
{
	m_show_original->setChecked(o);
	m_show_registered->setChecked(r);
	m_show_difference->setChecked(d);
	
}
void VolumeControls::setExpertMode(bool expert)
{
	if (expert)// show expert
	{
		m_advancedBox->show();
		m_generateAdvancedDifference->show();
	}
	else	// hide it all 
	{
		m_advancedBox->hide();
		m_generateAdvancedDifference->hide();
	}
}
void VolumeControls::showNoneError()
{
	m_chbx_AdvancedMesh0->setChecked(false);
	m_chbx_AdvancedMesh1->setChecked(false);
	m_chbx_AdvancedMesh2->setChecked(false);
	m_chbx_AdvancedMesh3->setChecked(false);
	m_chbx_AdvancedMesh4->setChecked(false);
	m_chbx_AdvancedMesh5->setChecked(false);
	m_chbx_AdvancedMesh6->setChecked(false);
	m_chbx_AdvancedMesh7->setChecked(false);
	m_chbx_AdvancedMesh8->setChecked(false);
	m_chbx_AdvancedMesh9->setChecked(false);
	showErrorMesh();
}
void VolumeControls::showAllError()
{
	emit StatusMessage("Generating visualization data....");
	m_chbx_AdvancedMesh0->setChecked(true);
	m_chbx_AdvancedMesh1->setChecked(true);
	m_chbx_AdvancedMesh2->setChecked(true);
	m_chbx_AdvancedMesh3->setChecked(true);
	m_chbx_AdvancedMesh4->setChecked(true);
	m_chbx_AdvancedMesh5->setChecked(true);
	m_chbx_AdvancedMesh6->setChecked(true);
	m_chbx_AdvancedMesh7->setChecked(true);
	m_chbx_AdvancedMesh8->setChecked(true);
	m_chbx_AdvancedMesh9->setChecked(true);
	showErrorMesh();
	emit StatusMessage("Ready");
}
void VolumeControls::generateAdvancedDifference()
{
	if (m_varVis->generateAdvancedDiffContour())
		m_advancedBox->setEnabled(true);
	
	else
		emit StatusMessage("No difference volume loaded -> cancel");

}
void VolumeControls::showOriginalMesh()
{
	if (m_radio_showMeshes->isChecked())
	{
		if (m_varVis->getAnalyseMeshActor().size()>0)
		{
			m_varVis->getAnalyseMeshActor().at(0)->GetProperty()->SetOpacity(m_opacity_Original->text().toDouble());
			m_varVis->getAnalyseMeshActor().at(0)->SetVisibility(m_show_original->isChecked());
			m_varVis->getRenderWindow()->Render();
			if (m_show_original->isChecked())
				m_varVis->setIndexOfVolume(0);
		}
	}
	if (m_radio_showVolumen->isChecked())
	{
		if (m_varVis->getAnalyseVolumen().size()>0)
		{
			m_varVis->getAnalyseVolumen().at(0)->SetVisibility(m_show_original->isChecked());
			m_varVis->getRenderWindow()->Render();
			if (m_show_original->isChecked())
				m_varVis->setIndexOfVolume(0);
		}
	}
}

void VolumeControls::showRegisteredMesh()
{
	if (m_radio_showMeshes->isChecked())
	{
		if (m_varVis->getAnalyseMeshActor().size()>0)
		{
			m_varVis->getAnalyseMeshActor().at(1)->GetProperty()->SetOpacity(m_opacity_Registered->text().toDouble());
			m_varVis->getAnalyseMeshActor().at(1)->SetVisibility(m_show_registered->isChecked());
			m_varVis->getRenderWindow()->Render();
			if (m_show_registered->isChecked())
				m_varVis->setIndexOfVolume(1);
		}
	}

	if (m_radio_showVolumen->isChecked())
	{
		if (m_varVis->getAnalyseVolumen().size()>0)
		{
			m_varVis->getAnalyseVolumen().at(1)->SetVisibility(m_show_registered->isChecked());
			m_varVis->getRenderWindow()->Render();
			if (m_show_registered->isChecked())
				m_varVis->setIndexOfVolume(1);
		}
	}
}

void VolumeControls::showDifferenceMesh()
{
	if (m_radio_showMeshes->isChecked())
	{
		if (m_useDifferenceVol && m_varVis->getAnalyseMeshActor().size()>2)
		{
			m_varVis->getAnalyseMeshActor().at(2)->GetProperty()->SetOpacity(m_opacity_Difference->text().toDouble());
			m_varVis->getAnalyseMeshActor().at(2)->SetVisibility(m_show_difference->isChecked());
			m_varVis->getRenderWindow()->Render();
			if (m_show_difference->isChecked())
				m_varVis->setIndexOfVolume(2);
		}
	}
	if (m_radio_showVolumen->isChecked())
	{
		if (m_useDifferenceVol && m_varVis->getAnalyseVolumen().size()>2)
		{
			m_varVis->getAnalyseVolumen().at(2)->SetVisibility(m_show_difference->isChecked());
			m_varVis->getRenderWindow()->Render();
			if (m_show_difference->isChecked())
				m_varVis->setIndexOfVolume(2);
		}
	}
}
void VolumeControls::showBar()
{
	m_varVis->showMagnitudeBar(m_chbx_showBar->isChecked());
	m_varVis->getRenderWindow()->Render();

}
void VolumeControls::showErrorMesh()
{
	
	m_varVis->setErrorMeshVisibility(0,m_chbx_AdvancedMesh0->isChecked());
	m_varVis->setErrorMeshVisibility(1,m_chbx_AdvancedMesh1->isChecked());
	m_varVis->setErrorMeshVisibility(2,m_chbx_AdvancedMesh2->isChecked());
	m_varVis->setErrorMeshVisibility(3,m_chbx_AdvancedMesh3->isChecked());
	m_varVis->setErrorMeshVisibility(4,m_chbx_AdvancedMesh4->isChecked());
	m_varVis->setErrorMeshVisibility(5,m_chbx_AdvancedMesh5->isChecked());
	m_varVis->setErrorMeshVisibility(6,m_chbx_AdvancedMesh6->isChecked());
	m_varVis->setErrorMeshVisibility(7,m_chbx_AdvancedMesh7->isChecked());
	m_varVis->setErrorMeshVisibility(8,m_chbx_AdvancedMesh8->isChecked());
	m_varVis->setErrorMeshVisibility(9,m_chbx_AdvancedMesh9->isChecked());
	m_varVis->getRenderWindow()->Render();
}

void VolumeControls::setIsoValues()
{
	if (m_varVis->getAnalyseMeshActor().size()!=0)
	{
		m_varVis->SetErrorIsoValue(m_isoValueErrorMesh->text().toDouble());
		m_varVis->SetMeshsIsoValue(m_isoValueMeshes->text().toDouble());
		loadItem(m_itemList->currentItem());
		setShowVolume();
	}
}
void VolumeControls::loadPath(QString path)
{
	// check if Folder Contains mhdFiles
	QDir subsetDir(path,"*.mhd");
	
	// clear the list if it is not empty
	if (m_itemList->count()>1)
	{
		m_subsetList.clear();
		m_itemList->clear();
	}

	// Search for All Files in this Folder
	m_subsetList=subsetDir.entryList();
	
	// generate ItemList Entry
	for (int iA=0;iA<m_subsetList.size();iA++)
	{
		QString tempItem= m_subsetList.at(iA);
		int helper= tempItem.indexOf("_",0);
		int helper2=tempItem.indexOf("_",helper+1);
		int pos=tempItem.indexOf("_",helper2+1);
		tempItem.remove(pos,tempItem.length());
		
		QList<QListWidgetItem*> tempList=m_itemList->findItems(tempItem,Qt::MatchExactly);
		
		if (tempList.size()==0)
			m_itemList->addItem(tempItem);
	}


}
void VolumeControls::setPath()
{
	QString path=QFileDialog::getExistingDirectory(this, tr("Select directory containing aligned and registered datasets for comparison."),
                            "",// FIXME: Use current SDMVisConfig base path here
							QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	// break if user has canceld the selection
	if (path.isEmpty())
		return;
	// check if Folder Contains mhdFiles
	QDir subsetDir(path,"*.mhd");
	int numOfMhdFiles=subsetDir.count();
	if (numOfMhdFiles==0)
		return;
	m_pathToVolumeData=path;

	loadPath(m_pathToVolumeData);
}

void VolumeControls::loadItem(QListWidgetItem *item)
{
	m_radio_showNoneDifferenceErrors->setChecked(true);
	m_advancedBox->setEnabled(false);
	showNoneError();
//	m_show_original->setChecked(true);
//	m_show_difference->setChecked(false);
//	m_show_registered->setChecked(false);

	const QString modes[4] = {"plain","abs","L2","L2squared"};
	const QString suffixDifference("_DIFFERENCE");

	int mode = 3; // FIXME: mode seems to have no effect in current diffvol version?!

	QString myName= item->text();
	QStringList listToLoad=m_subsetList.filter(myName);
	int size=listToLoad.size();
	bool loadDifferenceVolume=false;
	m_DiffVolCalculated=false;

	// clear the old names;
	difference.clear();
	original.clear();
	registed.clear();

	// find the order
	for (int iA=0;iA<size;iA++)
	{
		if (listToLoad.at(iA).endsWith(suffixDifference+".mhd"))
		{
			difference=m_pathToVolumeData+"/"+listToLoad.at(iA);
			continue;
		}
		if (listToLoad.at(iA).endsWith("registered.mhd"))
		{
			registed=m_pathToVolumeData+"/"+listToLoad.at(iA);
			continue;
		}
		if (listToLoad.at(iA).endsWith("aligned.mhd"))
		{
			original=m_pathToVolumeData+"/"+listToLoad.at(iA);
			continue;
		}
	}

	if (!difference.isEmpty())
	{
		// we do have a calculated Volume :)
		m_DiffVolCalculated=true;
	}

	if (difference.isEmpty() && m_useDifferenceVol)
	{// we wanna use differenceVol but we dont have one 
		
		// calculate? 
		if( QMessageBox::question( this, tr("VarVis: There is no DifferenceVolume Calculated"),
			tr("Do You Wannt To Calculate it"),
			QMessageBox::Yes | QMessageBox::No,  QMessageBox::Yes )
			== QMessageBox::Yes )
		{
			// Calculate it 
			difference=original;
			difference.remove(".mhd");
			difference.append(suffixDifference+".mhd");

			// now we have the three fileNames
			// we can use diffvol(orig, reg, diffc)
			QStringList args;
			args.push_back("-m" + modes[mode]);
			args.push_back(original);
			args.push_back(registed);
			args.push_back(difference);

			// let the proc begin to calculate the diffVol
			QProcess* process= new QProcess();
			process->start("diffvol",args);
			connect(process,SIGNAL(finished(int)),this,SLOT(proc_finished(int)));
	
		}
		else 
		{
			// you dont wanna calculate it , your bad!
		}
	}// decition done

	// now what to do? 
	if (!m_useDifferenceVol)
	{
		// just load original and registed 
		// call VarVisRenderer :: loadVolumen(original,registered)
		m_varVis->loadVolumen(original,registed);
	}
	if (m_useDifferenceVol && m_DiffVolCalculated)
	{
		// diffVol is calculated so load all Files
		m_varVis->loadVolumen(original,registed,difference);
	}

	showDifferenceMesh();
	showOriginalMesh();
	showRegisteredMesh();

}

void VolumeControls::calculateAllDifferenceVolumes()
{

	/*
		TODO : 

		find an other way for calculation

		we create in worstcase 29 Process 
		witch leads to a System Overhead

		Idea : save the process in a QVector<QProcess>
		and start the First 
		when he emits his readySignal then start 
		the next 
		>> linear execution of Processes 

		// problem when one Process does not have
		 exitValue=0 
			decide what to do 
			go on or break all Calculation

		// find out what cause the overhead
		 creating QProcess or the parallel execution
		 of this !


	
	
	
	*/
	int size=m_itemList->count();
	if (size==0)
	{
	  emit StatusMessage(tr("No Items Loaded -> cancel"));
	  return;
	}

	// do calculate all Volumes 
	// get Informations 
	const QString modes[4] = {"plain","abs","L2","L2squared"};
	sizeOfItems=0;
	sizeOfFinishedProcesses=0;
	const QString suffixDifference("_DIFFERENCE");
	int mode = 3; // FIXME: mode seems to have no effect in current diffvol version?!


	
	QString tmp_registed;
	QString tmp_original;
	QString tmp_difference;


	for (int iA=0;iA<m_itemList->count();iA++)
	{
		emit StatusMessage("Starting process for calculation of  difference volume "+
				QString::number(iA)+
				" / "+
				QString::number(m_itemList->count())			
			);
	
		tmp_registed.clear();
		tmp_original.clear();
		tmp_difference.clear();

		QString itemName=m_itemList->item(iA)->text();
		QStringList listToLoad=m_subsetList.filter(itemName);

		// find the order
		for (int iB=0;iB<listToLoad.size();iB++)
		{
			if (listToLoad.at(iB).endsWith(suffixDifference+".mhd"))
			{
				tmp_difference=m_pathToVolumeData+"/"+listToLoad.at(iB);
				continue;
			}
			if (listToLoad.at(iB).endsWith("registered.mhd"))
			{
				tmp_registed=m_pathToVolumeData+"/"+listToLoad.at(iB);
				continue;
			}
			if (listToLoad.at(iB).endsWith("aligned.mhd"))
			{
				tmp_original=m_pathToVolumeData+"/"+listToLoad.at(iB);
				continue;
			}
		}

		if (!tmp_difference.isEmpty())
		{
			// skip this process 
			emit StatusMessage("Volume " + QString::number(iA)+ " / "+ QString::number(m_itemList->count())+
				" already exists ->Skipp it ");

			continue;
		}
		// is it found? 
		if (!tmp_registed.isEmpty() && !tmp_original.isEmpty() )
		{
			//found
			//generete difference string 
			tmp_difference=tmp_original;
			tmp_difference.remove(".mhd");
			tmp_difference.append(suffixDifference+".mhd");
		
			// create Proc and start it 
			QStringList args;
			args.push_back("-m" + modes[mode]);
			args.push_back(tmp_original);
			args.push_back(tmp_registed);
			args.push_back(tmp_difference);

			// let the proc begin to calculate the diffVol
			QProcess* process= new QProcess();
			process->start("diffvol",args);
			connect(process,SIGNAL(finished(int)),this,SLOT(multiProcess_Finished(int)));
			sizeOfItems++;

		}
		else
		{
			return ;
		}


		
	
	}
	if (sizeOfItems==0)
		emit StatusMessage(tr("All difference volumes already Calculated"));
	else
		emit StatusMessage(tr("Waiting for all processes to complete"));
}

void VolumeControls::multiProcess_Finished(int exitValue)
{
	if (exitValue==0)
	{
		
		sizeOfFinishedProcesses++;
		emit StatusMessage("Process "+
			QString::number(sizeOfFinishedProcesses)+
			" / "+
			QString::number(sizeOfItems)+
			" complete"			
			);


		if (sizeOfFinishedProcesses==sizeOfItems)
		{
			loadPath(m_pathToVolumeData);
		}
	}
	else 
	{
		QMessageBox::warning(this,tr("Warning"),
			"Difference Volume NOT calculated! \n Maybe diffvol.exe not Found in Path or something else went wrong");
	}
}

void VolumeControls::proc_finished(int exitValue)
{
	if (exitValue==0)
	{
		m_varVis->loadVolumen(original,registed,difference);
		QFileInfo info(difference);
		QString filName=info.fileName();
		m_subsetList.append(info.fileName()); // add it to list so we dont need to calc it again 
		
	}
	else 
	{
		QMessageBox::warning(this,tr("Warning"),
			"Difference Volume NOT calculated! \n Maybe diffvol.exe not Found in Path or something else went wrong");

	}
}

void VolumeControls::setUsedifferenceVol()
{
	m_useDifferenceVol=m_chbx_useDiffVol->isChecked();
	int index= *m_varVis->getIndexOfVolume();
	if (!m_useDifferenceVol && index>1)
		*m_varVis->getIndexOfVolume()=0;
}

void VolumeControls::setShowVolume()
{
	//m_radio_showMeshes->setChecked(!m_radio_showVolumen->isChecked());
	m_varVis->setShowVolumeMeshes(m_radio_showMeshes->isChecked());
	showDifferenceMesh();
	showRegisteredMesh();
	showOriginalMesh();
}
void VolumeControls::setShowMeshes()
{
	//m_radio_showVolumen->setChecked(!m_radio_showMeshes->isChecked());
	m_varVis->setShowVolumeMeshes(m_radio_showMeshes->isChecked());
	showDifferenceMesh();
	showRegisteredMesh();
	showOriginalMesh();
}