#ifndef SDMTENSORPROBEWIDGET_H
#define SDMTENSORPROBEWIDGET_H

#include "SDMTensorProbe.h"
#include <QWidget>
#include <QList>
#include <QModelIndex>

class TensorVisBase;
class SDMTensorDataProvider;
class QTreeView;
class QStandardItemModel;

class SDMTensorProbeWidget : public QWidget
{
	Q_OBJECT

signals:
	void statusMessage( QString );	
	void capturedProbe( int );
	void appliedProbe( int );
	
public:
	SDMTensorProbeWidget( QWidget* parent=0 );

	void setMasters( TensorVisBase* tvb, SDMTensorDataProvider* tdp );

	void setProbes( QList<SDMTensorProbe> probes );

	/// Get copy of probes
	QList<SDMTensorProbe> getProbes() /*const*/; 

	/// Access i-th probe (for modification)
	SDMTensorProbe& getProbe( int idx );

public slots:
	/// Call this slot from master to add current settings as new probe to list
	void captureNewProbe();
	void updateListView();
	
protected slots:
	void applyProbe( const QModelIndex & index );
	void removeSelectedProbe();
	void applySelectedProbe();
	void updateProbes(); // Apply changes from GUI to internal probes list

protected:
	// Returns index of new probe
	int addProbe( SDMTensorProbe probe );	

private:
	QList<SDMTensorProbe>  m_probes;
	TensorVisBase*         m_tvis;
	SDMTensorDataProvider* m_tdata;

	QTreeView*             m_listView;
	QStandardItemModel*    m_listModel;
};

#endif // SDMTENSORPROBEWIDGET_H
