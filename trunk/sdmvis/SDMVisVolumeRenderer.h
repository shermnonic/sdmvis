#ifndef SDMVISVOLUMERENDERER_H
#define SDMVISVOLUMERENDERER_H

//#include <GL/glew.h>
#include <GL/GLConfig.h>
#include <QGLWidget>
#include <QGroupBox>
#include <QList>
#include <QPoint>
#include <QVector3D>
#include <qmatrix4x4.h>
#include <GL/GLTexture.h>

#ifndef Q_MOC_RUN 
// Workaround for BOOST_JOIN problem: Undef critical code for moc'ing.
// There is a bug in the Qt Moc application which leads to a parse error at the 
// BOOST_JOIN macro in Boost version 1.48 or greater.
// See also:
//	http://boost.2283326.n4.nabble.com/boost-1-48-Qt-and-Parse-error-at-quot-BOOST-JOIN-quot-error-td4084964.html
//	http://cgal-discuss.949826.n4.nabble.com/PATCH-prevent-Qt-s-moc-from-choking-on-BOOST-JOIN-td4081602.html
//	http://article.gmane.org/gmane.comp.lib.boost.user/71498
#include "VolumeManager.h"
#include "Warpfield.h"
#include "BarPlotWidget.h"
#include "SphereSelection.h"
#include "SDMVisInteractiveEditing.h"
#include "mattools.h"
#include "e7/Engines/Trackball2.h"
#include "e7/VolumeRendering/RayPickingInfo.h"
#include "StatisticalDeformationModel.h"
#endif

#ifdef SDMVIS_USE_OVERDRAW
#include "OverdrawQGLWidget.h"
#endif


// external classes
class VolumeDataHeader;
class VolumeRendererRaycast;
class BatchProcessingDialog;
class VolumeDataHeader;
class SDMVisConfig;
class SDMVisInteractiveEditingOptionsWidget;

// forward declarations
class SDMVisVolumeRendererOptionsWidget;
class SDMVisVolumeRendererControlWidget;
class SDMVisVolumeRenderer;


//------------------------------------------------------------------------------
//	Camera
//------------------------------------------------------------------------------

/// Simple camera for \a SDMVisVolumeRenderer, based on \a Trackball2
class Camera
{
public:
	Camera() 
		{}
	Camera( Trackball2 tb2, double fov )
		: m_trackball2(tb2),
		  m_fov(fov)
		{}

	//QMatrix4x4 getModelviewMatrix() const { return m_camMatrix; }
	double     getFOV()        const { return m_fov; }
	Trackball2 getTrackball2() const { return m_trackball2; }

private:
	// Trackball
	Trackball2 m_trackball2;
	// Further camera parameters
	double m_fov;
};


//==============================================================================
//	SDMVisVolumeRendererOptionsWidget
//==============================================================================

class QSlider;
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QStackedWidget;
class QPushButton;

/// Widget with rendering options for \a SDMVisVolumeRenderer
class SDMVisVolumeRendererOptionsWidget : public QGroupBox
{
	Q_OBJECT

	friend class SDMVisVolumeRendererControlWidget;

signals:
	void isoValueChanged( double d );
	void alphaValueChanged( double d );
	void renderModeChanged( int i );
	void stepSizeChanged( double d );

public:
	SDMVisVolumeRendererOptionsWidget( QWidget* parent=0 );

protected slots:
	void isoValueChanged( int i );
	void alphaValueChanged( int i );
	void dispatchRenderMode( int i );

protected:
	void connectMaster( SDMVisVolumeRenderer* master );
	void setSliderValueRelative( QSlider* slider, double value );

private:
	QSlider* m_isoSlider;
	QSlider* m_alphaSlider;
	QDoubleSpinBox* m_stepSizeSpinBox;
	QComboBox*      modeCombo;
	QStackedWidget* modeStack;
	QComboBox* m_integratorCombo;
	QSpinBox*  m_integratorStepsSpinBox;
	QPushButton* m_screenshotButton;
};

//==============================================================================
//	SDMVisVolumeRendererControlWidget
//==============================================================================
class PlotWidget;
class TraitSelectionWidget;

/// Widget with scatter plot view for \a SDMVisVolumeRenderer
class SDMVisVolumeRendererControlWidget : public QWidget
{
	Q_OBJECT

	friend class SDMVisVolumeRenderer;

public:
	SDMVisVolumeRendererControlWidget( QWidget* parent=0 );

	BarPlotWidget*        getBarPlotWidget() { return m_bars; }
	PlotWidget*           getPlotWidget   () { return m_plotter; }
	TraitSelectionWidget* getTraitSelector() { return m_traitSelector; }

public slots:
	void updateLambdas();
	
protected:
	void connectMaster( SDMVisVolumeRenderer* master );

private:
	BarPlotWidget*        m_bars;
	PlotWidget*           m_plotter;
	TraitSelectionWidget* m_traitSelector;
	
	SDMVisVolumeRendererOptionsWidget* m_vrenOpts;
	SDMVisVolumeRenderer* m_master;
};


//==============================================================================
//	SDMVisVolumeRenderer
//==============================================================================

/// Raycaster for a statistical deformation model
/// \sa SDMVisVolumeRendererControlWidget
/// \sa SDMVisVolumeRendererOptionsWidget
/// \sa Camera
#ifdef SDMVIS_USE_OVERDRAW
class SDMVisVolumeRenderer : public OverdrawQGLWidget // was: QGLWidget
#else
class SDMVisVolumeRenderer : public QGLWidget
#endif
{
	Q_OBJECT

signals:
	void lambdasChangedByAnimation();
	void pickedRay( RayPickingInfo );
	
public:	
	enum InteractionMode { 
		ModeTrackball, 
		ModeSelectROI,
		ModePicking,
		NumInteractionModes
	};
	enum ColorMode { ColorPlain, ColorCustom, ColorTexture };
	
	SDMVisVolumeRenderer( QWidget* parent=0, const QGLWidget* shareWidget=0, Qt::WindowFlags f=0 );
	virtual ~SDMVisVolumeRenderer();
	
	bool loadVolume( QString mhdFilename );

	///@{ Get actions and widgets
	QList<QAction*> getActionsRenderer() { return m_actionsRenderer; }
	QList<QAction*> getActionsDebug   () { return m_actionsDebug;    }
	QList<QAction*> getActionsAnalysis() { return m_actionsAnalysis; }
	QAction* getActionModeTrackball() { return m_actModeTrackball; }
	QAction* getActionModeRubberbandEditing() { return m_actModeRubberbandEditing; }
	SDMVisVolumeRendererControlWidget* getControlWidget() { return myControlWidget; }	
	///@}

	double getIsovalueRelative() const;
	double getAlphaRelative() const;
	double getStepSize() const;
	int    getRenderMode() const;
	int    getIntegrator() const;
	int    getIntegratorSteps() const;

	void setColorMode( ColorMode mode ) { m_colormode = mode; }
	int  getColorMode() const { return m_colormode; }

	/// Get lambda coefficient for warpfield i
	double getLambda( int i ) const;
	/// Convenience function, return vector with lambda coefficients
	QVector<double> getLambdas() const;

	/// Set working directory for batch processes
	void setWorkingDirectory( QString path ) { m_baseDir = path; }
	
	const VolumeDataHeader* getVolumeData(){return getAnyWarpfield();} // FIXME: irritating function name?!

	void setModeString( QString ms ) { m_modeString = ms; }

public slots:
	///@{ Config setters
	// TODO: Instead of separate functions/members for warpfields and names we could
	//       also store a SDMVisMainWindow::Config and use a single updateConfig()
	//       function/slot.
	//       Alternatively a global SDMVisConfig* could be made available to all
	//       modules which need access (would require signal/slot actions on 
	//       modification of the global config to update all depending modules).
	bool setWarpfields( const QList<Warpfield>& warpfields );
	bool setMeanwarp( const Warpfield& meanwarp );
	void setScale( int idx, double value );  // selective update of config param

	bool setWarpfieldsFromSDM( unsigned numModes );

	/// Assummes that the given config pointer remains valid!
	/// If warpfields have already been set by \a setWarpfields() loading them
	/// again can be prevented by specifying update=false.
	void setConfig( SDMVisConfig* config, bool update=true );
	///@}

	void setSDM( StatisticalDeformationModel* sdm );

	void setIsovalueRelative( double iso );
	void setAlphaRelative( double alpha );
	void setStepSize( double step );
	void setRenderMode( int mode );
	void setIntegrator( int type );
	void setIntegratorSteps( int steps );

	/// Set lambda coefficient for warpfield i
	void setLambda( int i, double lambda );
	void setLambdas( QVector<double> lambdas );

	///@{ Camera functions
	Camera getCamera();
	void setCamera(Camera cam);
	void saveCamera();
	void saveCamera( QString filename );
	void loadCamera();
	void loadCamera( QString filename );
	///@}

	void toggleOffscreen( bool checked );

	void showEditDialog();

	void makeScreenshot( QString filename );
	void makeScreenshot();

	// This one-liner is shown inside the rendering
	void setTitle( QString title );

protected slots:
	bool reinitShader();
	void startAnimation();
	void stopAnimation();
	void toggleOverlay( bool visible );
	void changeGradientColors();
	void toggleDebug( bool checked );
	void triggerMode( QAction* );
	void toggleBBox( bool visible );
	
	///@{ ROI
	void toggleROI( bool visible );
	void exportROI();
	void specifyROI();
	///@}

	///@{ Warp animation 
	void animateWarp();
	void toggleAnimateWarp( bool enable );
	void changeWarpAnimationSpeed();
	void changeWarpAnimationWarp();
	///@}

	void setOutputType( QAction* act );

protected:
#ifdef SDMVIS_USE_OVERDRAW
	// Overdraw implementation
	void overdraw( QPainter& painter );
#endif

	void setInitialized( bool b );
	void downloadVolume( VolumeDataHeader* vol, void* dataptr );
	///@{ ROI
	void saveROI( QString filename );
	///@}

	///@{ QGLWidget implementation
	void initializeGL();
	void resizeGL( int w, int h );
	void paintGL();
	QTimer* m_animationTimer; // TODO: refactor animation here to update or something unambigious to the warp animation stuff
	///@}

	///@{ Mouse events
	virtual void mousePressEvent  ( QMouseEvent* e );
	virtual void mouseReleaseEvent( QMouseEvent* e );
	virtual void mouseMoveEvent   ( QMouseEvent* e );
	virtual void wheelEvent       ( QWheelEvent* e );
	///@}
	
	void drawBackground();
	void drawEditArrow( QVector3D p, QVector3D q );

	void resetShader();

	///@{ LOD
	enum { FastRendering, QualityRendering };
	void setLOD( int level );
public slots:
	void setLODUpdate( bool b ) { setLOD( b ? FastRendering : QualityRendering ); }
	///@}

private:
	void invokeRenderUpdate();

	void lockRenderUpdate()  { m_renderLock = true;  }
	void unlockRenderUpdate(){ m_renderLock = false; }

	/// Return pixel position converted into normalized view coordinate in [-1,1]
	QPointF pixelPosToViewPos(const QPointF& p);
	/// Return pixel position converted into unit normalized coordinates in [0,1]
	QPointF pixelPosToUnitPos(const QPointF& p);
	QVector3D pixelPosToSelectionVec( const QPointF& p );
	void drawBackgroundGradient();

// WORKAROUND
public:
	const VolumeDataHeader* getAnyWarpfield();
	/// Returns any warpfield if available, else reference volume
	const VolumeDataHeader* getAnyReferenceVolume();

private:
	bool m_firstInitialization;
	bool m_initialized;

	SDMVisConfig*          m_config;
	StatisticalDeformationModel* m_sdm;

	VolumeDataHeader*      m_vol;
	VolumeRendererRaycast* m_vren;
	GL::GLTexture          m_vtex;
	VolumeManager          m_vman;       // only used for warpfields yet
	QList<Warpfield>       m_warpfields;
	Trackball2             m_trackball2;
	bool                   m_overlay;     ///< overlay additional information
	InteractionMode        m_mode;
	bool                   m_showROI;
	bool                   m_showBBox;
	double                 m_cameraFOV;
	bool                   m_renderLock;  // invokeRenderUpdate() forbidden?
	
	QPoint                 m_mousePos;
	QPoint                 m_pickStartPos;
	QPoint                 m_pickCurPos;

	int                    m_colormode;

	QString                m_modeString;

	QString                m_title; // one-liner shown inside the rendering

	///@{ Interactive editing
	SDMVisInteractiveEditing m_edit;
	SDMVisInteractiveEditingOptionsWidget* m_editWidget;
	bool                   m_editValidDeform;
	///@}

	///@{ Batch processing
	BatchProcessingDialog* m_batchproc;
	QString                m_baseDir;     ///< working directory for batch processes
	QString                m_batchVoltoolsDir;
	///@}

	///@{ ROI
	SphereSelection        m_roi;
	///@}

	///@{ Warp animation 
	// (not to mix up with the frequent render-update which is also sometimes called Animation)
	QTimer*                m_animTimer;
	int                    m_animSpeed;
	int                    m_animWarp;
	///@}

	// only for printing infos, set by downloadVolume()
	QString m_volName;

	QList<QAction*> m_actionsRenderer,
	                m_actionsDebug,
					m_actionsAnalysis;

	// debug actions
	QAction *actShowDebugInfo;

	SDMVisVolumeRendererControlWidget* myControlWidget;

	// Some actions have to be members to allow access from outside
	// e.g. to directly trigger mode actions.
	QAction* m_actModeTrackball;
	QAction* m_actModeRubberbandEditing;

	struct ColorGradient { 
		QColor colors[2];
		ColorGradient() 
		{ 
			colors[0] = QColor(255,255,255);  
			colors[1] = QColor(.7*255,.7*255,.7*255);
		}
		void getrgb( int c, float* rgb )
		{
			if( c < 0 || c > 1 ) return;
			rgb[0] = colors[c].red()   / 255.f; 
			rgb[1] = colors[c].green() / 255.f; 
			rgb[2] = colors[c].blue()  / 255.f;
		}
	};
	ColorGradient m_gradient;
};


#endif // SDMVISVOLUMERENDERER_H
