#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Qt - INCLUDES
#include <QHBoxLayout>
#include <QMainWindow>
#include <QDockWidget>
#include <QTabWidget>
#include <QAction>
#include <QMouseEvent>

// OWN - INCLUDES
#include "VolumeControls.h"	
#include "varVisControls.h"
#include "RoiControls.h"
#include "VarVisWidget.h"
#include "VarVisRender.h"
#include "GlyphControls.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

	VarVisWidget	* m_vtkWidget_sync;		
	VarVisWidget	* m_vtkWidget;			// vtk Widet
	QDockWidget		* m_dockStation;		// DockWidet for Controls
	QDockWidget		* m_dockStationVolumeControls;		// DockWidet for Controls

	QTabWidget		* m_centralTabWidget;	// tab Widget 
	VarVisControls  * m_controls;			// controls widget
	VolumeControls  * m_volumeControls;     // controls for VolumeStuff
	GlyphControls   * m_glyphControls;
	RoiControls		* m_roiContorls;	
	QAction *actOpen;
	QAction *actOpenWarp;
	QAction *actSaveMesh;
	QAction *actLoadMesh;

private:
	QString m_baseDir;					// base Dir for data selection 
	VarVisRender * m_vren;
	VarVisRender * m_vren_sync;
	//GlyphVisualization* m_warpvis;    // OBSOLETE - safe to remove?
	 
	bool setVolume( QString mhdFilename );
	bool setWarp( QString mhdFilename );
	void createActions();

public slots:
	void openVolumeDataset	();
	void openWarpDataset	();
	void saveMesh			();
	void loadMesh			();
	void statusMessage(QString);

};
#endif
