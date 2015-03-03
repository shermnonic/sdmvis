#ifndef TRAITDIALOG_H
#define TRAITDIALOG_H

#include <QDialog>

#ifndef Q_MOC_RUN 
// Workaround for BOOST_JOIN problem: Undef critical code for moc'ing.
// There is a bug in the Qt Moc application which leads to a parse error at the 
// BOOST_JOIN macro in Boost version 1.48 or greater.
// See also:
//	http://boost.2283326.n4.nabble.com/boost-1-48-Qt-and-Parse-error-at-quot-BOOST-JOIN-quot-error-td4084964.html
//	http://cgal-discuss.949826.n4.nabble.com/PATCH-prevent-Qt-s-moc-from-choking-on-BOOST-JOIN-td4081602.html
//	http://article.gmane.org/gmane.comp.lib.boost.user/71498
#include "mat/numerics.h"
#include "mat/SVMTrain.h"
#endif

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
