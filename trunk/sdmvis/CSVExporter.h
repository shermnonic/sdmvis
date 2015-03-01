#ifndef CVSEXPORTER_H
#define CVSEXPORTER_H

#include "numerics.h"
#include <QDialog>
#include <QWidget>
#include <QLineEdit>
#include <QRadioButton >
class CSVExporter : public QDialog
{
	Q_OBJECT
public:
	CSVExporter(QWidget *parent = 0);

private:
	bool m_boolDataLoaded;
	bool m_boolPCModel;
	bool m_boolDotSeperation;

	int  m_pcx;
	int  m_pcy;
	QString m_path;
	QString m_name;
	QString m_tempName;
	Matrix* m_pcaMatrix;
	
	QLineEdit* m_pathLineEdit;
	QLineEdit* m_nameLineEdit;
	QLineEdit *m_sepLineEdit;
	
	QString m_seperator;
	QRadioButton *m_commaChBox;
	QRadioButton *m_dotChBox;
	QRadioButton *m_pcaChBox;
	QRadioButton *m_pcaMatrixChBox;

	public slots:
		void setPcComp(int pcX,int pcY);				// select row in matrix
		void setMatrix(Matrix * pcaMatrix);				// set Matrix
		void setPath(QString Path,QString Name);		// path and name given form mainwindow on loading config
		void do_Export();								// write 
		void do_changePath();							// change dir where file will be saved
		void commaCheckBox();							// slots for mutal exclusuin on the radio buttonss
		void dotCheckBox();
		void pcaModelCheckBox();
		void pcaMatrixCheckBox();
};
#endif // CSVExporter_H