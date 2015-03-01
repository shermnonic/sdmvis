#include "CSVExporter.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QFileDialog>
#include <QPushButton>
#include <QGroupBox>

// Constructor
CSVExporter::CSVExporter(QWidget *parent)
{
	// groupBox
	QGroupBox *pathGroup =new QGroupBox("Path And Name Settings");
	QGroupBox *ModelGroup=new QGroupBox("Select Data");
	QGroupBox *numGroup  =new QGroupBox("Select Number Seperator");
	QGroupBox *sepGroup  =new QGroupBox("Select Seperator String");

	// sub layouts
	QGridLayout* pathLayout = new QGridLayout;
	QHBoxLayout* sepLayout  = new QHBoxLayout;
	QHBoxLayout* pcaLayout  = new QHBoxLayout;
	QHBoxLayout* numLayout  = new QHBoxLayout;
	QVBoxLayout* mainLayout = new QVBoxLayout;
	
	// misc	
	QPushButton	*exportButton	 = new QPushButton("Export");
	QLabel		*sepLabel		 = new QLabel("");
	QLabel		*pathLabel		 = new QLabel("Path");
	QLabel		*nameLabel		 = new QLabel("Name");
	QPushButton *changePathButton= new QPushButton("change");
	
	m_sepLineEdit	  = new QLineEdit;
	m_pathLineEdit	  = new QLineEdit();
	m_nameLineEdit	  = new QLineEdit();
	m_pcaChBox		  = new QRadioButton("PCA Model");
	m_pcaMatrixChBox  = new QRadioButton("PCA Matrix");
	m_commaChBox	  = new QRadioButton("Comma");
	m_dotChBox        = new QRadioButton("Dot");
	m_sepLineEdit->setMaximumWidth(30);
	
	// set layouts
	sepLayout->addWidget(m_sepLineEdit,Qt::AlignLeft);
	sepLayout->addWidget(sepLabel,Qt::AlignLeft);

	pathLayout->addWidget(pathLabel,0,0);
	pathLayout->addWidget(m_pathLineEdit,0,1);
	pathLayout->addWidget(changePathButton,0,2);
	pathLayout->addWidget(nameLabel,1,0);
	pathLayout->addWidget(m_nameLineEdit,1,1);

	pcaLayout->addWidget(m_pcaChBox);
	pcaLayout->addWidget(m_pcaMatrixChBox);

	numLayout->addWidget(m_commaChBox);
	numLayout->addWidget(m_dotChBox);

	// set group layouts
	pathGroup ->setLayout(pathLayout);
	ModelGroup->setLayout(pcaLayout);
	numGroup  ->setLayout(numLayout);
	sepGroup  ->setLayout(sepLayout);
	
	// set main layout
	mainLayout->addWidget(pathGroup);
	mainLayout->addWidget(ModelGroup);
	mainLayout->addWidget(numGroup);
	mainLayout->addWidget(sepGroup);
	mainLayout->addWidget(exportButton);
	
	setLayout(mainLayout);

	
	// connections
	connect(exportButton    ,SIGNAL(clicked()),this,SLOT(do_Export()));
	connect(changePathButton,SIGNAL(clicked()),this,SLOT(do_changePath()));
	connect(m_dotChBox      ,SIGNAL(clicked()),this,SLOT(dotCheckBox()));
	connect(m_commaChBox    ,SIGNAL(clicked()),this,SLOT(commaCheckBox()));
	connect(m_pcaChBox      ,SIGNAL(clicked()),this,SLOT(pcaModelCheckBox()));
	connect(m_pcaMatrixChBox,SIGNAL(clicked()),this,SLOT(pcaMatrixCheckBox()));

	// init 
	m_dotChBox->setChecked(true);
	m_pcaChBox->setChecked(true);
	m_sepLineEdit->setText(" ;");
	resize(600,200);
}

// slots 
void CSVExporter::do_changePath()
{
	QDir directory;
	directory.setPath(m_path);
    QString path = QFileDialog::getExistingDirectory (this, tr("Directory"), directory.path());
    if ( path.isNull() == false )
    	m_pathLineEdit->setText(path);  
}

void CSVExporter::dotCheckBox()
{
	if ( m_dotChBox->isChecked() && m_commaChBox->isChecked() )
		m_commaChBox->setChecked(false);
	
	m_boolDotSeperation=m_dotChBox->isChecked();
}

void CSVExporter::commaCheckBox()
{
	if (m_dotChBox->isChecked() && m_commaChBox->isChecked())
		m_dotChBox->setChecked(false);
	
	m_boolDotSeperation=m_dotChBox->isChecked();
}

void CSVExporter::pcaModelCheckBox()
{
	if (m_pcaChBox->isChecked() && m_pcaMatrixChBox->isChecked() )
		m_pcaMatrixChBox->setChecked(false);
	m_boolPCModel=m_pcaChBox->isChecked();
	m_nameLineEdit->setText(m_name);
}

void CSVExporter::pcaMatrixCheckBox()
{
	if (m_pcaChBox->isChecked() && m_pcaMatrixChBox->isChecked() )
		m_pcaChBox->setChecked(false);
	
	m_boolPCModel=m_pcaChBox->isChecked();
	m_nameLineEdit->setText(m_tempName+"(Matrix).csv");
}

void CSVExporter::setPcComp(int pcX,int pcY)
{
	m_pcx=pcX;
	m_pcy=pcY;
}

void CSVExporter::setMatrix(Matrix* pcaMatrix)
{
	m_pcaMatrix=pcaMatrix;
}

void CSVExporter::setPath(QString Path, QString Name)
{
	Path.remove(Name);
	Name.remove(".ini");
	m_tempName=Name;
	Name.append("(PC"+
				QString::number(m_pcx+1)+
				"_PC"+
				QString::number(m_pcy+1)+
				").csv");

	Path.append("CSVExport/");
	m_path=Path;
	m_name=Name;
	
	m_nameLineEdit->setText(Name);
	m_pathLineEdit->setText(Path);
}

void CSVExporter::do_Export()
{
	m_seperator=m_sepLineEdit->text();
	QString absPath(m_pathLineEdit->text());
	if (!m_pathLineEdit->text().endsWith("/"))
		absPath.append("/");
	
	absPath.append(m_nameLineEdit->text());
	if(!(QDir(m_path).exists()))
		QDir().mkdir(m_path);

	QFile file(absPath);
	if ( file.open(QIODevice::WriteOnly | QIODevice::Text) )
	{
		if(m_boolPCModel)
		{
			QTextStream stream( &file );
			for (int iA=0;iA<m_pcaMatrix->size1();iA++) // quad matrix 
			{
				QString element1=QString::number(m_pcaMatrix->at_element(m_pcx,iA));
				QString element2=QString::number(m_pcaMatrix->at_element(m_pcy,iA));
				
				if(!m_boolDotSeperation)
				{
					element1.replace(".",",");
					element2.replace(".",",");			
				}
				stream << element1 << m_seperator << element2  << endl;
			}
		}
		else // whole Matrix
		{
			QTextStream stream( &file );
			for (int iA=0;iA<m_pcaMatrix->size1();iA++) // quad matrix 
			{
				for (int iB=0;iB<m_pcaMatrix->size1()-1;iB++)
				{
					QString element1=QString::number(m_pcaMatrix->at_element(iA,iB));
					if(!m_boolDotSeperation)
						element1.replace(".",",");
		
					stream << element1  << m_seperator ;
				}
				QString lastElement=QString::number(m_pcaMatrix->at_element(iA,m_pcaMatrix->size1()-1));
				if(!m_boolDotSeperation)
					lastElement.replace(".",",");

				stream << lastElement << endl;
			}
		}	
	}
}