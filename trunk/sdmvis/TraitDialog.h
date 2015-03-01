#ifndef TRAITDIALOG_H
#define TRAITDIALOG_H

#include <QDialog>
#include "mat/numerics.h"
#include "mat/SVMTrain.h"

class QStandardItemModel;
class QTreeView;
class ComboDelegate;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;

class TraitDialog : public QDialog
{
	Q_OBJECT

public:
	TraitDialog( QWidget* parent=0 );

	void setBaseDir( QString baseDir ) { m_baseDir = baseDir; }

public slots:
	void setNames( QStringList names );

protected slots:
	void openNames();
	void openMatrix();
	void loadLabels();
	void computeTrait();
	void defaultWeights();

protected:
	bool getClassification( std::vector<int>& labels, int& nA, int& nB );

private:
	QString             m_baseDir;
	QStandardItemModel* m_model;
	QTreeView*          m_treeView;
	ComboDelegate*      m_groupDelegate;

	// SVM parameters
	QSpinBox*           m_ncompSpinBox;	
	QDoubleSpinBox*     m_weightC;
	QDoubleSpinBox*     m_weightW1;
	QDoubleSpinBox*     m_weightW2;
	QCheckBox*          m_lambdaScaling;

	QCheckBox*          m_updateMatlab;

	QStringList m_names;

	Matrix   m_V;      ///< first k eigenvectors
	SVMTrain m_svm;
	Vector   m_trait;  ///< trait vector in V-space (aka 'projector')

	// FIXME: unused yet?!
	//Vector   m_svm_w;  ///< SVM hyperplane normal w
	//double   m_svm_b;  ///< SVM hyperplane distance b to origin
};

#endif // TRAITDIALOG_H
