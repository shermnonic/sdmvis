#ifndef VTKVISWIDGET_H
#define VTKVISWIDGET_H

#include <QWidget>
#include "VTKVisPrimitives.h"
class QVTKWidget;
class QAction;

class VTKVisWidget : public QWidget
{
	Q_OBJECT
public:
	VTKVisWidget( QWidget* parent=NULL );

	QList<QAction*> getActions() { return m_actions; }

public slots:
	void resetCamera();
	void openVolumeDataset ();  // open reference volume
	void openWarpDataset();
	void saveIsosurface();

	void setBaseDir( QString dir ) { m_baseDir = dir; }

protected slots:
	///@{ Forward signals to VRen/WarpVis (which don't support Qt signals)
	// VRen
	void toggleVolume ( bool visible );
	void toggleContour( bool visible );
	void toggleOutline( bool visible );
	void toggleAxes   ( bool visible );
	void toggleGrid   ( bool visible );
	// VectorfieldGlyphVisualization2
	void toggleWarpVis( bool visible );
	void toggleWarpLegend( bool visible );
	///@}
	void changeGlyphScale();
	void changeIsovalue();
	void changeGridsize();
	void updateActions();

protected:
	bool setVolume( QString mhdFilename );
	bool setWarp  ( QString mhdFilename );

	///@{ GUI actions
	        // file
	QAction *actOpen,
	        *actOpenWarp,
	        *actSaveIsosurface,
	        // view
	        *actToggleVolume,
			*actToggleWarpVis,
			*actToggleWarpLegend,
	        *actToggleContour,
	        *actToggleAxes,
	        *actToggleGrid,
	        *actToggleOutline,
	        *actIsovalue,
	        *actGridsize,
			*actGlyphScale,
	        *actResetCamera;
	///@}

	QList<QAction*> m_actions;
	QString m_baseDir;

private:
	VRen2 m_vren;                /// volume visualizer
	VectorfieldGlyphVisualization2* m_warpvis;
	QVTKWidget* m_vtkWidget;	
	int m_selvol;
};

#endif // VTKVISWIDGET_H
