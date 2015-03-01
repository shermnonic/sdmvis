#include "DatasetWidget.h"
#include <QtGui>

//==============================================================================
//	DatasetControlWidget
//==============================================================================

DatasetControlWidget::DatasetControlWidget( QWidget* parent )
: m_master(0)
{
	QPushButton* butSetWarps = new QPushButton(tr("Set Warpfields"));
	
	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget( butSetWarps );
	layout->addStretch( 15 );

	setLayout( layout );

	connect( butSetWarps, SIGNAL(clicked()), this, SLOT(setWarpfields()) );
}

void DatasetControlWidget::setWarpfields()
{
	if( !m_master )
		return;

	QStringList wnames = 
		QFileDialog::getOpenFileNames( this, tr("Select warpfields..."),
			"G:\\Work\\Analysis\\Mandibles_08_RM250_Gerbillinae\\logdemons4_2\\warps", 
			tr("Volume description file (*.mhd)") );	

	if( wnames.isEmpty() )
		return;

	// strip path and file extension (path should be the same for every item)
	QFileInfo fi0( wnames.at(0) );
	for( int i=0; i < wnames.size(); ++i ) 
	{
		QFileInfo fi( wnames.at(i) );
		wnames[i] = fi.baseName();

		if( fi.absolutePath() != fi0.absolutePath() )
			qWarning("Mismatch in warpfield file paths!");
	}

	m_master->setWarpFilenames( wnames );
}


//==============================================================================
//	DatasetWidget
//==============================================================================

DatasetWidget::DatasetWidget( QWidget* parent )
: QWidget(parent)
{
	// --- setup table model ---

	m_model = new QStandardItemModel(0,NumColumns);
	m_tableView = new QTableView;
	m_tableView->setModel( m_model );

	// make sure header index matches column index (see enum in header file)
	QStringList header; 
	header << tr("Name")        // 0 
		   << tr("Filename")    // 1
		   << tr("Reference")   // 2
		   << tr("Features")    // 3
		   << tr("Aligned")     // 4
		   << tr("Registered")  // 5
		   << tr("Warp")        // 6 
		   << tr("Error");      // 7
	m_model->setHorizontalHeaderLabels( header );

	// --- layout ---

	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget( m_tableView );
	layout->setContentsMargins( 0,0,0,0 );

	setLayout( layout );

	// --- control widget ---
	m_controlWidget = new DatasetControlWidget();
	m_controlWidget->setMaster( this );
}

void DatasetWidget::setNames( QStringList names )
{
	m_model->setRowCount( names.size() );
	for( int i=0; i < names.size(); ++i )
	{
		//QStandardItem* itemID = new QStandardItem( tr("%1").arg(i) );
		//itemID->setEditable( false );
		//m_model->setItem( i, 0, itemID );

		QStandardItem* itemName = new QStandardItem( names.at(i) );
		itemName->setEditable( false );
		m_model->setItem( i, ColName, itemName );
	}

	m_tableView->resizeColumnsToContents();
	m_tableView->resizeRowsToContents();
}

void DatasetWidget::setWarpFilenames( QStringList wnames )
{
	if( m_model->rowCount() == 0 )
	{
		// current model is empty, setup new one

		m_model->setRowCount( wnames.size() );
		for( int i=0; i < wnames.size(); ++i )
		{
			QString name = wnames.at(i);
			QRegExp qr("[^_]+_[^_]+_[^_]+_[^_]+"); // HACK: naming convention hardcoded!
			name.indexOf( qr );
			name = name.left( qr.matchedLength() );

			QString wname = wnames.at(i);
			wname = wname.right( wname.lastIndexOf('_') );

			QStandardItem* itemName = new QStandardItem( name );
			itemName->setEditable( false );

			QStandardItem* warpName = new QStandardItem( wname );
			warpName->setEditable( false );

			m_model->setItem( i, ColName, itemName );
			m_model->setItem( i, ColWarp, warpName );
		}
	}
	else
	{
		// merge with current (non-empty) model

		for( int i=0; i < m_model->rowCount(); ++i )
		{
			QString name = m_model->item( i, ColName )->text();

			QRegExp qr(name+"\\S*");
			int idx = wnames.indexOf(qr);

			if( idx == -1 )
			{
				// no warp corresponding to Name found
				//qWarning("No warp corresponding to "+name+" found");
			}
			else
			{
				QString wname = wnames.at(idx);
				wname = wname.remove( 0, wname.lastIndexOf('_')+1 );

				QStandardItem* warpName = new QStandardItem( wname );
				warpName->setEditable( false );

				m_model->setItem( i, ColWarp, warpName );
			}
		}
	}

	m_tableView->resizeColumnsToContents();
	m_tableView->resizeRowsToContents();
}
