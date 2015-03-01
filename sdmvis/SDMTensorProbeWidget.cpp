#include "SDMTensorProbeWidget.h"
#include "TensorVisBase.h"
#include "SDMTensorDataProvider.h"

#include <QTreeView>
#include <QStandardItemModel>
#include <QStringList>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

SDMTensorProbeWidget
  ::SDMTensorProbeWidget( QWidget* parent )
  : QWidget(parent)
{
	m_listView  = new QTreeView();
	m_listModel = new QStandardItemModel();

	m_listView->setMinimumHeight( 200 );

	QPushButton* butAdd = new QPushButton(tr("Add"));
	QPushButton* butDel = new QPushButton(tr("Del"));
	QPushButton* butSet = new QPushButton(tr("Set"));

	QHBoxLayout* butLayout = new QHBoxLayout;
	butLayout->addWidget( butSet );
	butLayout->addWidget( butAdd );
	butLayout->addWidget( butDel );
	
	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget( m_listView );
	layout->addLayout( butLayout );

	this->setLayout( layout );
	
	connect( butAdd, SIGNAL(clicked()), this, SLOT(captureNewProbe()) );
	connect( butDel, SIGNAL(clicked()), this, SLOT(removeSelectedProbe()) );	
	connect( butSet, SIGNAL(clicked()), this, SLOT(applySelectedProbe()) );	
	//connect( m_listView, SIGNAL(doubleClicked(const QModelIndex&)),
	//         this,       SLOT  (applyProbe   (const QModelIndex&)) );
}

void SDMTensorProbeWidget
  ::setMasters( TensorVisBase* tvb, SDMTensorDataProvider* tdp )
{
	m_tvis  = tvb;
	m_tdata = tdp;
}

void SDMTensorProbeWidget
  ::captureNewProbe()
{
	if( !m_tvis || !m_tdata )
		return;
	
	SDMTensorProbe probe;
	probe.getFrom( m_tvis, m_tdata );
	
	int idx = addProbe( probe );

	emit capturedProbe( idx );
}

void SDMTensorProbeWidget
  ::applyProbe( const QModelIndex & index )
{
	int idx = index.row();
	
	if( idx<0 || idx >= m_probes.size() )
		return;
	
	if( !m_tvis || !m_tdata )
		return;	
	
	m_probes[idx].applyTo( m_tvis, m_tdata );

	emit appliedProbe( idx );
}

void SDMTensorProbeWidget
  ::setProbes( QList<SDMTensorProbe> probes )
{
	m_probes = probes;

	// Update UI
	updateListView();
}

int SDMTensorProbeWidget
  ::addProbe( SDMTensorProbe probe )
{
	m_probes.push_back( probe );
	
	// Update UI
	updateListView();

	return m_probes.size()-1;
}

void SDMTensorProbeWidget
  ::applySelectedProbe()
{
	applyProbe( m_listView->currentIndex() );
}

void SDMTensorProbeWidget
  ::removeSelectedProbe()
{
	int idx = m_listView->currentIndex().row();
	
	if( idx<0 || idx >= m_probes.size() )
		return;
	
	m_probes.removeAt( idx );
	
	// Update UI
	updateListView();
}

void SDMTensorProbeWidget
  ::updateListView()
{
	QStringList sl;	

	m_listModel->clear();
	m_listModel->setHorizontalHeaderLabels( QStringList() << "#" << "Name" << "Position" );

	for( int i=0; i < m_probes.size(); i++ )
	{
		// Probe is stored in normalized coords, convert to physical for display
		double pt[3];
		m_tdata->space().getPointFromNormalized( m_probes[i].point, pt );

		QStandardItem* itemNumber = new QStandardItem(
				tr("%1").arg( i+1 ,2) );

		QStandardItem* itemPosition = new QStandardItem(
			tr("(%1,%2,%3)")
				.arg(pt[0],5,'f',2)
				.arg(pt[1],5,'f',2)
				.arg(pt[2],5,'f',2) );

		QStandardItem* itemName = new QStandardItem(
				QString::fromStdString(m_probes[i].name) );

		itemNumber  ->setEditable( false );
		itemPosition->setEditable( false );
		itemName    ->setEditable( true  );

		m_listModel->setItem( i, 0, itemNumber );
		m_listModel->setItem( i, 1, itemName );
		m_listModel->setItem( i, 2, itemPosition );
	}
	
	//m_listModel->setStringList( sl );
	//m_listView->setEditTriggers ( QAbstractItemView::NoEditTriggers );
	m_listView->setSelectionMode( QAbstractItemView::SingleSelection );
	m_listView->setModel( m_listModel );

	for( int column=0; column < 3; column++ )
		m_listView->resizeColumnToContents( column );
}

void SDMTensorProbeWidget
  ::updateProbes()
{
	assert( m_listModel->rowCount() == m_probes.size() );

	// Read back any changes made by the user in the UI to the list
	for( int i=0; i < m_listModel->rowCount(); i++ )
	{
		// Read probe name
		m_probes[i].name = m_listModel->item( i, 1 )->text().toStdString();
	}
}

QList<SDMTensorProbe> SDMTensorProbeWidget
  ::getProbes() /*const*/
{
	updateProbes();
	return m_probes;
}

SDMTensorProbe& SDMTensorProbeWidget
  ::getProbe( int idx )
{
	updateProbes();
	return m_probes[idx];
}
