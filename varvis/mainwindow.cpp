/* mainWindow.cpp */

// QT - INCLUDES
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QEvent>

// OWN - INCLUDES
#include "mainwindow.h"
#include "VarVisRender.h"
#include "GlyphControls.h"


// VTK - INCLUDES
#include <vtkActor.h>
#include <vtkCellArray.h>
#include <vtkDoubleArray.h>
#include <vtkElevationFilter.h>
#include <vtkFloatArray.h>
#include <vtkFieldDataToAttributeDataFilter.h>
#include <vtkLine.h>
#include <vtkPointData.h>
#include <vtkPointSource.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProbeFilter.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTransformToGrid.h>
#include <vtkGridTransform.h>
#include <vtkTriangle.h>
#include <vtkWarpScalar.h>
#include <vtkWarpVector.h>
#include <vtkPolyDataWriter.h>
#include <vtkPolyDataReader.h>
#include <QStatusBar>

using namespace std;

// Main Window Constructor 
MainWindow::MainWindow(QWidget *parent) :  QMainWindow(parent)
{
	QHBoxLayout* mainLayout=new QHBoxLayout;
//	m_baseDir.append("C:\\_Vitalis\\test_data\\");
	// create objects
	m_dockStationVolumeControls = new QDockWidget(tr("Volume Controls"),this);
	
	m_vtkWidget			= new VarVisWidget();
	m_vtkWidget_sync	= new VarVisWidget();
	
	m_dockStation		= new QDockWidget(tr("Controls"),this);
	
	m_controls			= new VarVisControls();
	m_centralTabWidget	= new QTabWidget();
	m_vren				= new VarVisRender();
	m_vren_sync			= new VarVisRender();

	m_volumeControls   = new VolumeControls();
	m_roiContorls      = new RoiControls();

	m_volumeControls->setVarVisRenderer(m_vren);
	m_roiContorls->setVarVisRenderer(m_vren);

	// setup vtkWidget 
	m_vtkWidget->GetRenderWindow()->AddRenderer(m_vren->getRenderer());
	m_vtkWidget->GetRenderWindow()->SetLineSmoothing( 1 );
	m_vren->getRenderer()->SetBackground(0.8,0.8,0.8);
	m_vren->setRenderWindow(m_vtkWidget->GetRenderWindow());

	
	// some hardCoded Paths for varvis StandAlone 
	m_baseDir="C:\\_vitalis\\test_data";

	m_controls->setPathToMask(m_baseDir);
	m_roiContorls->setPathToMask(m_baseDir);
	m_controls->setVRen(m_vren);
	m_vren->setElementSpacing(0.0,0.0,0.0);

	// setup MainWindow
	m_centralTabWidget->addTab( m_vtkWidget, tr("Vtk Widget")  ); // holds whole pca model
	addDockWidget( Qt::RightDockWidgetArea, m_dockStation );
	m_dockStation->setWidget(m_controls);

	addDockWidget( Qt::LeftDockWidgetArea, m_dockStationVolumeControls );
	m_dockStationVolumeControls->setWidget(m_roiContorls);
	/*
	addDockWidget( Qt::LeftDockWidgetArea, m_dockStationVolumeControls );
	m_dockStationVolumeControls->setWidget(m_volumeControls);
	*/
	setCentralWidget( m_centralTabWidget );
	createActions();

	m_vren->setWarpVis(0);
	m_vren->useGaussion(true);
	m_vtkWidget->setVren(m_vren);

	m_vren_sync->syncWith(m_vren);
	m_vtkWidget_sync->update();
	
	resize(600,600);
	connect(m_vren,SIGNAL(statusMessage(QString)),this,SLOT(statusMessage(QString)));
	connect(m_volumeControls,SIGNAL(StatusMessage(QString)),this,SLOT(statusMessage(QString)));
	statusBar()->showMessage("Ready");	

		// okay lets create a camera 
	VTKPTR<vtkCamera> masterCamera = VTKPTR<vtkCamera> ::New();
	
	m_vren->getRenderer()->SetActiveCamera(masterCamera);

}

void MainWindow::statusMessage(QString message)
{
	// show the message directly
	statusBar()->showMessage(message,0); 
}
// -- actions --	
void MainWindow::createActions()
{
	// -- actions --	
	actOpen = new QAction( tr("&Open volume dataset..."), this );
	actOpen->setShortcut( tr("Ctrl+O") );
	actOpen->setStatusTip( tr("Open volume dataset from disk.") );
	
	actOpenWarp = new QAction( tr("Open warpfield dataset..."), this );
	actOpenWarp->setShortcut( tr("Ctrl+W") );
	
	actSaveMesh= new QAction( tr("Save Mesh ..."), this );
	actSaveMesh->setShortcut( tr("Ctrl+S") );
	actLoadMesh= new QAction( tr("Load Mesh ..."), this );
	actLoadMesh->setShortcut( tr("Ctrl+L") );

	// -- menuFile entries --
	QMenu *menuFile;
	menuFile = menuBar()->addMenu( tr("&File") );
	menuFile->addAction( actOpen );
	menuFile->addAction( actOpenWarp );
	menuFile->addAction(actSaveMesh);
	menuFile->addAction(actLoadMesh);

	// -- connections --
	connect( actOpen,     SIGNAL(triggered()), this, SLOT(openVolumeDataset()));
	connect( actOpenWarp, SIGNAL(triggered()), this, SLOT(openWarpDataset()));
	connect( actSaveMesh, SIGNAL(triggered()), this, SLOT(saveMesh()));
	connect( actLoadMesh, SIGNAL(triggered()), this, SLOT(loadMesh()));
	
}

void MainWindow::saveMesh()
{
	QString filename;
	filename = QFileDialog::getSaveFileName( this,
		tr("Save Mesh "),
		m_baseDir, tr("Volume description file (*.msh)") );

	// cancelled?
	if( filename.isEmpty() )
		return;

	VTKPTR<vtkPolyDataWriter> meshWriter=VTKPTR<vtkPolyDataWriter>::New();
	meshWriter->SetInput(m_vren->getMesh());
	meshWriter->SetFileName(filename.toAscii());
	meshWriter->Write();


}

void MainWindow::loadMesh()
{
	
	m_vren->generateAdvancedDiffContour();
	//m_vren->clusterVolumeGlyphs();
	//m_vren->getRenderWindow()->Render();
}
// -- setWarp -- 
bool MainWindow::setWarp( QString mhdFilename )
{
	m_vren->setWarp(mhdFilename, m_controls->get_BackwardsTr_State());
	return true;
}

// -- open Warp Data -- 
void MainWindow::openWarpDataset()
{
	QString filename;
	filename = QFileDialog::getOpenFileName( this,
		tr("Open warpfield dataset"),
		m_baseDir, tr("Volume description file (*.mhd)") );

	// cancelled?
	if( filename.isEmpty() )
		return;

	// extract absolute path and filename
	QFileInfo info( filename );
	m_baseDir = info.absolutePath(); // name is info.fileName()	

	// replace current volume with temporary one
	bool success = setWarp( filename );
	m_vtkWidget->update();
}

// -- setVolume --
bool MainWindow::setVolume( QString mhdFilename )
{
	if( !m_vren->load_reference( mhdFilename.toAscii() ) )  // was: filename.c_str()
	{
		QMessageBox::warning( this, tr("Error setting volume"),
			tr("Error: Could not setup volume rendering correctly!") );
		return false;
	}

	m_vren->setVolumeVisibility(false);
	//m_vren->getRenderer()->ResetCamera();
	//m_vren->getRenderer()->GetActiveCamera()->Azimuth(-90);
	return true;
}

// -- open Volume Data -- 
void MainWindow::openVolumeDataset()
{
	QString filename;
	filename = QFileDialog::getOpenFileName( this,
		tr("Open volume dataset"),
		m_baseDir, tr("Volume description file (*.mhd)") );

	// cancelled?
	if( filename.isEmpty() )
		return;
	
	m_vren->setGaussionRadius(m_controls->getGaussionRadius());
	m_vren->setNumberOfSamplingPoints(m_controls->getSampleRange());
	m_vren->setPointSize(m_controls->getSampleRadius());

	setVolume(filename);
	m_vtkWidget->update();
	
	
}

//-- Main Window Destructor --
MainWindow::~MainWindow()
{

}

