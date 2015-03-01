#include <QtGui>
#include <iostream>
#include "BatchProcessingDialog.h"

BatchProcessingDialog::BatchProcessingDialog( QWidget* parent )
	: QDialog(parent),
	  m_curProc(-1)
{
	m_cmdTree = new QTreeWidget;
	m_cmdTree->setColumnCount( 4 );
	m_cmdTree->setHeaderLabels( QStringList() 
		<< "Run?" << "Command" << "Description" << "Status" );

	// -- UI ------------------------------------------------------------------
	
	QFont infoFont("Courier",8);
	QFont conFont("Courier",8);
	
	
	m_progressBar= new QProgressBar(this);
	
	m_progressBar->setTextVisible(false);
	m_progressBar->setValue(0);
	

	m_con  = new QTextEdit;
	m_con->setCurrentFont( conFont );
	//m_con->setLineWidth( 120 );
	m_con->setLineWrapMode( QTextEdit::NoWrap );
	m_con->setReadOnly( true );
	m_con->setTextColor( QColor(64,128,128) );
	
	m_info = new QTextEdit;
	m_info->setCurrentFont( infoFont );
	m_info->setReadOnly( true );
	
	QLabel* conLabel  = new QLabel( tr("Console log") );
	conLabel->setBuddy( m_con );
	QLabel* infoLabel = new QLabel( tr("Batch commands") );
	infoLabel->setBuddy( m_info );
	
	QVBoxLayout* lcon = new QVBoxLayout;
	lcon->addWidget( conLabel );
	lcon->addWidget( m_con );
	
	QVBoxLayout* linfo = new QVBoxLayout;
	linfo->addWidget( infoLabel );
	linfo->addWidget( m_cmdTree ); // was: m_info
	
	QPushButton* butStart  = new QPushButton(tr("Start"));
	QPushButton* butCancel = new QPushButton(tr("Cancel"));
	QPushButton* butSaveLog= new QPushButton(tr("Save Log"));
	//butCancel->setEnabled( false );

	QHBoxLayout *lbut = new QHBoxLayout;
	lbut->addWidget( butStart  );
	lbut->addWidget( butCancel );
	lbut->addWidget( butSaveLog );

	// tabbed layout

	// combine in widgets for tabs
	QWidget* w0 = new QWidget;	w0->setLayout( linfo );
	QWidget* w1 = new QWidget;  w1->setLayout( lcon );
	QTabWidget* tabWidget = new QTabWidget;
	tabWidget->addTab( w0, tr("Command batch") );
	tabWidget->addTab( w1, tr("Console output") );

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget( tabWidget );
	layout->addLayout( lbut );
	
	layout->addWidget(m_progressBar);
	setLayout( layout );
	
	setWindowTitle( tr("Batch processing") );
	
	// -- process -------------------------------------------------------------
	
	m_proc = new QProcess( this );
	connect( m_proc, SIGNAL(readyReadStandardOutput()), this, SLOT(updateConsole()) );
	connect( m_proc, SIGNAL(readyReadStandardError ()), this, SLOT(updateConsole()) );
	connect( m_proc, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(procFinished(int,QProcess::ExitStatus)) );
	connect( m_proc, SIGNAL(started()), this, SLOT(procStarted()) );
	connect( m_proc, SIGNAL(error(QProcess::ProcessError)), this, SLOT(procError(QProcess::ProcessError)) );

	connect( butStart , SIGNAL(clicked()), this, SLOT(start()) );
	connect( butCancel, SIGNAL(clicked()), this, SLOT(reset()) );
	connect( butSaveLog,SIGNAL(clicked()), this, SLOT(saveLog()) );
	this->resize(600,400);
}

void BatchProcessingDialog::initBatch( QList<BatchCommand> batch )
{
	m_batch = batch;
	m_curProc = -1;
	
	m_info->clear();
	for( int i=0; i < m_batch.size(); ++i )
		m_info->append( tr("[Command %1:] ").arg(i+1) + m_batch[i].toString() );

	m_cmdTree->clear();
	for( int i=0; i < m_batch.size(); ++i )
	{
		QTreeWidgetItem* item = new
			QTreeWidgetItem( (QTreeWidget*)0, QStringList() 
				<< ""                         // run?
				<< m_batch[i].toString("\n")  // command
				<< m_batch[i].desc            // description
				<< ""                         // status
			  );
		item->setToolTip( 1, m_batch[i].toString() );
		
		m_cmdTree->insertTopLevelItem( i, item );
		
		QCheckBox* runCheckBox = new QCheckBox;
		runCheckBox->setChecked( m_batch[i].run );
		m_cmdTree->setItemWidget( item, 0, runCheckBox );
		connect( runCheckBox, SIGNAL(stateChanged(int)), this, SLOT(updateRunState()) );

		// DEBUG
		std::cout << "DEBUG: batch["<<i<<"] " << m_batch[i].toString().toStdString() << std::endl;
	}
}

#define DEBUG_BPD_BATCH(MARKER) \
	for(int i=0;i<m_batch.size();++i) std::cout<<MARKER<<std::endl<<"m_batch["<<i<<"].run="<<(m_batch[i].run?"true":"false")<<std::endl;

void BatchProcessingDialog::updateRunState()
{
	for( int i=0; i < m_cmdTree->topLevelItemCount(); ++i ) {
		QTreeWidgetItem* item = m_cmdTree->topLevelItem(i);
		QCheckBox* runCheckBox = qobject_cast<QCheckBox*>(
			m_cmdTree->itemWidget( item, 0 ) );
		if( runCheckBox )
			m_batch[i].run = runCheckBox->isChecked();
	}

	DEBUG_BPD_BATCH("BatchProcessingDialog::updateRunState()")
}

void BatchProcessingDialog::updateCmdStatus( int i, QString status )
{
	QTreeWidgetItem* item = m_cmdTree->topLevelItem(i);
	if( item )
		item->setText( 3, status );	
}

bool BatchProcessingDialog::getNextCommand( BatchCommand& cmd, int& index )
{
	index++;
	if( index >= m_batch.size() )
		return false;

	DEBUG_BPD_BATCH("BatchProcessingDialog::getNextCommand()")

	cmd = m_batch[index];
	while( !cmd.run && (index < m_batch.size()-1) )
	{
		m_con->append( tr("### Skipping command %1 because it is disabled ###").arg(index+1) );
		updateCmdStatus( index, tr("skipped") );
		index++;
		cmd = m_batch[index];
	}
	if( !cmd.run )
	{
		m_con->append( tr("### Skipping command %1 because it is disabled ###").arg(index+1) );
		updateCmdStatus( index, tr("skipped") );
	}
	return cmd.run;
}

void BatchProcessingDialog::start()
{
	DEBUG_BPD_BATCH("BatchProcessingDialog::start()")
m_progressBar->setMaximum(100);	
m_progressBar->setMinimum(0);
m_progressBar->setValue(0);

	if( m_batch.isEmpty() )
		return;

	m_curProc = -1;
	BatchCommand curCmd;
	if( !getNextCommand( curCmd, m_curProc ) )
	{
		m_con->append( tr("#####################################################\n"
						  "##  No commands enabled!  \n"
						  "#####################################################") );
		return;
	}

	m_proc->start( curCmd.prog, curCmd.args );	
	if( !m_proc->waitForStarted() )
	{
		updateCmdStatus( m_curProc, tr("error!") );
		m_con->append( tr("#####################################################\n"
						  "##  First command could not be started!  \n"
						  "#####################################################") );
		return;
	}
	setWindowTitle( tr("Batch processing (running)") );
}

void BatchProcessingDialog::reset()
{
	m_proc->kill();
	m_con->append( tr("#####################################################\n"
		              "##  Batch processing RESET  \n"
	                  "#####################################################") );	
	m_info->clear();
	m_batch.clear();
	m_curProc = -1;
	m_crashed = false;
	
	m_cmdTree->clear();
	
	setWindowTitle( tr("Batch processing") );
}

void BatchProcessingDialog::updateConsole()
{
	QByteArray sout = m_proc->readAllStandardOutput(),
	           eout = m_proc->readAllStandardError();
	
	// TODO: color standard and error output differently
	m_con->setTextColor( QColor(0,255,0) );
	m_con->append( sout );
	m_con->setTextColor( QColor(255,0,0) );
	m_con->append( eout );
	m_con->setTextColor( QColor(64,128,128) );
}

void BatchProcessingDialog::procFinished( int exitCode, QProcess::ExitStatus exitStatus )
{
	m_con->append( tr("#####################################################\n"
		              "##  Process %1 finished with exit code %2  \n"
	                  "#####################################################")
					  .arg(m_curProc+1)
					  .arg(exitCode) );


	

	if( exitStatus == QProcess::Crashed )
	{


		updateCmdStatus( m_curProc, tr("crashed!") );
		m_con->append( tr("#####################################################\n"
						  "##  Last process CRASHED! Aborting batch processing! \n"
						  "#####################################################") );
		reset();
		m_crashed = true;
		emit crashed();
		emit finished();
		m_progressBar->setValue(0);

		QMessageBox::warning(this,tr("ERROR"),tr("Error : Last process CRASHED! Aborting batch processing !"));

	}

	updateCmdStatus( m_curProc, tr("finished") );

	BatchCommand cmd;
	if( !getNextCommand( cmd, m_curProc ) )
	{		
		m_con->append( tr("#####################################################\n"
						  "##  Batch processing FINISHED  \n"
						  "#####################################################") );
		
		// do nothing, requires user to reset() manually before starting next batch job
		setWindowTitle( tr("Batch processing (finished)") );
		
		if (m_crashed)
		{
			emit finished();
			m_progressBar->setValue(0);
		}
		else
		{
			m_crashed = false; // why false
			emit finished();
			m_progressBar->setMaximum(100);
			m_progressBar->setValue(100);
			QMessageBox::warning(this,tr("Finished"),tr("Batch processing FINISHED !"));
			m_progressBar->setValue(0);
			this->close(); 
		

		}
		
		//emit mhdFinished();
	}
	else
	{
		// start next process
		int oldValue=m_progressBar->value();
		int step=100/m_batch.size();
		m_progressBar->setValue((oldValue+step));
		m_proc->start( cmd.prog, cmd.args );
	}
}

void BatchProcessingDialog::procStarted()
{
	m_con->append( tr("#####################################################\n"
		              "##  Process %1 / %2 started \n"
					  "##  %3 \n"
	                  "#####################################################")
					  .arg(m_curProc+1)
					  .arg(m_batch.size())
					  .arg(m_batch[m_curProc].desc) );

	updateCmdStatus( m_curProc, tr("started...") );
	m_con->append( QString("> ") + m_batch[m_curProc].toString() );
}

void BatchProcessingDialog::procError( QProcess::ProcessError err )
{
	updateCmdStatus( m_curProc, tr("error!") );
	m_con->append( tr("#####################################################\n"
		              "##  Error on process %1! \n"
					  "##  \n"
	                  "#####################################################")
					  .arg(m_curProc+1)
					  );
}

void BatchProcessingDialog::printConsole( QString msg )
{
	m_con->setTextColor( QColor(255,100,100) );
	m_con->append( msg );
}

QProcess* BatchProcessingDialog::process()
{
	return m_proc;
}


QString BatchProcessingDialog::BatchCommand::toString( QString separator )
{
	QString cmd = prog;
	for( int i=0; i < args.size(); ++i )
		cmd = cmd + QString(" ") + args[i];
	return cmd;
}

void BatchProcessingDialog::saveLog()
{
	QString filename = QFileDialog::getSaveFileName( this, tr("Save log..."),
		"", tr("Text file (*.txt)") );
	if( filename.isEmpty() )
		return;

	QTextDocumentWriter writer( filename );
	if( !writer.write( m_con->document() ) )
	{
		QMessageBox::warning( this, tr("sdmvis: Error saving log"),
			tr("Could not save log to %1!").arg(filename) );
		return;
	}
}
