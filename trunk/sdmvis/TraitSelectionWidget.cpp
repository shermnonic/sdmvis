#include "TraitSelectionWidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include "TraitComb.h"
#ifdef SDMVIS_MATLAB_ENABLED
#include "Matlab.h"
#endif
#include <fstream>
#include <vector>

#define GET_FILE_SIZE( size, fs ) \
	fs.seekg( 0, ios::end );      \
	size = (size_t)fs.tellg();    \
	fs.seekg( 0, ios::beg );


// Constructor
void TraitSelectionWidget::setSelectionBox(QComboBox* selectioBox)
{
//	m_selectComBox=selectioBox;
}

TraitSelectionWidget::TraitSelectionWidget(QWidget *parent)
{
	// init 
	traitIndex=0;
	m_defaultElementScaleValue=0.05;
	
	// Selection Group
	m_selectionGroup =new QGroupBox(tr("Select Trait"));

	QVBoxLayout *selectionLayout        = new QVBoxLayout;
	QVBoxLayout *modifiedSelectionLayout= new QVBoxLayout;
	QHBoxLayout *statusLayout           = new QHBoxLayout;
	QHBoxLayout *selectionButtonsLayout = new QHBoxLayout;
	QVBoxLayout *spacerLayout           = new QVBoxLayout;
	
	m_selectComBox= new QComboBox();
	m_butCreate	= new QPushButton(tr("Create"));
	m_butAdd    = new QPushButton(tr("Add"));
	m_butRemove = new QPushButton(tr("Remove"));

	selectionButtonsLayout->addWidget(m_butCreate);
	selectionButtonsLayout->addWidget(m_butAdd);
	selectionButtonsLayout->addWidget(m_butRemove);

	selectionLayout->addWidget(m_selectComBox);
	selectionLayout->addLayout(selectionButtonsLayout);

#ifdef SDMVIS_VARVIS_ENABLED
	m_pushTraitToVarVis= new QCheckBox(tr("Push trait to VarVis"));
	modifiedSelectionLayout->addLayout(selectionLayout);
	//modifiedSelectionLayout->addWidget(m_pushTraitToVarVis);
	m_selectionGroup->setLayout(modifiedSelectionLayout); 

	connect(m_pushTraitToVarVis,SIGNAL(clicked()),this,SLOT(clearVarVis()));
#else
	m_selectionGroup->setLayout(selectionLayout);
#endif

	// Trait Properties
	QPushButton *loadNamesButton	= new QPushButton(tr("Load Names..."));
	QPushButton *loadLabelsButton	= new QPushButton(tr("Load Labels..."));
	QPushButton *butOpenMatrix		= new QPushButton(tr("Open eigenvector matrix..."));
	QPushButton *butDefaultWeights	= new QPushButton(tr("Default weights"));
	
	butComputeTrait	= new QPushButton(tr("Compute trait..."));
	butComputeWarp	= new QPushButton(tr("Compute warpfield..."));
	butReloadTrait  = new QPushButton(tr("Reload Trait"));

	butReloadTrait->setDisabled(true);

	m_identify         = new QLineEdit();
	m_description	   = new QTextEdit();
	m_ncompSpinBox     = new QSpinBox;
	QLabel* ncompLabel = new QLabel(tr("Number of components"));
	
	m_ncompSpinBox->setMinimum( 1 );
	m_ncompSpinBox->setMaximum( 10 ); // depends on number of names
	m_ncompSpinBox->setSingleStep( 1 );
	
	ncompLabel->setBuddy( m_ncompSpinBox );

	QHBoxLayout* lncomp = new QHBoxLayout;
	lncomp->addWidget( ncompLabel );
	lncomp->addWidget( m_ncompSpinBox );

	// weights C,w1,w2	
	m_weightC      = new QDoubleSpinBox;
	m_weightW1     = new QDoubleSpinBox;
	m_weightW2     = new QDoubleSpinBox;
	m_elementScale = new QDoubleSpinBox;

	m_elementScale->setRange( 0.0, 10.0 );
	m_elementScale->setSingleStep( 0.1 );
	m_elementScale->setValue( m_defaultElementScaleValue );

	QLabel* weightCLabel  = new QLabel(tr("Weight C:"));
	QLabel* weightW1Label = new QLabel(tr("Class A weight w1:"));
	QLabel* weightW2Label = new QLabel(tr("Class B weight w2:"));
	
	QDoubleSpinBox* weights[3] = { m_weightC, m_weightW1, m_weightW2 };
	QLabel* weightLabels[3] = { weightCLabel, weightW1Label, weightW2Label };
	
	QGridLayout* lweights = new QGridLayout;
	for( int i=0; i < 3; ++i ) {
		weights[i]->setRange( 0.0, 1000.0 );
		weights[i]->setSingleStep( 0.1 );
		weights[i]->setValue( 1.0 );
		weightLabels[i]->setBuddy( weights[i] );
		lweights->addWidget( weightLabels[i], i,0 );
		lweights->addWidget( weights[i], i,1 );
	}
	
	lweights->addWidget( butDefaultWeights );

	// scale dimension w/ eigenvalues?
	m_lambdaScaling = new QCheckBox(tr("Lambda scaling"));
	m_lambdaScaling->setChecked( false );
	
	// additional options (scale, number of components)
	QVBoxLayout* lopt = new QVBoxLayout;
	lopt->addLayout( lncomp );
	lopt->addWidget( m_lambdaScaling );

	QGroupBox* weightGroup = new QGroupBox(tr("C-SVM weights"));
	weightGroup->setLayout( lweights );

	QHBoxLayout* scaleLayout = new QHBoxLayout;
	QLabel *scaleLabel= new QLabel(tr("Warp:"));

	scaleLayout->addWidget(scaleLabel);
	scaleLayout->addWidget(m_elementScale);

	QGroupBox* scaleGroup = new QGroupBox(tr("Warpfield scaling factor"));
	scaleGroup->setLayout( scaleLayout );

	QGroupBox* optGroup = new QGroupBox(tr("Options"));
	optGroup->setLayout( lopt );
	
	QLabel *spacerLabel= new QLabel("");
	spacerLayout->addWidget(spacerLabel);
	spacerLayout->addWidget(spacerLabel);
	
	m_treeWidget=new QTreeWidget();
	QStringList headerList;
	headerList<<tr("Name")<<tr("Group");
	m_treeWidget->setHeaderLabels(headerList);

    QHeaderView *header = m_treeWidget->header();
    header->setStretchLastSection(false);
    header->setResizeMode(0, QHeaderView::Stretch);
    header->setResizeMode(1, QHeaderView::ResizeToContents);

	m_traitStatus= new QLabel("Trait : ");
	m_warpStatus = new QLabel("Warp : ");

	QLabel *idLabel= new QLabel(tr("Identifier")); 
	QLabel *desLabel= new QLabel(tr("Description"));

	// --- new tabbed layout ---
	QVBoxLayout *idLayout = new QVBoxLayout;
	idLayout->addWidget(idLabel);
	idLayout->addWidget(m_identify);
	idLayout->addWidget(desLabel);
	idLayout->addWidget(m_description);

	QVBoxLayout *treeLayout = new QVBoxLayout;
	treeLayout->addWidget(m_treeWidget);
	treeLayout->addWidget(loadLabelsButton);

	QVBoxLayout *optLayout  = new QVBoxLayout;
	optLayout->addWidget(optGroup);
	optLayout->addWidget(weightGroup);
	optLayout->addWidget(scaleGroup);
	optLayout->addWidget(butComputeTrait);
	optLayout->addWidget(butComputeWarp);
	optLayout->addWidget(butReloadTrait);	
	optLayout->addStretch(16);

	statusLayout->addWidget(m_traitStatus);
	statusLayout->addWidget(m_warpStatus);

	QWidget* tabInfo = new QWidget;
	tabInfo->setLayout( idLayout );

	QWidget* tabLabels = new QWidget;
	tabLabels->setLayout( treeLayout );

	QWidget* tabSVM = new QWidget;
	tabSVM->setLayout( optLayout );

	QTabWidget* tabs = new QTabWidget();
	tabs->addTab( tabInfo,   tr("Info") );
	tabs->addTab( tabLabels, tr("Labels") );
	tabs->addTab( tabSVM,    tr("SVM") );
	m_optionTab = tabs;

	QVBoxLayout * mainLayout= new QVBoxLayout;
	mainLayout->addWidget(m_selectionGroup);
	mainLayout->addWidget(tabs);
	mainLayout->addLayout(statusLayout);
	mainLayout->setContentsMargins(1,1,1,1);

	setLayout(mainLayout);

	// set up connections
	connect(m_selectComBox   ,SIGNAL(activated(int)),		this, SLOT(setIndex(int)));
	connect(loadNamesButton  ,SIGNAL(clicked()),			this, SLOT(openNames()));
	connect(loadLabelsButton ,SIGNAL(clicked()),			this, SLOT(openLabels()));
	connect(butComputeTrait  ,SIGNAL(clicked()),			this, SLOT(computeTrait()) );
	connect(butComputeWarp   ,SIGNAL(clicked()),			this, SLOT(computeWarp()) );
	connect(butDefaultWeights,SIGNAL(clicked()),			this, SLOT(defaultWeights()) );
	connect(m_butAdd		 ,SIGNAL(clicked()),			this, SLOT(addExistTrait()));
	connect(m_butCreate		 ,SIGNAL(clicked()),			this, SLOT(addNewTrait()));
	connect(m_butRemove		 ,SIGNAL(clicked()),			this, SLOT(removeTraitFromCFG()));
	connect(butReloadTrait	 ,SIGNAL(clicked()),			this, SLOT(reloadTrait()));
	connect(m_elementScale	 ,SIGNAL(valueChanged(double)),	this, SLOT(scaleValueChanged(double)));
	
	connect(m_elementScale	,SIGNAL(valueChanged(double)),this, SLOT(verifyValidation()));
	connect(m_ncompSpinBox	,SIGNAL(valueChanged(int)),this, SLOT(verifyValidation()));
	connect(m_weightC		,SIGNAL(valueChanged(double)),this, SLOT(verifyValidation()));
	connect(m_weightW1		,SIGNAL(valueChanged(double)),this, SLOT(verifyValidation()));
	connect(m_weightW2		,SIGNAL(valueChanged(double)),this, SLOT(verifyValidation()));

	butComputeWarp->setDisabled(true);
}

#ifdef SDMVIS_VARVIS_ENABLED
void TraitSelectionWidget::pushTraitToVarVis(bool value)
{
	m_pushTraitToVarVis->blockSignals(true);
	m_pushTraitToVarVis->setChecked(value);
	m_pushTraitToVarVis->blockSignals(false);
	clearVarVis();

}

void TraitSelectionWidget::clearVarVis()
{	
	if (m_pushTraitToVarVis->isChecked())
	{
		emit setGetTraitsFromSDMVIS(true);
		emit loadNewTrait(m_selectComBox->currentIndex());
	}
	else
	{
		emit clearVarVisBox();
		emit setGetTraitsFromSDMVIS(false);	
	}
}
#endif // SDMVIS_VARVIS_ENABLED

void TraitSelectionWidget::setValidation(bool valid)
{
	if (valid)
	{
		m_traitStatus->setText("Trait :  Valid");
		if (m_traits.at(m_selectComBox->currentIndex()).valid)
		{
			m_warpStatus->setText("Warp : Valid");
			butComputeTrait->setDisabled(true);
			butComputeWarp->setDisabled(true);
		}
		else
		{
			m_warpStatus->setText("Warp : Invalid");
			butComputeTrait->setDisabled(true);
			butComputeWarp->setDisabled(false);		
		}		
	}
	else
	{
		m_traitStatus->setText("Trait :  Invalid");
		m_warpStatus->setText("Warp : Invalid");
		butComputeTrait->setEnabled(true);
		butComputeWarp->setDisabled(true);
	}
}

void TraitSelectionWidget::verifyValidation()
{
	if (!m_traits.isEmpty())
	{
		setValidation(false);
		int traitsSize=m_traits.size()-1;
		int index=m_selectComBox->currentIndex();
		bool valid=true;
		if (index>=0 && index<=traitsSize)
		{
			if (m_traits.at(index).numOfComp!=m_ncompSpinBox->text().toInt())
				valid=false;
		
			if (m_traits.at(index).weightC!=m_weightC->text().toDouble())
				valid=false;

			if (m_traits.at(index).weight1!=m_weightW1->text().toDouble())
				valid=false;

			if (m_traits.at(index).weight2!=m_weightW2->text().toDouble())
				valid=false;

			double realElementScale=m_traits.at(index).elementScale;
			double changedElementScale=m_elementScale->text().toDouble();	

			for(int i=0;i<m_traits.at(index).labels.size();i++)
			{
				//compare the labels
				int realValue= m_traits.at(index).labels[i];
				int changedValue=m_classVector.at(i);
				if (changedValue==0)
					changedValue=-1;

				if (realValue!=changedValue)
					valid=false;			
			}
		}
		else
		{
			butComputeTrait->setEnabled(true);
			return;
		}
		setValidation(valid);	
	}
}

// Functions 
void TraitSelectionWidget::textChanged()
{
// set the Text of the current Trait Index
	if (!m_traits.isEmpty())
	{
		int traitsSize=m_traits.size()-1;
		int index=m_selectComBox->currentIndex();
		if (index>=0 && index<=traitsSize)
			m_traits[m_selectComBox->currentIndex()].description=m_description->toPlainText();	
	}
}

void TraitSelectionWidget::setTraitList(QList<Trait> traits)
{
	// genereate combobox elements
	m_selectComBox->clear();
	m_barValues.resize(traits.size());
	m_validTrait.resize(traits.size());
	m_validWarp.resize(traits.size());
	m_fromCfg.resize(traits.size());
	m_edited.resize(traits.size());

	m_scaleValues.resize(traits.size());
	m_traits=traits;
	
	for (int iA=0;iA<traits.size();iA++)
	{
		m_selectComBox->addItem(traits.at(iA).filename);
		m_scaleValues[iA]=traits.at(iA).elementScale;
		if (traits.at(iA).mhdFilename!="empty")
			m_fromCfg[iA]=true;
		else
			m_fromCfg[iA]=false;
	
		// connect each trait with m_description
		connect(m_description,SIGNAL(textChanged()),this,SLOT(textChanged()));
	}
//	emit setSpecificTrait(0); // set on first trait 
}

void TraitSelectionWidget::clearAllValues()
{
	m_identify->clear();
	m_description->clear();
	m_weightC->setValue(1.0);
	m_weightW1->setValue(1.0);
	m_weightW2->setValue(1.0);
	m_elementScale->setValue(m_defaultElementScaleValue);

	for (int iA=0;iA<m_classVector.size();iA++)
		m_classVector.replace(iA,0);

	setLabels();
}

void TraitSelectionWidget::setNames(QStringList names)
{
	m_names = names;
	m_ncompSpinBox->setMaximum(m_names.size());
	m_ncompSpinBox->setValue(m_names.size());
	m_treeWidget->clear();
	m_combList.clear();
	m_classVector.clear();
	for (int iA=0;iA<m_names.size();iA++)
	{
		 QTreeWidgetItem *tmpItem= new QTreeWidgetItem();
		 tmpItem->setText(0,m_names.at(iA));
		 m_treeWidget->addTopLevelItem(tmpItem);
		 TraitComb *aBox=new TraitComb(iA);
		 aBox->addItem("Class A");
		 aBox->addItem("Class B");
		 m_combList.push_back(aBox);
		 m_classVector.push_back(0); // not init values
		 m_treeWidget->setItemWidget ( tmpItem, 1, aBox);
	     connect(m_combList.at(iA),SIGNAL(activated(int)),this,SLOT(setValue(int)));
	}
}

void TraitSelectionWidget::openMatrix(QString filename)
{
	using namespace std;
	
	if( filename.isEmpty() )
		return;

	// open matrix
	ifstream f( filename.toAscii(), ios::binary );
	if( !f.is_open() ) {
		QMessageBox::warning( this, tr("sdmvis: Error opening matrix"),
			tr("Could not open %1").arg(filename) );
		return;
	}

	// guess dimensionality from number of names
	// (must have number of names columns and element type is float32)
	size_t size; GET_FILE_SIZE(size,f);
	size_t elementsize = 4; // float32 = 4 byte
	int nrows = m_classVector.size(),
	    ncols = (size/elementsize) / nrows;
	if( (size/elementsize) % nrows != 0 ) {
		QMessageBox::warning( this, tr("sdmvis: Error matrix size mismatch"),
			tr("Matrix file hase %1 elements which does not match the number of"
			   "names %2!").arg(size/elementsize).arg(nrows) );
		f.close();
		return;
	}

	// load matrix
	float* buf = new float[nrows*ncols];
	f.read( (char*)buf, size );
	f.close();

	m_V.resize( nrows, ncols );
	rednum::matrix_from_rawbuffer<float,Matrix,double>( m_V, buf );

	cout << "Eigenvector matrix " << nrows << " x " << ncols << " set" << endl;
	delete [] buf;
}

void TraitSelectionWidget::loadMatrix(Matrix * eigenVectorMatrix)
{
	m_V=*eigenVectorMatrix;
}


void TraitSelectionWidget::setFirstValidation(){
	for (int iA=0;iA<m_traits.size();iA++){
		m_validWarp[iA]=true;
		m_validTrait[iA]=true;
	}
		// validation of trait and warpfield
		m_traitStatus->setText("Trait :  Valid");
		m_warpStatus->setText("Warp : Valid");
		
}
void TraitSelectionWidget::enable_TraitProperties(bool yes)
{
	m_optionTab->setEnabled(yes);
#ifdef SDMVIS_VARVIS_ENABLED
	m_pushTraitToVarVis->setEnabled(yes);
#endif
}
void TraitSelectionWidget::enable_selectionProperties(bool yes)
{
	m_selectionGroup->setEnabled(yes);
}



// Slots 
void TraitSelectionWidget::removeTraitFromCFG()
{
	// remove item form combo box
	int index=m_selectComBox->currentIndex();
	m_traits.removeAt(index);
	m_selectComBox->removeItem(index);
	index=m_selectComBox->currentIndex();

	m_butAdd->setEnabled(true);
	m_butCreate->setEnabled(true);
	m_selectComBox->setEnabled(true);

	//debug 
	int debug = m_traits.size();
	// load the trait before
	{
		// get size of comboBox
		int size = m_selectComBox->count();
		if (size!=0)
		{
			emit updateTraitList(m_traits,index);
			emit setIndex(m_selectComBox->currentIndex());
		}
		else
		{
			// there is no trait to load 
			clearAllValues();
			enable_TraitProperties(false); 
			emit unloadWarpfield();
			emit updateTraitList(m_traits,m_selectComBox->currentIndex());
		}
	}
}

void TraitSelectionWidget::reloadTrait()
{
	Trait t;
	t.readTrait(m_twfFileName);
	emit newTraitInTown(t,m_selectComBox->currentIndex());
	  
	m_validTrait[m_selectComBox->currentIndex()]=true;
	m_validWarp[m_selectComBox->currentIndex()]=true;
	
	// validation of trait and warpfield
	verifyValidation();
}

void TraitSelectionWidget::addNewTrait()
{
	int pos = m_selectComBox->count(); // add new trait at last position
	createNewTrait(pos);
}

void TraitSelectionWidget::addExistTrait()
{	
	QString filename = QFileDialog::getOpenFileName( this, tr("Open trait configuration file..."),
		m_baseDir, tr("Trait configuration file (*.twf)") );

	if ( filename.isNull() == false )
	{	
		m_validTrait.push_back(true);
		m_validWarp.push_back(true);
		m_fromCfg.push_back(true);
		m_edited.push_back(false);
		m_scaleValues.push_back(m_defaultElementScaleValue);
		
		Trait tempTrait;
		tempTrait.readTrait(filename);
		QFileInfo info(filename);
		QString tempName=tempTrait.mhdFilename;
		
		int index=tempName.indexOf("WARP_");
		tempName.remove(0,index);

		tempTrait.fullPathFileName=info.absolutePath()+"/"+tempName;
		emit newTraitInTown(tempTrait,m_selectComBox->count());
		enable_TraitProperties(true);
	}
}
#ifdef SDMVIS_VARVIS_ENABLED
bool TraitSelectionWidget::getPushToVarVis()
{
	return m_pushTraitToVarVis->isChecked();
}
#endif
void TraitSelectionWidget::scaleValueChanged(double value)
{
	
	if (!m_traits.isEmpty())
	{
		int traitsSize=m_traits.size()-1;
		int index=m_selectComBox->currentIndex();
		if (index>=0 && index<=traitsSize)
		{
			m_traits[m_selectComBox->currentIndex()].elementScale=m_elementScale->text().toDouble();
			emit setTraitScale(m_selectComBox->currentIndex(), value);
			m_scaleValues[m_selectComBox->currentIndex()]=value;
		}
	}
}

void TraitSelectionWidget::computeWarp()
{	// tell the main window to compute the traitWarpField

	if (m_fromCfg[m_selectComBox->currentIndex()])
	{
		if( QMessageBox::question( this, tr("sdmvis: Compute trait warpfield"),
		tr("You are going to overwrite a valid trait warpfield, are you sure?"),
		QMessageBox::Yes | QMessageBox::No,  QMessageBox::Yes )
		== QMessageBox::Yes )
		{
			emit computeWarpField(m_mhdFilename,m_matFilename); 
			// absolute path
		}
	}
	else
	{	if (m_mhdRelFilename=="empty")
		{
			// find out the relativ Part
			QString relPath=m_twpRelFilename;
			QFileInfo info(m_twfFileName);
		
			QString name = info.fileName();
			relPath.remove(name);
			QString absPath=m_twfFileName;
			absPath.remove(name);
			name.remove(".twf");
			
			//WARP_filename.mhd file
			m_mhdRelFilename=(relPath+"WARP_"+name+".mhd");
			m_mhdFilename=(absPath+"WARP_"+name+".mhd");
			//MAT_filename
			m_matRelFilename=(relPath+"MAT_"+name+"-w.mat");
			m_matFilename=(absPath+"MAT_"+name+"-w.mat");

			// FIXME: use always "-w" trait? or only for eigenwarp-based recon?
		}
		emit computeWarpField(m_mhdFilename,m_matFilename);
	}
}


void TraitSelectionWidget::warpfieldGenerated()
{
	m_selectComBox->setEnabled(true);
	m_selectComBox->setToolTip("");
	m_butAdd->setEnabled(true);
	m_butAdd->setToolTip("");
	m_butCreate->setEnabled(true);
	m_butCreate->setToolTip("");
	m_wholeTrait.valid=true;
	m_wholeTrait.mhdFilename=m_mhdRelFilename;
	
	m_wholeTrait.writeTrait(m_twfFileName);
	m_validWarp[m_selectComBox->currentIndex()]=true;
	m_fromCfg[m_selectComBox->currentIndex()]=true;
	m_edited[m_selectComBox->currentIndex()]=false;
	emit newTraitInTown(m_wholeTrait,m_selectComBox->currentIndex());
	emit loadNewTrait(m_selectComBox->currentIndex());
}

void TraitSelectionWidget::setValue(int index)
{
	TraitComb * who=(TraitComb*)sender(); // dirty cast
	int whoIndex= who->getIndex();
	m_classVector[whoIndex]=index;

	m_validTrait[m_selectComBox->currentIndex()]=false;
	m_validWarp[m_selectComBox->currentIndex()]=false;

	// validation of trait and warpfield
	verifyValidation();

	m_edited[m_selectComBox->currentIndex()]=true;
	int count=m_traits.count()-1;
	int id= m_selectComBox->currentIndex();
	if (!m_traits.empty()&& (m_traits.count()-1)>=m_selectComBox->currentIndex())
		m_traits[m_selectComBox->currentIndex()].setInvalid();
	if(m_fromCfg[m_selectComBox->currentIndex()])
	{
		butReloadTrait->setEnabled(true);
		QString oldName=m_selectComBox->currentText();
		if (!oldName.contains("*"))
			oldName.append("*");
		m_selectComBox->setItemText(m_selectComBox->currentIndex(),oldName);
	}
}
void TraitSelectionWidget::openNames()
{
	QString filename = QFileDialog::getOpenFileName( this, tr("Open names..."),
		m_baseDir, tr("Textfile (*.txt)") );

	if( filename.isEmpty() )
		return;

	QFile myFile(filename); // Create a file handle for the file named
	QString line;
	QStringList names;
	
	if (!myFile.open(QIODevice::ReadOnly)) // Open the file
	{
		// TODO: handle error		
	}

	QTextStream stream( &myFile ); // Set the stream to read from myFile
	 while ( !stream.atEnd() ) {           
		line = stream.readLine();   
		names<<line;
	 }
	myFile.close();
	setNames( names );
}

void TraitSelectionWidget::openLabels()
{
	QString filename = QFileDialog::getOpenFileName( this, tr("Open Labels..."),
			m_baseDir, tr("Textfile (*.txt)") );

	if( filename.isEmpty() )
		return;

	QFile myFile(filename); // Create a file handle for the file named
	QString line;
	QStringList names;
	
	if (!myFile.open(QIODevice::ReadOnly)) // Open the file
	{
		// TODO: handle error		
	}

	int lineCount=0;
	QTextStream stream( &myFile ); // Set the stream to read from myFile
	 while ( !stream.atEnd() ) {           
		line = stream.readLine();   
		m_classVector[lineCount]=line.toInt();
		lineCount++;
	 }
	myFile.close();

	setLabels();
	butComputeTrait->setEnabled(true);
	if(m_fromCfg[m_selectComBox->currentIndex()])
		butReloadTrait->setEnabled(true);
}

void TraitSelectionWidget::setLabels()
{
	int tmpValue;
	int cVsize=m_classVector.size();
	for (int iA=0;iA<m_classVector.size();iA++)
	{
		tmpValue=m_classVector.at(iA);
		m_combList.at(iA)->setCurrentIndex(tmpValue);
	}
}

void TraitSelectionWidget::clearTrait()
{
	m_selectComBox->clear();
	m_validTrait.clear();
	m_validWarp.clear();
	m_fromCfg.clear();
	m_edited.clear();
	m_identify->clear();
	m_elementScale->clear();
	m_description->clear();
	m_traits.clear();
	m_weightC->setValue(1.0);
	m_weightW1->setValue(1.0);
	m_weightW2->setValue(1.0);

	butComputeTrait->setEnabled(false);
}

void TraitSelectionWidget::setIndex(int index)
{
	m_selectComBox->blockSignals(true);
	m_selectComBox->setCurrentIndex(index);
	traitIndex=index;
	m_identify->setText(m_traits.at(index).identifier);
	m_description->setText(m_traits.at(index).description);

	// path stuff
	m_twpRelFilename=m_traits.at(index).filename;
	m_mhdRelFilename=m_traits.at(index).mhdFilename;
	m_matRelFilename=m_traits.at(index).matFilename;

	// create abs file path
	m_twfFileName=m_baseDir+"/"+m_twpRelFilename;
	m_mhdFilename=m_baseDir+"/"+m_mhdRelFilename;
	m_matFilename=m_baseDir+"/"+m_matRelFilename;

	for (unsigned int iA=0;iA<m_traits.at(index).labels.size();iA++)
	{
		int tmpValue=m_traits.at(index).labels[iA];	
		if (tmpValue==-1)
			tmpValue=0;
		m_classVector[iA]=tmpValue;
	}

	m_ncompSpinBox->setValue(m_traits.at(index).numOfComp);
	m_weightC->setValue(m_traits.at(index).weightC);
	m_weightW1->setValue(m_traits.at(index).weight1);
	m_weightW2->setValue(m_traits.at(index).weight2);
	m_elementScale->setValue(m_scaleValues.at(index));
	setLabels();

	// validation of trait and warpfield
	verifyValidation();

	emit(loadBarValue(0,m_barValues.at(index)));
	emit loadNewTrait(index);
	emit setTraitScale(m_selectComBox->currentIndex(), m_elementScale->text().toDouble());
	m_selectComBox->blockSignals(false);
}

void TraitSelectionWidget::setSpecificTrait(int index)
{
	// warpfield ist geladen , lade alles andere 
	m_selectComBox->blockSignals(true);
	m_selectComBox->setCurrentIndex(index);
	m_selectComBox->blockSignals(false);
	traitIndex=index;
	m_identify->setText(m_traits.at(index).identifier);
	m_description->setText(m_traits.at(index).description);
	m_elementScale->setValue(m_traits.at(index).elementScale);

	m_twpRelFilename=m_traits.at(index).filename;
	m_mhdRelFilename=m_traits.at(index).mhdFilename;
	m_matRelFilename=m_traits.at(index).matFilename;

	// create abs file path
	m_twfFileName=m_baseDir+"/"+m_twpRelFilename;
	m_mhdFilename=m_baseDir+"/"+m_mhdRelFilename;
	m_matFilename=m_baseDir+"/"+m_matRelFilename;

	for (unsigned int iA=0;iA<m_traits.at(index).labels.size();iA++)
	{
		int tmpValue=m_traits.at(index).labels[iA];	
		if (tmpValue==-1)
			tmpValue=0;
		m_classVector[iA]=tmpValue;
	}
		
	m_ncompSpinBox->setValue(m_traits.at(index).numOfComp);
	m_weightC->setValue(m_traits.at(index).weightC);
	m_weightW1->setValue(m_traits.at(index).weight1);
	m_weightW2->setValue(m_traits.at(index).weight2);

	m_elementScale->setValue(m_scaleValues.at(index));
	setLabels();
	QString debugString=m_traits.at(index).mhdFilename;
	verifyValidation();

	if (m_traits.at(index).mhdFilename!="empty")
		emit(loadBarValue(0,m_barValues.at(index)));
}

QString TraitSelectionWidget::getRelativePath( QString path, QString relativeTo ) const
{
	if( relativeTo.isEmpty() )
		relativeTo = m_baseDir;
	QDir baseDir( relativeTo );
	return baseDir.relativeFilePath( path );
}

void TraitSelectionWidget::createNewTrait(int index)
{
	QString PathToTrait=m_baseDir + "/"+m_configName;
	if (!QDir(PathToTrait).exists())
		QDir().mkdir(PathToTrait);

	QString filename = QFileDialog::getSaveFileName(this, //getSaveFileName
		  tr("Select File"), PathToTrait, "*.twf"); // twf = trait warpfield 		
	
	if ( filename.isNull() == false )
	{
		QFileInfo fileInfo(filename);

		if (fileInfo.fileName().contains(" "))
		{
		QMessageBox::warning(this,tr("Warning"),tr("Warning : Trait name contains spaces. \n This is not allowed. \n CANCEL!"));
		return;
		}

		enable_TraitProperties(true);
		// save filename variables 
		m_twfFileName=filename;
		m_twpRelFilename=getRelativePath(filename,m_baseDir);
		butReloadTrait->setDisabled(true);

		// find out the relativ Part
		QString relPath=m_twpRelFilename;
		QFileInfo info(filename);
		QString name = info.fileName();
		relPath.remove(name);
		QString absPath=filename;
		absPath.remove(name);
		name.remove(".twf");
		
		//WARP_filename.mhd file
		m_mhdRelFilename=(relPath+"WARP_"+name+".mhd");
		m_mhdFilename=(absPath+"WARP_"+name+".mhd");
		//MAT_filename
		m_matRelFilename=(relPath+"MAT_"+name+"-w.mat");
		m_matFilename=(absPath+"MAT_"+name+"-w.mat");

		// load default values 
		m_identify->clear();
		m_description->clear();
		m_validTrait.push_back(false);
		m_validWarp.push_back(false);
		m_fromCfg.push_back(false);
		m_edited.push_back(false);
		m_scaleValues.push_back(m_defaultElementScaleValue);
		clearAllValues();
	
		m_weightC->setValue(1.0);
		m_weightW1->setValue(1.0);
		m_weightW2->setValue(1.0);
		m_traitStatus->setText("Trait :  INVALID");
		m_warpStatus->setText("Warp : INVALID");
   		m_elementScale->setValue(m_defaultElementScaleValue);

		m_selectComBox->addItem(m_twpRelFilename);
		m_selectComBox->blockSignals(true);
		m_selectComBox->setCurrentIndex(index);
		m_selectComBox->blockSignals(false);
		m_selectComBox->setDisabled(true);
		m_selectComBox->setToolTip("Calculate the Trait and Warpfield first!");
		m_butAdd->setDisabled(true);
		m_butAdd->setToolTip("Calculate the Trait and Warpfield first!");
		m_butCreate->setDisabled(true);
		m_butCreate->setToolTip("Calculate the Trait and Warpfield first!");
		butReloadTrait->setDisabled(true);
		butReloadTrait->setToolTip("Calculate the Trait and Warpfield first!");
		butComputeTrait->setEnabled(true);
		butComputeWarp->setEnabled(false);
		m_butRemove->setDisabled(true);

		m_selectComBox->blockSignals(true);
		m_selectComBox->setCurrentIndex(m_selectComBox->count()-1);
		m_selectComBox->blockSignals(false);
		unloadWarpfield();
	}
}

void TraitSelectionWidget::sanityCheck()
{
	for (int iA=0;iA<m_selectComBox->count();iA++)
	{
		if (m_edited.at(iA))
		{			
			butReloadTrait->setEnabled(true);
			QString oldName=m_selectComBox->itemText(iA);
			if (!oldName.contains("*"))
				oldName.append("*");
			m_selectComBox->setItemText(iA,oldName);
			m_traits[iA].valid=false;		
		}
	}
}

void TraitSelectionWidget::setBarValue(int index,double value)
{
	m_barValues[traitIndex]=value;
}

void TraitSelectionWidget::defaultWeights()
{
	// FIXME: compute/verify weights via line search and cross correlation!!

	// get labels and group sizes (although labels are not needed here)
	std::vector<int> labels;
	int nA(0), nB(0);
	if( !getClassification( labels, nA, nB ) )
		return;

	// weights (100 / group size)
	double wA = 100. / (double)nA,
		   wB = 100. / (double)nB;

	m_weightC ->setValue( 1.0 );
	m_weightW1->setValue( wA );
	m_weightW2->setValue( wB );
}

bool TraitSelectionWidget::getClassification( std::vector<int>& labels, int& nA, int& nB )
{
	using namespace std;

	if( m_classVector.size() <= 1 ) 
	{
		QMessageBox::warning(this,tr("sdmvis: Warning"),
			tr("Classification needs 2 or more datasets!"));
		return false;
	}
// debug 
	int debug =0;
	int cVsize=m_classVector.size();
	for (int iA=0;iA<m_classVector.size();iA++)
	{
		debug =m_classVector.at(iA);
		int rage=32;
		rage++;
	}

// debug ende
	labels.clear(); 
	labels.resize(m_classVector.size());
	nA = 0;
	nB = 0;

	// get classification	
	cout << "Classification:" << endl << "---------------" << endl;
	for( int r=0; r < m_classVector.size(); ++r )
	{
		int group = m_classVector.at(r);
				// 0 - group A
				// 1 - group B

		labels.at(r) = (group==0) ? -1 : +1;
		if( group==0 ) nA++; else nB++;
		
		QString name = m_names.at(r);
		cout << r << ": group " << (group==0?"A":"B") << "  (" << name.toStdString() << ")" << endl;
	}

	// sanity check
	if( nA==0 || nB==0 ) {
		QMessageBox::warning(this,tr("sdmvis: Warning"),
			tr("Names are not assigned to different groups!"));
		return false;
	}

	return true;
}


void TraitSelectionWidget::computeTrait()
{
	using namespace std;
	
	// ------------------------ Setup problem----------------------------------
	m_wholeTrait.description=m_description->toPlainText();
	m_wholeTrait.identifier=m_identify->text();

	// wozu brauchen wir dies?
	m_wholeTrait.classNames[0]="Class A";
	m_wholeTrait.classNames[1]="Class B";
	m_wholeTrait.elementScale=m_elementScale->text().toDouble();
	
	m_wholeTrait.mhdFilename=m_mhdRelFilename;
	m_wholeTrait.filename=m_twpRelFilename;
	m_wholeTrait.weightC=m_weightC->text().toDouble();
	m_wholeTrait.weight1=m_weightW1->text().toDouble();
	m_wholeTrait.weight2=m_weightW2->text().toDouble();
	m_wholeTrait.numOfComp=m_ncompSpinBox->text().toInt();
	m_wholeTrait.matFilename=m_matRelFilename;
	m_wholeTrait.valid=false;

	int nrows = m_classVector.size();
	vector<int> labels( nrows );
	int nA(0), nB(0); // group sizes

	// get labels and group sizes
	if( !getClassification( labels, nA, nB ) )
		return;

	// instance matrix
	if( m_V.size1() != nrows )	{
		QMessageBox::warning(this,tr("sdmvis: Warning"),
			tr("Mismatch number of names and number of columns in eigenvector matrix!"));
		return;
	}
	// reduce to selected number of components (ie columns)
	Matrix V_k = m_V;
	int ncols = m_ncompSpinBox->value(); // was: m_V.size2()
	assert( ncols>0 && ncols<=(int)m_V.size2() );
	V_k.resize( V_k.size1(), ncols );

	vector<double> data;
	rednum::matrix_to_stdvector<double,Matrix>( V_k, data );
	//~~~ double* data = rednum::matrix_to_rawbuffer<double,Matrix>( V_k );
	cout << "Reduced SVM instance matrix to " << ncols << " columns." << endl;

	// setup SVM
	m_svm.clear();

	// default parameters for our application
	svm_parameter param = m_svm.params();
	param.svm_type = C_SVC;
	param.kernel_type = LINEAR;
	m_svm.set_params( param );

	// user weights
	double wA = m_weightW1->value(),
		   wB = m_weightW2->value();
	m_svm.params().C = m_weightC->value();
	m_svm.add_weight( -1, wA );
	m_svm.add_weight( +1, wB );
	cout << "Weight C = " << m_svm.params().C << endl
		 << "Weight class -1 = " << wA << endl
		 << "Weight class +1 = " << wB << endl;		  

	m_svm.setup_problem( nrows, ncols, &labels[0], &data[0] );
	//~~~ delete [] data; data=NULL;

	// ------------------------ Compute trait ---------------------------------

	// cross validation
	double crossval = m_svm.cross_validation( 10 );
	cout << "SVM 10-fold cross validation error (not normalized) = " << crossval << endl;

	// train SVM
	cout << "Training SVM..." << endl;
	if( !m_svm.train() )
	{
		QMessageBox::warning(this,tr("sdmvis: Error in SVM training"),
			tr("Support Vector Machine training failed!\nError: %1")
			.arg( m_svm.getErrmsg().c_str() ));
		return;
	}

	// get model SV, coefficients
	Matrix SV( m_svm.nrows(), m_svm.ncols() );
	Vector sv_coef( m_svm.nrows() );
	rednum::matrix_from_rawbuffer<double,Matrix,double>( SV     , m_svm.sv() );
	rednum::vector_from_rawbuffer<double,Vector,double>( sv_coef, m_svm.sv_coef() );

	namespace ublas = boost::numeric::ublas;

	// compute normal vector w = SV'*sv_coef  and distance b
	Vector w = ublas::prod( ublas::trans(SV), sv_coef );
	double b = - m_svm.model()->rho[0];

	if( m_svm.model()->label[0] == -1 ) {
		// see libsvm faq for rationale here
		w *= -1.;
		b *= -1.;
	}

	// project w into column space of V
	Vector v_w = ublas::prod( V_k, w );          // FIXME: normalization?!

	
	// store members
	//m_svm_w = w;
	//m_svm_b = b;
	//m_trait = v_w;
	m_trait = w;
	m_wholeTrait.distance=b*0.1;
	m_wholeTrait.trait=m_trait;
	// save Labels in trait
	m_wholeTrait.labels.resize(labels.size());
	for (unsigned int iA=0; iA<labels.size();iA++)
		m_wholeTrait.labels[iA]=labels[iA];

	

	// ------------------------ Save results ----------------------------------

	// save projector
	/*	QString projector_filename = QFileDialog::getSaveFileName( this,
		tr("Save trait vector (projector)..."),
		m_baseDir, tr("Raw matrix (*.mat)") );
	*/
	
	QString projector_filename = m_matFilename; // FULL PATH needed 
	
	if( !projector_filename.isEmpty() )
	{
		cout << "Saving " << v_w.size() << " x 1 V-space trait to \"" << projector_filename.toStdString() << "\"..." << endl;
		rednum::save_vector<float,Vector>( v_w, projector_filename.toAscii() );

		// auto-naming for further automatically generated files
		QString basename = projector_filename.left( projector_filename.size() - 4 );

		// save resulting SVM model	
		{
			QString model_filename = basename + ".svm";
			cout << "Saving resulting SVM model to \"" << model_filename.toStdString() << "\"..." << endl;
			m_svm.save_model( model_filename.toAscii() );
		}

		// save hyperplane normal and distance as raw vector 
		// (for eventual use in Matlab; last component is distance to origin)
		{
			QString normal_filename = basename + "-normal.mat";
			cout << "Saving hyperplane normal+distance to \"" << normal_filename.toStdString() << "\"..." << endl;
			Vector wb = w; 
			wb.resize( wb.size() + 1 );
			wb(wb.size()-1) = b;
			rednum::save_vector<float,Vector>( wb, normal_filename.toAscii() );
		}

		// save plain trait vector (padded by zeros to full eigenspace dimension)
		{
			QString w_filename = basename + "-w.mat";
			cout << "Saving plain trait vector to \"" << w_filename.toStdString() << "\"..." << endl;
			
			Vector wfull = w;			
			wfull.resize( nrows );
			for( int i=w.size(); i < nrows; ++i )
				wfull(i) = 0.0;

			cout << "w_full = ( " << endl;
			for(unsigned  int i=0; i < wfull.size(); ++i )
				cout << wfull(i) << endl;
			cout << ")" << endl;

			rednum::save_vector<float,Vector>( wfull, w_filename.toAscii() );
		}
	}

	if (m_fromCfg[m_selectComBox->currentIndex()])
	{
		// push trait in config, so plotter can draw classVector and traitLines 
		m_selectComBox->setEnabled(true);
		m_selectComBox->setToolTip("");
		m_butAdd->setEnabled(true);
		m_butAdd->setToolTip("");
		m_butCreate->setEnabled(true);
		m_butRemove->setDisabled(true);
		m_butCreate->setToolTip("");
		butReloadTrait->setEnabled(true);
		m_butCreate->setToolTip("");
		m_wholeTrait.valid=false;
		
		emit newTraitInTown(m_wholeTrait,m_selectComBox->currentIndex());
		// edited trait	
		QString oldName=m_selectComBox->currentText();
		if (!oldName.contains("*"))
			oldName.append("*");
		m_selectComBox->setItemText(m_selectComBox->currentIndex(),oldName);

	}
	else
	{
		m_butRemove->setEnabled(true);
		m_wholeTrait.mhdFilename="empty";
		emit newTraitInTown(m_wholeTrait,m_selectComBox->currentIndex());
	
	}

	m_validTrait[m_selectComBox->currentIndex()]=true;
	m_traitStatus->setText("Trait :  Valid");
	m_warpStatus->setText("Warp : Invalid");
	butComputeWarp->setEnabled(true);
	m_butRemove->setEnabled(true);
	

	// ------------------------ Matlab plots ----------------------------------

#ifdef SDMVIS_MATLAB_ENABLED
	if( true ) // was: if( m_updateMatlab->isChecked() )
	{
		static Matlab g_matlab;
		static bool g_matlabRunning = false;

		QString names_filename("temp_names.txt");
		QString mdir( "..\\sdmvis\\matlab\\" ), // FIXME: hardcoded script directory
				odir( ".\\" ), // FIXME: hardcoded output directory (relative to mdir)
				tmpnames( mdir+odir+names_filename );

		// HACK: Write tmp_names.txt to be loaded in Matlab engine via loadNames()
		ofstream tmp( tmpnames.toAscii() );
		if( tmp.is_open() )
		{
			for( int i=0; i < m_names.size(); ++i )
				tmp << (m_names[i].toStdString()) << endl;
			tmp.close();
		}
		else
			cout << "Warning: Could not open \"" << tmpnames.toStdString() << "\"!" << endl;

		// Matlab init
		if( !g_matlab.ep ) {
			g_matlab.setSilent( false );
			if( g_matlab.init() ) {
				g_matlab.cd( mdir.toAscii() ); 
				g_matlabRunning = true;
			}
		}

		// Matlab plots
		if( g_matlabRunning )
		{
			double* raw_V_k = rednum::matrix_to_rawbuffer<double,Matrix>( V_k );
			double* raw_w   = rednum::vector_to_rawbuffer<double,Vector>( w );
			assert( raw_V_k );
			assert( raw_w );

			cout << "Putting variables to Matlab engine" << endl;

			QString tmpnamescmd = QString("names_filename='") + names_filename + QString("';\n"); 
			g_matlab.eval( tmpnamescmd.toAscii() );

			uploadToMatlab( g_matlab.ep, "w", w.size(),1,              raw_w );
			uploadToMatlab( g_matlab.ep, "V", V_k.size1(),V_k.size2(), raw_V_k );
			uploadToMatlab( g_matlab.ep, "b", 1,1,                     &b );
			uploadToMatlab( g_matlab.ep, "labels", labels.size(),1 ,   &labels[0] );

			////////////////////////////////////////////////////////////////////
			// TODO: 
			// - working directory? (for eventual figure and .mat outputs)
			////////////////////////////////////////////////////////////////////

			delete [] raw_V_k;
			delete [] raw_w;

			g_matlab.eval( (tr("[names_full names] = loadNames('%1');").arg("temp_names.txt")).toAscii() );
			g_matlab.eval( (tr("hint='%1';").arg("foo")).toAscii() );
			g_matlab.eval( (tr("outdir='%1';").arg(odir)).toAscii() );
			g_matlab.eval( "scriptSVMAnalysis" );
		}		
	}
#endif // SDMVIS_MATLAB_ENABLED
}

