#include "MainWindow.h"
#include "Viewer.h"

#include <QtGui>

const QString APP_NAME        ( "slvis" );
const QString APP_ORGANIZATION( "University Bonn Computer Graphics" );
#define       APP_ICON        QIcon(QPixmap(":/data/icon.png"))


MainWindow::MainWindow()
{
	// Create UI (widgets, menu, actions, connections)
	createUI();	

	// Load application settings
	readSettings();

	// Default startup
	statusBar()->showMessage( tr("Ready.") );
}

MainWindow::~MainWindow()
{
}

void MainWindow::createUI()
{
	setWindowTitle( APP_NAME );
	setWindowIcon( APP_ICON );
	
	// --- widgets ---
	
	m_viewer = new Viewer();
	setCentralWidget( m_viewer );

	// --- actions ---

	QAction
		*actQuit;
	
	actQuit = new QAction( tr("&Quit"), this );
	actQuit->setStatusTip( tr("Quit application.") );
	actQuit->setShortcut( tr("Ctrl+Q") );
	
	// --- build menu ---

	QMenu
		*menuFile,
		*menuViewer;

	menuFile = menuBar()->addMenu( tr("&File") );
	menuFile->addAction( actQuit );

	menuViewer = menuBar()->addMenu( tr("&Viewer") );
	menuViewer->addActions( m_viewer->getActions() );

	// --- connections ---

	connect( actQuit, SIGNAL(triggered()), this, SLOT(close()) );

}

void MainWindow::destroy()
{
	static bool destroyed = false;
	if( destroyed ) return;

	m_viewer->destroyGL();
	
	destroyed = true;
}

void MainWindow::closeEvent( QCloseEvent* event )
{
	destroy();
	writeSettings();
	QMainWindow::closeEvent( event );
}

void MainWindow::writeSettings()
{
	QSettings settings( APP_ORGANIZATION, APP_NAME );
	settings.setValue( "geometry"   , saveGeometry() );
	settings.setValue( "windowState", saveState()    );
	settings.setValue( "baseDir"    , m_baseDir      );
}

void MainWindow::readSettings()
{
	QSettings settings( APP_ORGANIZATION, APP_NAME );
	m_baseDir = settings.value( "baseDir", QString("../data/") ).toString();
	restoreGeometry( settings.value("geometry")   .toByteArray() );
	restoreState   ( settings.value("windowState").toByteArray() );
	// for dock widgets use restoreDockWidget( .. );
}
