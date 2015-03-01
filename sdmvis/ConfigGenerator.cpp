// Own Inlcudes
#include "ConfigGenerator.h"
#include "SDMVisConfig.h"

// QtIncludes
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QFileDialog>
#include <QPushButton>
#include <QGroupBox>
#include <QByteArray>
#include <QSettings>
#include <QSpinBox>

QString ConfigGenerator::s_labelChooseFileButton = tr("Choose");
QString ConfigGenerator::s_labelPath = tr("Path:");

//------------------------------------------------------------------------------
//	HELPER FUNCTIONS
//------------------------------------------------------------------------------

size_t get_filesizeGen( const char* filename )
{
	using namespace std;
	ifstream fs( filename );
	size_t size;
	if( fs.is_open() )
	{
		fs.seekg( 0, ios::end );
		size = (size_t)fs.tellg();
		fs.close();
		return size;
	}
	return 0;
}

QByteArray vectorToQByteArrayGen( Vector v )
{	
	double* raw_v = rednum::vector_to_rawbuffer<double>( v );
	QByteArray tmp( (char*)raw_v, v.size()*sizeof(double) );
	delete [] raw_v;
	return tmp;
}

QByteArray matrixToQByteArrayGen( Matrix M )
{
	double* raw_M = rednum::matrix_to_rawbuffer<double>( M );
	QByteArray tmp( (char*)raw_M, M.size1()*M.size2()*sizeof(double) );
	delete [] raw_M;
	return tmp;
}

//***************** std Constructor *************************************************
ConfigGenerator::ConfigGenerator(QWidget *parent)
{
	this->setWizardStyle(ClassicStyle);
	// layouts
	addPage(createIntroPage());
	addPage(createWarpPage());
	addPage(createNamesPage());
	addPage(createWarpMatPage());
	addPage(createEigenWarpMatPage());
	addPage(createPcaModelPage());
	addPage(createFinalPage());

	connect(this,SIGNAL(finished(int)),this,SLOT(do_generate(int)));
}

//*********************** PAGE generation ********************************************
QWizardPage* ConfigGenerator::createIntroPage()
{
	QWizardPage *page = new QWizardPage;
	page->setTitle(tr("Choose a reference volume dataset."));
	
	QLabel *descriptionLabel = new QLabel(tr(
			"The reference is the volume dataset to which the deformation fields are applied." 
			"In case of a PCA model this should be chosen as the corresponding mean estimate."));
	descriptionLabel->setWordWrap(true);

	m_referenceLE=new QLineEdit();
	QLabel *pathLabel = new QLabel(s_labelPath);
	QPushButton * referenceButton= new QPushButton(s_labelChooseFileButton);
	connect(referenceButton,SIGNAL(clicked()),this,SLOT(chooseFileRef()));

	QGridLayout *layout = new QGridLayout;
	layout->addWidget(pathLabel, 0, 0);
	layout->addWidget(m_referenceLE, 0, 1);
	layout->addWidget(referenceButton, 0, 2);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(descriptionLabel);
	mainLayout->addLayout(layout);

	page->setLayout(mainLayout);
	return page;
}

QWizardPage* ConfigGenerator::createWarpPage()
{
	QWizardPage *page = new QWizardPage;
	page->setTitle("Select warpfield(s)");

	QLabel *descriptionLabel = new QLabel(tr(
			"Choose warpfield(s) for interactive deformation of reference.\n"
			"You can select one warpfield (e.g. according to a trait vector), "
			"or five warpfields (corresponding to the eigenvectors in the PCA model).\n"
			"Select 1 or 5 corresponding MHD files in the File Open Dialog."));
		descriptionLabel->setWordWrap(true);

	m_warpLE=new QLineEdit();
	m_warpfieldLabel= new QLabel("");
	QPushButton * warpButton= new QPushButton(s_labelChooseFileButton);
	connect(warpButton,SIGNAL(clicked()),this,SLOT(chooseFileWarp()));

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(descriptionLabel);
	mainLayout->addWidget(warpButton);
	mainLayout->addWidget(m_warpfieldLabel);

	page->setLayout(mainLayout);
	return page;
}

QWizardPage* ConfigGenerator::createWarpScalePage() // warpscale page not avalible
{
	QWizardPage *page = new QWizardPage;
	page->setTitle(tr("Specify scaling for warpfield(s)"));

	QVBoxLayout *mainLayout=new QVBoxLayout;
	m_scaleVector.resize(m_fileNameList.size());

	for (int iA=0;iA<m_scaleVector.size();iA++)
	{
		QHBoxLayout *tmpLayout=new QHBoxLayout;
		QLabel *tmpLabel=new QLabel(m_fileNameList.at(iA));
		QSpinBox *tmpSpin= new QSpinBox();
		tmpSpin->setValue(0.2);
	
		tmpLayout->addWidget(tmpLabel);
		tmpLayout->addWidget(tmpSpin);
		mainLayout->addLayout(tmpLayout);
	}
	
	page->setLayout(mainLayout);
	return page;
}

QWizardPage* ConfigGenerator::createNamesPage()
{
	QWizardPage *page = new QWizardPage;
	page->setTitle(tr("Specify names"));
	QLabel *descriptionLabel = new QLabel(
		tr("Descriptive names of the specimen in the analysis dataset used for trait analysis and PCA visualization."
		   "The names are given in a textfile newline separated."
		   "The ordering of the names must be the same as in the dataset (most probably lexicographic)."));
	descriptionLabel->setWordWrap(true);

	m_nameListLE=new QLineEdit();
	QLabel *pathLabel = new QLabel(s_labelPath);
	QPushButton * namesButton= new QPushButton(s_labelChooseFileButton);
	connect(namesButton,SIGNAL(clicked()),this,SLOT(chooseFileNameList()));

	QGridLayout *layout = new QGridLayout;
	layout->addWidget(pathLabel, 0, 0);
	layout->addWidget(m_nameListLE, 0, 1);
	layout->addWidget(namesButton, 0, 2);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(descriptionLabel);
	mainLayout->addLayout(layout);

	page->setLayout(mainLayout);
	return page;
}

QWizardPage* ConfigGenerator::createWarpMatPage()
{
	QWizardPage *page = new QWizardPage;
	page->setTitle(tr("Specify warps matrix"));

	QLabel *descriptionLabel = new QLabel(tr(
		"The warps matrix MAT file contains the full set of deformation fields produced by a registration algorithm."
		"Each deformation field describes the registration between the particular volume dataset and the reference."));
	descriptionLabel->setWordWrap(true);

	m_warpMatLE=new QLineEdit();
	QLabel *pathLabel = new QLabel(s_labelPath);
	QPushButton * warpButton= new QPushButton(s_labelChooseFileButton);
	connect(warpButton,SIGNAL(clicked()),this,SLOT(chooseFileWarpMat()));

	QGridLayout *layout = new QGridLayout;
	layout->addWidget(pathLabel, 0, 0);
	layout->addWidget(m_warpMatLE, 0, 1);
	layout->addWidget(warpButton, 0, 2);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(descriptionLabel);
	mainLayout->addLayout(layout);

	page->setLayout(mainLayout);
	return page;
}

QWizardPage* ConfigGenerator::createFinalPage()
{
	QWizardPage *page = new QWizardPage;
	page->setTitle(tr("Specify config name"));
	
	QLabel *descriptionLabel = new QLabel(tr(
		"All configuration options of a visual analysis are stored in a single configuration file.\n"
		"Interactive visual analysis is performed for a specific set of registered volume datasets."));
	descriptionLabel->setWordWrap(true);

	m_savingPathLE=new QLineEdit();
	QLabel *pathLabel = new QLabel(s_labelPath);
	QPushButton * saveButton= new QPushButton(s_labelChooseFileButton);
	connect(saveButton,SIGNAL(clicked()),this,SLOT(chooseFileSavingPath()));

	QGridLayout *layout = new QGridLayout;
	layout->addWidget(pathLabel, 0, 0);
	layout->addWidget(m_savingPathLE, 0, 1);
	layout->addWidget(saveButton, 0, 2);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(descriptionLabel);
	mainLayout->addLayout(layout);

	page->setLayout(mainLayout);
	return page;
}

QWizardPage* ConfigGenerator::createPcaModelPage()
{
	QWizardPage *page = new QWizardPage;
	page->setTitle(tr("Specify PCA model"));
	
	QLabel *descriptionLabel = new QLabel(tr(
		"The PCA model is computed for efficiency on the smaller scatter matrix. "
		"It consists of MAT files containing this smaller scatter matrix, "
		"the corresponding (low dimensional) eigenvectors and their eigenvalues (stored as a vector)."));
	descriptionLabel->setWordWrap(true);

	m_pcaModelLE_scatter	 = new QLineEdit();
	m_pcaModelLE_eigenVector = new QLineEdit();
	m_pcaModelLE_eigenValue	 = new QLineEdit();
	
	QPushButton * scatterButton		= new QPushButton(s_labelChooseFileButton);
	QPushButton * eigenVectorButton	= new QPushButton(s_labelChooseFileButton);
	QPushButton * eigenValueButton	= new QPushButton(s_labelChooseFileButton);

	QLabel *scatterLabel			= new QLabel(tr("Scatter matrix"));
	QLabel *eigenVectorMatLabel		= new QLabel(tr("Eigenvector matrix"));
	QLabel *eigenValueVectorLabel	= new QLabel(tr("Eigenvalue vector"));

	connect(scatterButton,SIGNAL(clicked()),this,SLOT(chooseFilePcaModel_scatter()));
	connect(eigenVectorButton,SIGNAL(clicked()),this,SLOT(chooseFilePcaModel_eigenVectorMat()));
	connect(eigenValueButton,SIGNAL(clicked()),this,SLOT(chooseFilePcaModel_eigenValues()));

	QGridLayout *layout = new QGridLayout;
	layout->addWidget(scatterLabel, 0, 0);
	layout->addWidget(eigenVectorMatLabel, 1, 0);
	layout->addWidget(eigenValueVectorLabel, 2, 0);
	
	layout->addWidget(m_pcaModelLE_scatter, 0, 1);
	layout->addWidget(m_pcaModelLE_eigenVector, 1, 1);
	layout->addWidget(m_pcaModelLE_eigenValue, 2, 1);
	
	layout->addWidget(scatterButton, 0, 2);
	layout->addWidget(eigenVectorButton, 1, 2);
	layout->addWidget(eigenValueButton, 2, 2);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(descriptionLabel);
	mainLayout->addLayout(layout);

	page->setLayout(mainLayout);
	return page;
}

QWizardPage* ConfigGenerator::createEigenWarpMatPage()
{
	QWizardPage *page = new QWizardPage;
	page->setTitle(tr("Specify eigenwarps matrix"));

	QLabel *descriptionLabel = new QLabel(tr(
		"The eigenwarps matrix MAT file contains the full set of warpfields "
		"corresponding to the eigenvectors of the PCA model."));
	descriptionLabel->setWordWrap(true);

	m_eigenWarpMatLE			  = new QLineEdit();
	QLabel *pathLabel			  = new QLabel(s_labelPath);
	QPushButton * eigenWarpButton = new QPushButton(s_labelChooseFileButton);

	connect(eigenWarpButton,SIGNAL(clicked()),this,SLOT(chooseFileEigenWarpMat()));

	QGridLayout *layout = new QGridLayout;
	layout->addWidget(pathLabel, 0, 0);
	layout->addWidget(m_eigenWarpMatLE, 0, 1);
	layout->addWidget(eigenWarpButton, 0, 2);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(descriptionLabel);
	mainLayout->addLayout(layout);

	page->setLayout(mainLayout);
	return page;
}

//**********************  Slots ************************************************************
void ConfigGenerator::chooseFileRef()		  {selectPath(m_referenceLE,	"*.mhd",false);}
void ConfigGenerator::chooseFileWarp()		  {selectPath(m_warpLE,			"*.mhd",true) ;}
void ConfigGenerator::chooseFileNameList()	  {selectPath(m_nameListLE,		"*.txt",false);}
void ConfigGenerator::chooseFileWarpMat()	  {selectPath(m_warpMatLE,		"*.mat",false) ;}
void ConfigGenerator::chooseFileEigenWarpMat(){selectPath(m_eigenWarpMatLE,	"*.mat",false);}

void ConfigGenerator::chooseFilePcaModel_scatter()			 {selectPath(m_pcaModelLE_scatter,		"*.mat",false);}
void ConfigGenerator::chooseFilePcaModel_eigenVectorMat()	 {selectPath(m_pcaModelLE_eigenVector,	"*.mat",false);}
void ConfigGenerator::chooseFilePcaModel_eigenValues()		 {selectPath(m_pcaModelLE_eigenValue,	"*.mat",false);}

void ConfigGenerator::chooseFileSavingPath()  
{	
	QString filename;
	filename = QFileDialog::getSaveFileName( this,
		tr("Save sdmvis config file"),m_path, tr("SDMVis configuration file (*.ini)") );

	if( filename.isEmpty() )
		return;
	
	m_savingPathLE->setText(filename);

	
}
void ConfigGenerator::auto_generateConfig(QString savingPath,
										  QString reference,
										  QString warpsMatrix,
										  QString eigenWarpMatrix,
										  QString scatterMatrix,
										  QString EigenVector,
										  QString EigenValue,
										  QStringList warpFieldFileList
										  
										  )
{

		QFileInfo info(savingPath);
		m_path=info.absolutePath();
		m_config->setBasePath(m_path); //the path to config 
		
		// generate Relativ Paths to config
		
		


		m_config->setBasePath(m_path); //the path to config 
		m_config->setIdentifier(QString::number(m_config->version));
		m_config->setDescription(tr("N.A"));
		m_config->setReference(reference);
		m_config->clearWarpFieldList();
		m_config->setWarpfieldFileList(warpFieldFileList);
		m_config->setWarpsMatrix(warpsMatrix);
		m_config->setEigenwarpsMatrix(eigenWarpMatrix);

		m_config->loadScatterMatrix(scatterMatrix.toAscii(),m_config->getNames().size());
		m_config->loadPCAEigenvectors(EigenVector.toAscii(),m_config->getScatterMatrix().size1());
		m_config->loadPCAEigenvalues( EigenValue.toAscii(), m_config->getPCAEigenvectors().size2());
		// generated config has no traits! 
		m_config->clearTraits();
}

void ConfigGenerator::do_generate(int result)
{
	using namespace std;
	int nameSize;
	
	if( m_savingPathLE->text().isNull()) // no saving file selected
		return;

	if (!(m_savingPathLE->text().endsWith(".ini"))) // wrong file extension
		return; 

	//Push Values to the config Structure
	{
		QFileInfo info(m_savingPathLE->text());
		m_path=info.absolutePath();

		m_config->setBasePath(m_path); //the path to config 
		m_config->setIdentifier(QString::number(m_config->version));
		m_config->setDescription(tr("N.A"));
		m_config->setReference(m_referenceLE->text());
		m_config->clearWarpFieldList();

		for (int iA=0;iA<m_fileNameList.size();iA++)
		{
			QString relPath=m_config->getRelativePath(m_fileNameList.at(iA));
			m_fileNameList[iA]=relPath;
		}


		m_config->setWarpfieldFileList(m_fileNameList);
		m_config->setWarpsMatrix(m_warpMatLE->text());
		m_config->setEigenwarpsMatrix(m_eigenWarpMatLE->text());

		// open names textfile
		QFile readFile( m_nameListLE->text());
		if ( readFile.open( QIODevice::ReadOnly ) ) 
		{
			QTextStream readStream( &readFile );
			QString line;
			int lineNumber = 0;
			while ( !readStream.atEnd() ) 
			{
				line = readStream.readLine( );
				m_namesList.push_back(line);
				lineNumber++;
			}
			readFile.close();
			nameSize=lineNumber;
		}

		m_config->setNames(m_namesList);
		m_config->loadScatterMatrix(m_pcaModelLE_scatter->text().toAscii(),nameSize);
		m_config->loadPCAEigenvectors(m_pcaModelLE_eigenVector->text().toAscii(),m_config->getScatterMatrix().size1());
		m_config->loadPCAEigenvalues( m_pcaModelLE_eigenValue->text().toAscii(), m_config->getPCAEigenvectors().size2());
		// generated config has no traits! 
		m_config->clearTraits();
	}

	// write Config
	m_config->writeConfig(m_savingPathLE->text());
	// load Config
	emit configGenerated(m_savingPathLE->text());
}


//**********************  Functions ************************************************************
void ConfigGenerator::selectPath(QLineEdit * selectedLE,QString fileExtension, bool multiSelect)
{
	
	if(!multiSelect)
	{
		QString fileName = QFileDialog::getOpenFileName(this,
				 tr("Select file"), m_path, fileExtension);
		if ( fileName.isNull() == false )
    		selectedLE->setText(fileName);  
		QFileInfo info(fileName);

	m_path=info.absolutePath();
	}
	else
	{
		QStringList filenames = QFileDialog::getOpenFileNames( this,
					tr("Open warpfields..."), m_path, fileExtension );

		m_fileNameList=filenames;
		if ( filenames.isEmpty() == false )
		{
    		QString itemList;
			m_fileNameList=filenames;	
				
			for (int iA=0;iA<filenames.size();iA++)
				itemList.append("Warp "+QString::number(iA)+": "+filenames.at(iA)+"\n");

			m_warpfieldLabel->setText(itemList);
		}
	}
}

void ConfigGenerator::setConfigPath(QString pathToConfig)
{
	m_path=pathToConfig;
}
