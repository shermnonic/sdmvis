#ifndef CONFIGGENERATOR_H
#define CONFIGGENERATOR_H

// own Includes
#include "numerics.h"
#include "Warpfield.h"
#include "Trait.h"
#include "numerics.h"   

// Qt Includes
#include <QDialog>
#include <QWidget>
#include <QWizard>
#include <QLineEdit>
#include <QLabel>
//#include <QRadioButton >

class SDMVisConfig;

class ConfigGenerator : public QWizard
{
	Q_OBJECT

	static QString s_labelChooseFileButton;
	static QString s_labelPath;

public:
	friend class SDMVisConfig;
	ConfigGenerator(QWidget *parent = 0); // std Constructor

	void setConfigPath(QString pathToConfig);
	void setConfig(SDMVisConfig *pointerToCfg){m_config=pointerToCfg;} 
    void auto_generateConfig(QString savingPath,
							 QString reference,
							 QString warpsMatrix,
							 QString eigenWarpMatrix,
							 QString scatterMatrix,
							 QString EigenVector,
							 QString EigenValue,
							 QStringList warpFieldFileList
										  
										  );
	

private:
	QStringList		 m_fileNameList; // warpfield list
	QStringList		 m_namesList;
	QString			 m_path;
	QVector <double> m_scaleVector;

	QLineEdit	 *m_referenceLE;
	QLineEdit	 *m_warpLE;
	QLineEdit	 *m_scalingLE;
	QLineEdit	 *m_nameListLE; // names.txt
	QLineEdit	 *m_warpMatLE;
	QLineEdit	 *m_eigenWarpMatLE;
	QLineEdit	 *m_pcaModelLE;
	QLineEdit	 *m_savingPathLE;
	
	QLabel		 *m_warpfieldLabel;
	SDMVisConfig *m_config;

	QLineEdit *m_pcaModelLE_scatter;
	QLineEdit *m_pcaModelLE_eigenVector;
	QLineEdit *m_pcaModelLE_eigenValue;

	QWizardPage *createIntroPage();
	QWizardPage *createWarpPage();
	QWizardPage *createNamesPage();
	QWizardPage *createWarpScalePage();
	QWizardPage *createWarpMatPage();
	QWizardPage *createEigenWarpMatPage();
	QWizardPage *createPcaModelPage();
	QWizardPage *createFinalPage();

	void selectPath(QLineEdit *selectedLE,QString fileExtension, bool multiSelect);

signals:
	void configGenerated(QString);
	
private slots:
	void chooseFileRef();
	void chooseFileWarp();
	void chooseFileNameList();
	void chooseFileWarpMat(); 
	void chooseFileEigenWarpMat();
	void chooseFileSavingPath();
	void chooseFilePcaModel_scatter();
	void chooseFilePcaModel_eigenVectorMat();
	void chooseFilePcaModel_eigenValues();	
	void do_generate(int result);

};
#endif // ConfigGenerator_H
