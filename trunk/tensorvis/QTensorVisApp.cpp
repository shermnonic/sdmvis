#include "QTensorVisApp.h"
#include "QTensorVisWidget.h"

#include <QtGui>
#include <QVTKWidget.h>
#include <vtkRenderWindow.h>

#include <iostream>

//------------------------------------------------------------------------------
//	D'tor
//------------------------------------------------------------------------------
QTensorVisApp::~QTensorVisApp()
{
	std::cout << "~QTensorVisApp()\n";
}

//------------------------------------------------------------------------------
//	C'tor
//------------------------------------------------------------------------------
QTensorVisApp::QTensorVisApp()
{
	setWindowTitle( APP_NAME );
	setWindowIcon( QIcon(QPixmap(":/data/icons/logo.png")) );
	
	// -- Visualizations --

	m_tensorVis = new QTensorVisWidget();

	// -- Layout --

	setCentralWidget( m_tensorVis ); 

	// -- Control dock with options--
	
	m_controlDock = new QDockWidget(tr("Controls"),this);
	m_controlDock->setWidget( m_tensorVis->getControlWidget() );
	this->addDockWidget( Qt::RightDockWidgetArea, m_controlDock );

	// -- Actions --

	QAction* actExportOBJ;
	QAction* actResetCamera;
	QAction* actQuit;

	actExportOBJ = new QAction( tr("Export &Geometry..."), this );
	actExportOBJ->setStatusTip( tr("Export visible geometry as Wavefront OBJ file to disk.") );
	connect( actExportOBJ, SIGNAL(triggered()), 
		m_tensorVis, SLOT(exportOBJ()) );

	actQuit = new QAction( tr("&Quit"), this );
	actQuit->setShortcut( tr("Ctrl+Q") );
	actQuit->setStatusTip( tr("Quit application.") );
	connect( actQuit, SIGNAL(triggered()), this, SLOT(close()) );

	actResetCamera = new QAction( tr("&Reset Camera"), this );
	actResetCamera->setShortcut( tr("Ctrl+R") );
	connect( actResetCamera, SIGNAL(triggered()), 
		m_tensorVis, SLOT(resetCamera()) );	
	
	// -- Connections --

	connect( m_tensorVis, SIGNAL(loadedFile(QString)), 
		this, SLOT(updateWindowTitle(QString)) );
	connect( m_tensorVis, SIGNAL(statusMessage(QString)), 
		this, SLOT(showStatusMessage(QString)) );

	// -- Menu --

	QMenu* menuFile;
	QMenu* menuEdit;
	QMenu* menuView;
	
	menuFile = menuBar()->addMenu( tr("&File") );
	menuFile->addActions( m_tensorVis->getFileActions() );
	menuFile->addSeparator();
	menuFile->addAction( actExportOBJ );
	menuFile->addSeparator();
	menuFile->addAction( actQuit );

	menuEdit = menuBar()->addMenu( tr("&Edit") );
	menuEdit->addActions( m_tensorVis->getEditActions() );

	menuView = menuBar()->addMenu( tr("&View") );
	menuView->addAction( m_controlDock->toggleViewAction() );
	menuView->addSeparator();
	menuView->addAction( actResetCamera );
	menuView->addSeparator();
	menuView->addActions( m_tensorVis->getViewActions() );

	// -- Finish up --

	// Read settings
	readSettings();
	// Push setttings to visualization(s)
	m_tensorVis->setOpenBaseDir( m_baseDir );

	showStatusMessage( tr("Ready") );
}

//------------------------------------------------------------------------------
//	onChangedFile()
//------------------------------------------------------------------------------
void QTensorVisApp::updateWindowTitle( QString s )
{
	if( !s.isEmpty() )
		setWindowTitle( APP_NAME + " - " + s );
	else
		setWindowTitle( APP_NAME );
}

//------------------------------------------------------------------------------
//	showStatusMessage()
//------------------------------------------------------------------------------
void QTensorVisApp::showStatusMessage( QString s )
{
	statusBar()->showMessage( s );
}

//------------------------------------------------------------------------------
//	closeEvent()
//------------------------------------------------------------------------------
void QTensorVisApp::closeEvent( QCloseEvent* /*event*/ )
{
	writeSettings();
}

//------------------------------------------------------------------------------
//	readSettings()
//------------------------------------------------------------------------------
void QTensorVisApp::readSettings()
{
	QSettings settings( APP_ORGANIZATION, APP_NAME );
	QPoint pos  = settings.value( "qtensorvis/pos", QPoint(200,200) ).toPoint();
	QSize  size = settings.value( "qtensorvis/size",QSize(640,480)  ).toSize();

	m_baseDir  = settings.value( "paths/baseDir", QString("../data/") ).toString();

	resize( size );
	move( pos );
}

//------------------------------------------------------------------------------
//	writeSettings()
//------------------------------------------------------------------------------
void QTensorVisApp::writeSettings()
{
	QSettings settings( APP_ORGANIZATION, APP_NAME );
	settings.setValue( "qtensorvis/pos",  pos()  );
	settings.setValue( "qtensorvis/size", size() );
	settings.setValue( "paths/baseDir" , m_baseDir  );
}
