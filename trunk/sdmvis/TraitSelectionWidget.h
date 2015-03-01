#ifndef TRAITSELECTIONWIDGET_H
#define TRAITSELECTIONWIDGET_H

// Own Includes 
#include "Trait.h"
#include "mat/numerics.h"
#include "mat/SVMTrain.h"

// Qt Includes
#include <QWidget>
#include <QComboBox>
#include <QVector>
#include <QCheckBox>
#include <QTreeWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QStandardItemModel>
#include <QDoubleSpinBox>
// HelpClass
class TraitComb;

class TraitSelectionWidget : public QWidget
{
 Q_OBJECT
public:
    TraitSelectionWidget(QWidget *parent = 0);
	
#ifdef SDMVIS_VARVIS_ENABLED
	bool getPushToVarVis();
#endif
	// public funtions
	
	QComboBox *getSelectionBox(){return m_selectComBox;}
	void setTraitList(QList<Trait> traits);
	void clearTrait();
	void setNames( QStringList names );
	void openMatrix(QString filename);
	void loadMatrix(Matrix * eigenVectorMatrix);
	void setFirstValidation();
	void enable_TraitProperties(bool yes);
	void enable_selectionProperties(bool yes);
	void sanityCheck();
	QList<Trait> getTraitList(){return m_traits;}
	// implemented
	int  getTraitIndex(){return traitIndex;};
	void setConfigName(QString name){m_configName=name;
									 m_configName.remove(".ini");} // FIXME: what about "my.initial.ini"?
	void setPath(QString path){m_baseDir=path;}

void setSelectionBox(QComboBox *selectionBox);
private:
	void setValidation(bool valid);
	QComboBox		*m_selectComBox;
	
	QVector<double> m_barValues;
    QVector<double> m_scaleValues;
	QVector<int>    m_classVector;
	QVector<bool>   m_validTrait;
	QVector<bool>   m_validWarp;
	QVector<bool>   m_fromCfg;
	QVector<bool>   m_edited;
	
	
	QList<Trait>      m_traits;
	QList<TraitComb*> m_combList;
	QTreeWidget		 *m_treeWidget;
	
	QStringList	m_names;
	QString		m_baseDir;
	QString		m_mhdAbsPath;
	QString		m_configName;
	QString		m_saveFile;
	
	QString		m_mhdFilename;
	QString		m_twfFileName;
	QString		m_matFilename;

	QString		m_mhdRelFilename;
	QString		m_twpRelFilename;
	QString		m_matRelFilename;
	
	int traitIndex;
	bool m_bool_addnewTrait;
	double m_defaultElementScaleValue;
	
	
	QGroupBox *m_selectionGroup;
	QLineEdit *m_identify;
	QTextEdit *m_description;
	QTabWidget *m_optionTab;

	QPushButton * m_butAdd;
	QPushButton * m_butRemove;
	QPushButton * m_butCreate;
	
	QPushButton * butComputeTrait;
	QPushButton * butComputeWarp ;
	QPushButton * butReloadTrait;

	QLabel *m_traitStatus;
	QLabel *m_warpStatus;

#ifdef SDMVIS_VARVIS_ENABLED
	QCheckBox * m_pushTraitToVarVis;
#endif

	void	clearAllValues();
	void	setLabels();
	void	createNewTrait(int index);
	QString getRelativePath( QString path, QString relativeTo ) const;

	// SVM parameters
	QSpinBox*           m_ncompSpinBox;	
	QDoubleSpinBox*     m_elementScale;
	QDoubleSpinBox*     m_weightC;
	QDoubleSpinBox*     m_weightW1;
	QDoubleSpinBox*     m_weightW2;
	QCheckBox*          m_lambdaScaling;
	//QCheckBox*          m_updateMatlab;  // TODO: add the Matlab checkbox for 'expert mode'
	Matrix   m_V;      ///< first k eigenvectors
	SVMTrain m_svm;
	Vector   m_trait;
	Trait    m_wholeTrait;///< trait vector in V-space (aka 'projector')

protected:
	bool getClassification( std::vector<int>& labels, int& nA, int& nB );

public slots:
		void verifyValidation();
		void textChanged();
		void setSpecificTrait(int index);
		void setBarValue(int index,double value);
		void openNames();
		void openLabels();
		void defaultWeights();
		void computeTrait();
		void setValue(int index);
		void computeWarp();
		void warpfieldGenerated();
		void addExistTrait();
		void addNewTrait();
		void reloadTrait();
		void scaleValueChanged(double);
		void removeTraitFromCFG();
		void setIndex(int index);
#ifdef SDMVIS_VARVIS_ENABLED
		void clearVarVis();
		void pushTraitToVarVis(bool);
#endif
signals:
		void loadBarValue(int,double);
		void loadNewTrait(int);
		void newTraitInTown(Trait newTrait, int pos);
		void computeWarpField(QString mhdFilename,QString matFilename);
		void loadWarpfield(QString);
		void setTraitScale(int , double);
		void unloadWarpfield();
		void updateTraitList(QList<Trait>,int);
		void clearVarVisBox();
		void setGetTraitsFromSDMVIS(bool);
};

#endif //TRAITSELECTIONWIDGET_H