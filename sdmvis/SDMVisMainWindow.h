#ifndef SDMVISMAINWINDOW_H
#define SDMVISMAINWINDOW_H

//-----------------------------------------------------------------------------
// SDMVis hardcoded configuration
//-----------------------------------------------------------------------------

// Override defines from CMakeLists.txt
//#undef SDMVIS_VTK_ENABLED     // currently not used in code (VTK now obligatory)
//#undef SDMVIS_VARVIS_ENABLED  // undefined for performance test
//#define SDMVIS_VTKVISWIDGET_ENABLED  // currently we are not developing these VTK widgets!

// User defines
#define SDMVIS_USE_WEBKIT       // use WebKit for help browser (better looks)
#define SDMVIS_MANUAL_ANALYSIS  // obsolete manual analysis actions
#define DEVELOPER_MODE          // show all menus

#define SDMVIS_TRAITVIS_ENABLED
//#define SDMVIS_OLDQUALITYVIS_ENABLED
//#define SDMVIS_DATATABLE_ENABLED

//-----------------------------------------------------------------------------

#include <QMainWindow>
#include <QDialog>
#include <QList>
#include <QStringList>
#include <QHelpEngine>

#ifndef Q_MOC_RUN
// Workaround for BOOST_JOIN problem: Undef critical code for moc'ing.
// There is a bug in the Qt Moc application which leads to a parse error at the 
// BOOST_JOIN macro in Boost version 1.48 or greater.
// See also:
//	http://boost.2283326.n4.nabble.com/boost-1-48-Qt-and-Parse-error-at-quot-BOOST-JOIN-quot-error-td4084964.html
//	http://cgal-discuss.949826.n4.nabble.com/PATCH-prevent-Qt-s-moc-from-choking-on-BOOST-JOIN-td4081602.html
//	http://article.gmane.org/gmane.comp.lib.boost.user/71498
#include "Warpfield.h"
#include "SDMVisConfig.h"
#include "ScatterPlotWidget.h"
#include "Trait.h"
#include "StatisticalDeformationModel.h"
#endif

#ifdef SDMVIS_VTK_ENABLED
#include <vtkCameraInterpolator.h>
#endif

// external
class QGLWidget;
class SDMVisVolumeRenderer;
class DatasetWidget;
class TraitDialog;
class BatchProcessingDialog;

// forwards
class HelpBrowser;
class NumberedSpinner;
class SDMVisScalesDialog;
class SDMVisMainWindow;

//==============================================================================
//	HelpBrowser
//==============================================================================

#ifdef SDMVIS_USE_WEBKIT

#include <QWebView>

class HelpBrowser : public QWebView
{
Q_OBJECT
public:
	HelpBrowser( QHelpEngine* he, QWidget* parent=0 );

public slots:
	void setSource(const QUrl &url) { load(url); }

private:
	QHelpEngine* m_helpEngine;
};

#else

class HelpBrowser : public QTextBrowser
{
Q_OBJECT
public:
	HelpBrowser( QHelpEngine* he, QWidget* parent=0 )
		: QTextBrowser(parent), m_helpEngine(he)
	{}

public slots:
	QVariant HelpBrowser::loadResource( int type, const QUrl &url )
	{
		if( url.scheme() == "qthelp" )
			return QVariant( m_helpEngine->fileData(url) );
		else
			return QTextBrowser::loadResource( type, url );
	}

private:
	QHelpEngine* m_helpEngine;
};

#endif

//==============================================================================
//	NumberedSpinner
//==============================================================================

/// Widget containing double spinbox and label
/// provides index in valueChanged() signal
class QDoubleSpinBox;
class QLabel;
class NumberedSpinner : public QWidget
{
	Q_OBJECT
signals:
	void valueChanged( int idx, double d );
public:
	NumberedSpinner( int index, QWidget* parent=0 );
	int index() const { return m_index; }
	QDoubleSpinBox* spinBox() { return m_spinBox; }
	QLabel* label() { return m_label; }
protected slots:
	void emitValueChanged( double value ) { emit valueChanged(m_index,value); }
private:
	int m_index;
	QDoubleSpinBox* m_spinBox;
	QLabel* m_label;
};

//==============================================================================
//	SDMVisScalesDialog
//==============================================================================

/// SDMVis dialog to adjust warpfield normalization / scaling
class SDMVisScalesDialog : public QDialog
{
	Q_OBJECT
signals:
	void scaleChanged( int idx, double d );

public:
	SDMVisScalesDialog( QWidget* parent=0 );

	enum Constants { MaxEntries = 5 };

public slots:
	void setSDMVisConfig( SDMVisConfig* config );

	void updateScales( QList<Warpfield> warpfields );
	void updateScales( QList<float> scales );
	void updateLabels( QStringList labels );
	void computeAutoScaling();

private:
	QList<float> m_scales;
	QList<NumberedSpinner*> m_spinners;
	SDMVisConfig* m_config;
};


//==============================================================================
//	LookmarkWidget
//==============================================================================
class QComboBox;

class LookmarkWidget : public QWidget
{
	Q_OBJECT
signals:
	void setLookmarkFilename( QString );

public:
	LookmarkWidget( QWidget* parent=NULL );

	void clear();
	
	void setConfig( const SDMVisConfig& config );

protected slots:
	void setSelectedLookmark();

private:
	QStringList m_filenames;
	QComboBox*  m_combobox;
};


//==============================================================================
//	ReconTraitParameters
//==============================================================================
class VolumeDataHeader;
struct ReconTraitParameters
{
	QString sOutputSuffix;
	int N,  ///< size of vectorized warp fields (= 3*resX*resY*resZ)
		M;  ///< size of dataset (= number of specimens)
	int resX, resY, resZ; ///< resolution of volume
	double spacingX, spacingY, spacingZ; ///< spacing of volume

	// c'tors
	ReconTraitParameters() {}
	ReconTraitParameters( QString outsfx, int m, VolumeDataHeader* vol );

	QString configPath, traitFilename, warpsFilename;
};

//==============================================================================
//	RefineROIParameters
//==============================================================================
struct RefineROIParameters
{
	// Processing is performed on matrix disk files which follow a specific
	// naming convention.
	QString sInputSuffix, sOutputSuffix;
	int N,  ///< size of vectorized warp fields (= 3*resX*resY*resZ)
		M,  ///< size of dataset (= number of specimens)
		K;  ///< number of eigenwarps to consider
	int resX, resY, resZ; ///< resolution of volume

	// c'tors
	RefineROIParameters() {}
	RefineROIParameters( QString configPath_, QString warpsFilename_, 
		                 QString outsfx, int m, int k, 
		                 const VolumeDataHeader* vol );

	///@{ strings for batch processing (according to naming convention)
	QString configPath, warpsFilename, ROIBasename,
			sN,
			sM,
			sK,
		outWarps,
		outScatter,
		outPCAEigenvectors,
		outPCACoefficients,
		outPCAEigenvalues,
		outEigenwarps,
		outEigenwarps_global;
	///@}

	QVector<ROI> roiVector; ///< active set of spherical ROIs

	QStringList  eigenWarpList, eigenWarpListLocal;
	QString      savingPath;
	QString      reference;
};

//==============================================================================
//	SDMVisMainWindow
//==============================================================================
/// SDMVis main window
class SDMTensorVisWidget;
class SDMTensorOverviewWidget;
class VarVisRender;
class VolumeControls;
class VarVisControls;
class GlyphControls;
class VarVisWidget;
class RoiControls;

class SDMVisMainWindow : public QMainWindow
{
	Q_OBJECT

	static bool s_cfgPresent;

	// Symbolic names for the tab widget numbers
	enum TabIndices
	{
		TabRaycasterSDM,
#ifdef SDMVIS_TRAITVIS_ENABLED
		TabRaycasterTraitModel,
#endif
#ifdef SDMVIS_VARVIS_ENABLED
		TabSelectROI,
		TabGlyphVis,
#endif
#ifdef SDMVIS_OLDQUALITYVIS_ENABLED
		TabQualityVis,
#endif
#ifdef SDMVIS_TENSORVIS_ENABLED
		TabTensorVis,
		TabTensorOverviewVis,
#endif
#ifdef SDMVIS_DATATABLE_ENABLED
		TabDatasetTable,
#endif
		NumTabs
	};

	enum UnusedTabIndices
	{
		TabUnusedFirst = 999,
#ifndef SDMVIS_TRAITVIS_ENABLED
		TabRaycasterTraitModel,
#endif
#ifndef SDMVIS_VARVIS_ENABLED
		TabSelectROI,
		TabGlyphVis,
#endif		
#ifndef SDMVIS_OLDQUALITYVIS_ENABLED
		TabQualityVis,
#endif
#ifndef SDMVIS_TENSORVIS_ENABLED
		TabTensorVis,
		TabTensorOveriewVis,
#endif
#ifndef SDMVIS_DATATABLE_ENABLED
		TabDatasetTable,
#endif
		TabUnusedLast
	};

public:
	SDMVisMainWindow();
	~SDMVisMainWindow();
	void workaround(int index);
signals:
	void traitHasChanged(int);
public slots:
	void about();
	void help();
	// config:
	void statusMessage( QString );
	void setTraitScale( int traitID, double value );
	void openVolumeDataset();
	bool openVolumeDataset( QString filename );
	void openConfig();
	void saveConfig();                      /// save current config
	void saveConfigAs();                    /// show save file dialog
	void saveConfigAs( QString filename );  /// actual code to save config
	void setSyncTabs( bool enabled );
	void setExpertMode( bool expert );
	void generateConfig();
	void selectWarps();
	void selectWarpsMatrix();
	void selectEigenwarpsMatrix();
	void selectPCAModel();
	void openNames();
	void addTrait(Trait,int pos);
	// trait computation:
	void showTraitDialog();
	void showScatterPlot();
#ifdef SDMVIS_MANUAL_ANALYSIS
	void computeTraitWarpfield();
#endif
	void computeTraitWarpfieldFileName( QString, QString );
	void loadNewTrait( int index );
	void setNewTrait(int index);
	void showEigenvalues();
	void loadWarpfield( QString path );
	void unloadWarpfield();
	void updateTraitList( QList<Trait>, int );

	void loadLookmark();
	void loadLookmark( QString filename );
	void saveLookmark();

	void customWindowResize();

protected slots:
	void setScale( int idx, double value );
	void loadConfig( QString filename );
	void loadConfig2();
	void tabIndexChanged( int index );
	
	// debugging/development:
	void testBatchProcessingDialog();
	void testAutoScreenshots();
	void testAutoScreenshots2();

	void onToolTrackball();
	void onToolRubberband();
	void onToolTrait();
	void onToolTensor();
	void onToolROI();
	void onToolGlyphs();

protected:
	void closeEvent( QCloseEvent* event );
	bool setVolume( QString mhdFilename );
	bool setWarpfields( QList<Warpfield> warpfields );
	//bool setTraitWarpfield( QList<Warpfield> warpfields );
	void setNames( QStringList names );
	QString getBasePath();
	void computeTraitWarpfield_internal( ReconTraitParameters parms );

private:
	bool m_expertMode;

	// ROI specific stuff
	RefineROIParameters   m_roiParms;

	// Widgets and dialogs
	SDMVisVolumeRenderer* m_volumeRenderer;
	SDMVisVolumeRenderer* m_traitRenderer;
	SDMVisScalesDialog*   m_scalesDialog;
	DatasetWidget*        m_datasetWidget;
	TraitDialog*          m_traitDialog;
	BatchProcessingDialog*m_batchDialog;
	ScatterPlotWidget*    m_scatterPlotWidget;
	BatchProcessingDialog*m_batchproc ;
	LookmarkWidget*       m_lookmarkWidget;

	
	QTabWidget*  m_centralTabWidget ;
	QDockWidget* m_controlDock;

	QMenu* m_menuRaycaster,
		 * m_menuTraitRaycaster;

	bool m_syncTabs; // synchronize camera between raycaster widgets
	
	// SDM data
	SDMVisConfig m_config;
	StatisticalDeformationModel m_sdm;

	QList<Trait> m_tempTraitList;
	
	// Application settings
	void readSettings();
    void writeSettings();
	void setupConnections();
	void unloadConfig();	
	void loadWidgetsContents();
	QString m_baseDir;     // ... sync m_baseDir with config.sdm.basePath ?
	QString m_batchVoltoolsPath;
	QString m_loadedConfigName;

#ifdef SDMVIS_VARVIS_ENABLED
//////////////////////////////////////////////////////////////////////////////
//	VARVIS
//////////////////////////////////////////////////////////////////////////////
public slots:
	// ROI functionality now totally depends on VarVis module
	void batchROIfinished();
	void refineROI();
	void saveConfigROI( RefineROIParameters parms, bool local=false, QString reference="" );

protected slots:
	void openVolumeDatasetVarVis();
	void openWarpDatasetVarVis();
	void clearVarVisBox();	
private:
	// VarVis
	VarVisRender	    * m_varvisRender;
	VarVisControls      * m_varvisControls;
	VarVisWidget	    * m_varvisWidget;
	GlyphControls		* m_glyphControls;

	// Error visualisation
	VarVisRender	    * m_errorRender;
	VolumeControls      * m_errorVisControls;
	VarVisWidget	    * m_errorWidget;
	
	// Region of Interest
	VarVisRender	    * m_roiRender;
	RoiControls         * m_roiControls;
	VarVisWidget	    * m_roiWidget;	

	// Config
	QString               m_varvisBaseDir;

	// Aditional menu entries
	QMenu *m_menuVarVis ;
	QMenu *m_menuErrorVis;
#endif // SDMVIS_VARVIS_ENABLED

#ifdef SDMVIS_TENSORVIS_ENABLED
//////////////////////////////////////////////////////////////////////////////
//	TENSORVIS
//////////////////////////////////////////////////////////////////////////////	
private:
	// TensorVis widgets
	SDMTensorVisWidget* m_tensorVis;
	//QDockWidget*      m_tensorControlDock;
	QMenu*              m_tensorVisMenu;
	SDMTensorOverviewWidget* m_tensorOverviewVis;
#endif SDMVIS_TENSORVIS_ENABLED

#ifdef SDMVIS_VTK_ENABLED
	vtkCameraInterpolator* m_vtkCameraInterpolator;
	QTimer* m_cameraAnimationTimer;
	double m_cameraAnimationT;
protected slots:
	void animateLookmark();
#endif
};

#endif // SDMVISMAINWINDOW_H
