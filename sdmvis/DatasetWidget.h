#ifndef DATASETWIDGET_H
#define DATASETWIDGET_H

#include <QWidget>
#include <QStringList>

class QStandardItemModel;
class QTableView;

class DatasetWidget;
class DatasetControlWidget;

//==============================================================================
//	DatasetControlWidget
//==============================================================================
class DatasetControlWidget : public QWidget
{
Q_OBJECT

public:
	DatasetControlWidget( QWidget* parent=0 );

	void setMaster( DatasetWidget* master ) { m_master = master; }

public slots:
	void setWarpfields();

private:
	DatasetWidget* m_master;
};


//==============================================================================
//	DatasetWidget
//==============================================================================
class DatasetWidget : public QWidget
{
Q_OBJECT

	/// Column indices
	enum { ColName=0, ColFilename, ColReference, ColFeatures, ColAligned,
	       ColRegistered, ColWarp, NumColumns };

public:
	DatasetWidget( QWidget* parent=0 );

public slots:
	// convenience functions as long as dataset workflow is not established
	void setNames( QStringList names );

	void setWarpFilenames( QStringList wnames );

	DatasetControlWidget* getControlWidget() { return m_controlWidget; }

private:
	QStandardItemModel*   m_model;
	QTableView*           m_tableView;
	DatasetControlWidget* m_controlWidget;
};

#endif // DATASETWIDGET_H
