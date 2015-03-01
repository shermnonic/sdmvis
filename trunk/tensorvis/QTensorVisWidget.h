#ifndef _QTENSORVISWIDGET_H
#define _QTENSORVISWIDGET_H

#include <QWidget>
#include <QList>
#ifndef Q_MOC_RUN 
// Workaround for BOOST_JOIN problem: Undef critical code for moc'ing.
// There is a bug in the Qt Moc application which leads to a parse error at the 
// BOOST_JOIN macro in Boost version 1.48 or greater.
// See also:
//	http://boost.2283326.n4.nabble.com/boost-1-48-Qt-and-Parse-error-at-quot-BOOST-JOIN-quot-error-td4084964.html
//	http://cgal-discuss.949826.n4.nabble.com/PATCH-prevent-Qt-s-moc-from-choking-on-BOOST-JOIN-td4081602.html
//	http://article.gmane.org/gmane.comp.lib.boost.user/71498
//#include "VisPrimitives.h"
#include "TensorVis2.h"
#include "TensorDataFileProvider.h"
#include "TensorNormalDistributionProvider.h"
#include "WeightedTensorDataProvider.h"
#include "VolVis.h"
#include "ColorMapRGB.h"
#include "ModeAnimationWidget.h"
#endif
#include <vtkRenderer.h>
#ifndef VTKPTR
#include <vtkSmartPointer.h>
#define VTKPTR vtkSmartPointer
#endif

//#define QTENSORVISWIDGET_USE_QVTKWIDGET2 // <- Not working, leads to a crash in vtkRendering.dll!!
#ifdef QTENSORVISWIDGET_USE_QVTKWIDGET2
class QVTKWidget2;
#endif

// Forwards
class QDockWidget;
class QVTKWidget;
class QTensorVisOptionsWidget;
class QTimer;
class QAction;

/// Qt4 widget for Tensor Visualization of a Statistical Deformation Model
class QTensorVisWidget : public QWidget
{
	Q_OBJECT

signals:
	void statusMessage( QString );
	void loadedFile( QString );

public:
	QTensorVisWidget( QWidget* parent=NULL );
	virtual ~QTensorVisWidget();

public slots:
	void reset();
	void resetCamera();

	virtual QWidget* getControlWidget() { return (QWidget*)m_optionsWidget; }
	
	QList<QAction*> getFileActions() { return m_fileActions; }
	QList<QAction*> getViewActions() { return m_viewActions; }
	QList<QAction*> getEditActions() { return m_editActions; }
	
	/// Show file dialog and let the user select a volume dataset.
	/// Provided for convenience, internally \a setVolume() is called.
	void openVolumeDataset();
	/// Show file dialog and let the user select a tensor field dataset.
	/// Provided for convenience, internally \a setTensorDataset() is called.
	void openTensorDataset();
	/// Open a second tensor dataset, used for weighting the first one.
	void openTensorDataset2();
	/// Open tensor distribution dataset
	void openTensorDistribution();
	/// Set default directory for openXxx() commands, provided for convenience.
	void setOpenBaseDir( QString path ) { m_baseDir = path; }

	/// Show file dialog and let the user select a RGB color map textfile.
	void openColorMap();
	bool setColorMap( QString filename );


	// Not implemented:
	//void openDeformationFields();

	void saveTensorDataset();
	bool saveTensorDataset( QString filename );

	/// Save current view as PNG image, shows file dialog first.
	void saveScreenshot();
	/// Save current view as PNG image with given filename.
	void saveScreenshot( QString filename );	
	/// Export currently visible PolyData as Wavefront OBJ, shows file dialog.
	void exportOBJ();
	/// Export currently visible PolyData as Wavefront OBJ with given filename.
	void exportOBJ( QString filename );

	/// Show dialog to create mode animation.
	void showModeAnimationDialog();

	// WORKAROUND for missing super visualization and GUI class framework.
	//            See also related comment in C'tor.
	void toggleVolVis   ( bool );
	void toggleTensorVis( bool );
	void toggleAnimationTimer( bool );

	bool setTensorDataset( QString filename );
	bool setTensorDataset2( QString filename );
	bool setTensorDistribution( QString filename );
	bool setVolume       ( QString mhdFilename );
	void setVolume( VTKPTR<vtkImageData> volume );

protected slots:
	void redraw();

	void updateTensorVis();

	void updateColorMap();

	virtual void setTensorDataProvider( TensorDataProvider* m_tdataProvider );

	void changeBackgroundColors();

	void forceRedraw();

	void bakeModeAnimation();
	void renderModeAnimation();
	void makeModeAnimation( bool saveToDisk, bool allModes=false );

//protected:
public:
	vtkRenderer* getRenderer() { return m_renderer; }

	TensorVisBase* getTensorVis() { return &m_tvis; }

private:
	VolVis     m_volvis; ///< Volume visualization
	TensorVis2 m_tvis;   ///< Tensor visualization

	bool m_hasVolume;

	ColorMapRGB	m_colormap;
	
	TensorDataProvider*        m_tdataProvider;     // current tensor field
	TensorDataFileProvider     m_tdataFileProvider; // file 1
	TensorDataFileProvider     m_tdataFileProvider2;// file 2, used as weights
	WeightedTensorDataProvider m_tdataWeightedProvider; // weighted field 1 with 2
	TensorNormalDistributionProvider m_tdataGaussianProvider; // tensor distribution

	QList<QAction*> m_fileActions;
	QList<QAction*> m_viewActions;
	QList<QAction*> m_editActions;

	QTimer* m_animationTimer;
	QAction* m_animationTimerAction;

	ModeAnimationWidget* m_modeAnimationWidget;

	///@{ Widgets
#ifdef QTENSORVISWIDGET_USE_QVTKWIDGET2
	QVTKWidget2*             m_vtkWidget;
#else
	QVTKWidget*              m_vtkWidget;
#endif
	QTensorVisOptionsWidget* m_optionsWidget;
	QDockWidget*             m_controlDock;
	///@}

	/// VTK Renderer attached to m_vtkWidget
	VTKPTR<vtkRenderer> m_renderer;

	///@{ Application settings
	QString m_baseDir;
	///@}
};

#endif // _QTENSORVISWIDGET_H
