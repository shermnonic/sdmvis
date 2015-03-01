#include "SDMVisMainWindow.h"
#include "SDMVisVolumeRenderer.h"
#include "DatasetWidget.h"
#include "BatchProcessingDialog.h"
#include "TraitDialog.h"
#include <QtGui>
#include <QHelpContentWidget>
#include <QHelpIndexWidget>
#include <QTimer>
#include "Trait.h"
#include <VolumeRendering/VolumeUtils.h>  // load_volume(), create_volume_tex()
#include <fstream>
#include <string>
#include "plotWidget.h"
#include "TraitSelectionWidget.h"
#include "ConfigGenerator.h"
#include "e7/VolumeRendering/RayPickingInfo.h"

#ifdef SDMVIS_VTKVISWIDGET_ENABLED  // automatically set by cmake (see CMakeLists.txt)
#include "VTKVisWidget.h"
#endif // SDMVIS_VTKVISWIDGET_ENABLED 

#ifdef SDMVIS_VARVIS_ENABLED
#include "../varvis/VarVisWidget.h"
#include "../varvis/VarVisRender.h"
#include "../varvis/VarVisControls.h"
#include "../varvis/VolumeControls.h"
#include "../varvis/GlyphControls.h"
#include "../varvis/RoiControls.h"
#include <vtkRendererCollection.h>
#include <vtkTransform.h>
#endif // SDMVIS_VARVIS_ENABLED

#ifdef SDMVIS_TENSORVIS_ENABLED
#include "SDMTensorVisWidget.h"
#include "SDMTensorOverviewWidget.h"
#endif // SDMVIS_TENSORVIS_ENABLED

#ifdef SDMVIS_VTK_ENABLED
#include "VTKCameraSerializer.h"
#include <vtkCameraInterpolator.h>
#endif

bool SDMVisMainWindow::s_cfgPresent=false;

//==============================================================================
//	WebKit Help Stuff
// http://beaverdbg.googlecode.com/svn/trunk/beaverdbg/src/shared/help/helpviewer.cpp
//==============================================================================
#ifdef SDMVIS_USE_WEBKIT

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

class HelpNetworkReply : public QNetworkReply
{
public:
    HelpNetworkReply(const QNetworkRequest &request, const QByteArray &fileData,
        const QString &mimeType);

    virtual void abort();

    virtual qint64 bytesAvailable() const
        { return data.length() + QNetworkReply::bytesAvailable(); }

protected:
    virtual qint64 readData(char *data, qint64 maxlen);

private:
    QByteArray data;
    qint64 origLen;
};

HelpNetworkReply::HelpNetworkReply(const QNetworkRequest &request,
        const QByteArray &fileData, const QString &mimeType)
    : data(fileData), origLen(fileData.length())
{
    setRequest(request);
    setOpenMode(QIODevice::ReadOnly);

    setHeader(QNetworkRequest::ContentTypeHeader, mimeType);
    setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(origLen));
    QTimer::singleShot(0, this, SIGNAL(metaDataChanged()));
    QTimer::singleShot(0, this, SIGNAL(readyRead()));
}

void HelpNetworkReply::abort()
{
    // nothing to do
}

qint64 HelpNetworkReply::readData(char *buffer, qint64 maxlen)
{
    qint64 len = qMin(qint64(data.length()), maxlen);
    if (len) {
        qMemCopy(buffer, data.constData(), len);
        data.remove(0, len);
    }
    if (!data.length())
        QTimer::singleShot(0, this, SIGNAL(finished()));
    return len;
}

//------------------------------------------------------------------------
// HelpNetworkAccessManager
//------------------------------------------------------------------------
class HelpNetworkAccessManager : public QNetworkAccessManager
{
public:
    HelpNetworkAccessManager(QHelpEngine *engine, QObject *parent);

protected:
    virtual QNetworkReply *createRequest(Operation op,
        const QNetworkRequest &request, QIODevice *outgoingData = 0);

private:
    QHelpEngine *helpEngine;
};

HelpNetworkAccessManager::HelpNetworkAccessManager(QHelpEngine *engine,
        QObject *parent)
    : QNetworkAccessManager(parent), helpEngine(engine)
{
}

QNetworkReply *HelpNetworkAccessManager::createRequest(Operation op,
    const QNetworkRequest &request, QIODevice *outgoingData)
{
    const QString& scheme = request.url().scheme();
    if (scheme == QLatin1String("qthelp") || scheme == QLatin1String("about")) {
        const QUrl& url = request.url();
        QString mimeType = url.toString();
        if (mimeType.endsWith(QLatin1String(".svg"))
            || mimeType.endsWith(QLatin1String(".svgz"))) {
           mimeType = QLatin1String("image/svg+xml");
        }
        else if (mimeType.endsWith(QLatin1String(".css"))) {
           mimeType = QLatin1String("text/css");
        }
        else if (mimeType.endsWith(QLatin1String(".js"))) {
           mimeType = QLatin1String("text/javascript");
        } else {
            mimeType = QLatin1String("text/html");
        }
        return new HelpNetworkReply(request, helpEngine->fileData(url), mimeType);
    }
    return QNetworkAccessManager::createRequest(op, request, outgoingData);
}

//------------------------------------------------------------------------
// HelpPage
//------------------------------------------------------------------------
class HelpPage : public QWebPage
{
public:
    HelpPage(QHelpEngine *engine, QObject *parent);

protected:
    virtual QWebPage *createWindow(QWebPage::WebWindowType);

    virtual bool acceptNavigationRequest(QWebFrame *frame,
        const QNetworkRequest &request, NavigationType type);

private:
    QHelpEngine *helpEngine;
};

HelpPage::HelpPage(QHelpEngine *engine, QObject *parent)
    : QWebPage(parent), helpEngine(engine)
{}

QWebPage *HelpPage::createWindow(QWebPage::WebWindowType)
{
	return NULL; // no support for popup or tabbed windows
    //return centralWidget->newEmptyTab()->page();
}

static bool isLocalUrl(const QUrl &url)
{
    const QString scheme = url.scheme();
    if (scheme.isEmpty()
        || scheme == QLatin1String("file")
        || scheme == QLatin1String("qrc")
        || scheme == QLatin1String("data")
        || scheme == QLatin1String("qthelp")
        || scheme == QLatin1String("about"))
        return true;
    return false;
}

bool HelpPage::acceptNavigationRequest(QWebFrame *,
    const QNetworkRequest &request, QWebPage::NavigationType)
{
    const QUrl &url = request.url();
    if (isLocalUrl(url)) {
        if (url.path().endsWith(QLatin1String("pdf"))) {
            QString fileName = url.toString();
            fileName = QDir::tempPath() + QDir::separator() + fileName.right
                (fileName.length() - fileName.lastIndexOf(QChar('/')));

            QFile tmpFile(QDir::cleanPath(fileName));
            if (tmpFile.open(QIODevice::ReadWrite)) {
                tmpFile.write(helpEngine->fileData(url));
                tmpFile.close();
            }
            QDesktopServices::openUrl(QUrl(tmpFile.fileName()));
            return false;
        }
        return true;
    }

    QDesktopServices::openUrl(url);
    return false;
}

//------------------------------------------------------------------------
// HelpBrowser
//------------------------------------------------------------------------

HelpBrowser::HelpBrowser( QHelpEngine* he, QWidget* parent )
	: QWebView(parent), m_helpEngine(he)
{
	setPage( new HelpPage(he, this) );
	page()->setNetworkAccessManager( new HelpNetworkAccessManager(he, this) );
}

#endif // SDMVIS_USE_WEBKIT


//==============================================================================
//	ReconTraitParameters
//==============================================================================
ReconTraitParameters::ReconTraitParameters( QString outsfx, int m, VolumeDataHeader* vol )
{
	sOutputSuffix = outsfx;
	resX = vol->resX();
	resY = vol->resY();
	resZ = vol->resZ();
	spacingX = vol->spacingX();
	spacingY = vol->spacingY();
	spacingZ = vol->spacingZ();
	N = 3 * resX*resY*resZ;
	M = m;
}

//==============================================================================
//	RefineROIParameters
//==============================================================================
RefineROIParameters::RefineROIParameters( QString configPath_, QString warpsFilename_, 
										  QString outsfx, int m, int k, 
	                                      const VolumeDataHeader* vol )
{
	configPath = configPath_;
	warpsFilename = warpsFilename_;
	
	sOutputSuffix = outsfx;
	resX = vol->resX();
	resY = vol->resY();
	resZ = vol->resZ();
	N = 3 * resX*resY*resZ;
	M = m;
	K = k;
	
	sN = QString::number(N);
	sM = QString::number(M);
	sK = QString::number(K);


	outWarps            = configPath+"/" + "warps_"    +sOutputSuffix+".mat",
	outScatter	        = configPath+"/" + "scatter_t_"+sOutputSuffix+".mat",
	outPCAEigenvectors  = configPath+"/" + "yapca_"+sOutputSuffix+"_V"     +sK+".mat",
	outPCACoefficients  = configPath+"/" + "yapca_"+sOutputSuffix+"_C"     +sK+".mat",
	outPCAEigenvalues   = configPath+"/" + "yapca_"+sOutputSuffix+"_lambda"+sK+".mat",
	outEigenwarps       = configPath+"/" + "eigenwarps"       +sK+"_"+sOutputSuffix+".mat",
	outEigenwarps_global= configPath+"/" + "global_eigenwarps"+sK+"_"+sOutputSuffix+".mat";
	
	ROIBasename = configPath + "/" + "roi_" + sOutputSuffix;
}

//=============================================================================
//	NumberedSpinner
//=============================================================================

NumberedSpinner::NumberedSpinner( int index, QWidget* parent )
	: QWidget(parent),
	  m_index(index)
{
	m_spinBox = new QDoubleSpinBox();
	m_label = new QLabel( this );
	m_label->setBuddy( m_spinBox );

	QHBoxLayout* layout = new QHBoxLayout();
	layout->addWidget( m_label );
	layout->addWidget( m_spinBox );
	setLayout( layout );

	connect( m_spinBox, SIGNAL(valueChanged(double)), this, SLOT(emitValueChanged(double)) );
}

//=============================================================================
//	SDMVisScalesDialog
//=============================================================================

SDMVisScalesDialog::SDMVisScalesDialog( QWidget* parent )
	: QDialog(parent),
	  m_config( NULL )
{
	// create spin widgets
	for( int i=0; i < MaxEntries; ++i )
	{
		NumberedSpinner* nspin = new NumberedSpinner( i );
		nspin->spinBox()->setRange( 0.001, 2.0 );
		nspin->spinBox()->setValue( 1.0 );
		nspin->spinBox()->setDecimals( 4 );
		nspin->spinBox()->setSingleStep( 0.1 );

		nspin->label()->setText( tr("(Warpfield %1 not present)").arg(i+1) );

		connect( nspin, SIGNAL(valueChanged(int,double)), this, SIGNAL(scaleChanged(int,double)) );

		m_spinners.push_back( nspin );
	}

	// label
	QLabel* label = new QLabel( tr("Adjust warpfield scaling / normalization.") );

	// button
	QPushButton* butAuto = new QPushButton( tr("Default scaling") );
	connect( butAuto, SIGNAL(clicked()), this, SLOT(computeAutoScaling()) );

	// layout
	QGroupBox* spinGroup = new QGroupBox( tr("Warpfield scaling factors") );	
	QVBoxLayout* lspin = new QVBoxLayout();
	for( int i=0; i < m_spinners.size(); ++i ) {
		lspin->addWidget( m_spinners[i] );
		// disabled by default, activated after first call to updateScales()
		m_spinners[i]->setEnabled( false );
	}
	spinGroup->setLayout( lspin );

	QVBoxLayout* layout = new QVBoxLayout();
	//layout->addWidget( label );
	layout->addWidget( spinGroup );
	layout->addWidget( butAuto );
	setLayout( layout );
}

void SDMVisScalesDialog::setSDMVisConfig( SDMVisConfig* config )
{
	m_config = config;
	if( m_config )
	{
		updateScales( m_config->getWarpfields() );
	}
}

void SDMVisScalesDialog::computeAutoScaling()
{
	if( !m_config )
	{
		qDebug("No SDMConfig set!");
		return;
	}
	
	updateScales( m_config->getWarpfieldAutoScalings() );
}

void SDMVisScalesDialog::updateLabels( QStringList labels )
{
	for( int i=0; i < m_spinners.size(); ++i )
	{
		if( i < labels.size() )
		{
			m_spinners[i]->label()->setText( labels[i] );
		}
		else
		{
			m_spinners[i]->label()->setText( tr("(Warpfield %1 not present?)").arg(i+1) );
		}
	}
}

void SDMVisScalesDialog::updateScales( QList<float> scales )
{
	m_scales = scales;	

	// update spin widgets
	for( int i=0; i < m_spinners.size(); ++i )
	{
		if( i < m_scales.size() )
		{
			m_spinners[i]->spinBox()->setValue( m_scales[i] );
			m_spinners[i]->setEnabled( true );
		}
		else
		{
			// Disable this spinner
			m_spinners[i]->setEnabled( false );
			m_spinners[i]->spinBox()->setValue( 1.0 );
			m_spinners[i]->label()->setText( tr("(Warpfield %1 not present)").arg(i+1) );
		}
	}
}

void SDMVisScalesDialog::updateScales( QList<Warpfield> warpfields )
{
	QList<float> scales;
	QStringList labels;
	for( int i=0; i < warpfields.size(); ++i )
	{
		// New scaling value
		scales.push_back( warpfields[i].elementScale );

		// New label
		QString path = warpfields[i].mhdFilename;
		path.replace("/","\\");
		QStringList tokens = path.split("\\");
		labels.push_back( tokens.back() );
	}

	this->blockSignals(true);
	updateScales( scales );
	updateLabels( labels );
	this->blockSignals(false);
}


//==============================================================================
//	LookmarkWidget
//==============================================================================
LookmarkWidget::LookmarkWidget( QWidget* parent )
	: QWidget(parent)
{
	m_combobox = new QComboBox();	
	QPushButton* but = new QPushButton(tr("Apply"));
	connect( but, SIGNAL(clicked()), this, SLOT(setSelectedLookmark()) );
	QLabel* label = new QLabel(tr("Lookmark"));

	QHBoxLayout* layout = new QHBoxLayout();
	layout->addWidget( label );
	layout->addWidget( m_combobox, 5 );
	layout->addWidget( but, 1 );
	setLayout( layout );
}

void LookmarkWidget::clear()
{
	m_filenames.clear();
	m_combobox->clear();
}

void LookmarkWidget::setConfig( const SDMVisConfig& config )
{	
	clear();

	// Retrieve full paths
	for( int i=0; i < config.getLookmarks().size(); i++ )
		m_filenames.push_back( config.getAbsolutePath( config.getLookmarks().at(i) ) );

	// Fill combo box
	if( !config.getLookmarks().isEmpty() )
		m_combobox->addItems( config.getLookmarks() );
}

void LookmarkWidget::setSelectedLookmark()
{
	int idx = m_combobox->currentIndex();
	if( idx>=0 && idx < m_filenames.size() )
	{
		emit setLookmarkFilename(m_filenames.at(idx));
	}
	else
	{
		std::cout << "Warning: Invalid lookmark index!\n";
	}
}


//#define GRAPHICSVIEW_HACK
#ifdef GRAPHICSVIEW_HACK
//=============================================================================
//	GraphicsView hack to support transparent widgets on top of QGLWidgets
//  (see http://doc.trolltech.com/qq/qq26-openglcanvas.html)
//=============================================================================
#include <QGraphicsView>

class GraphicsView : public QGraphicsView
{
public:
	GraphicsView()
	{}
protected:
	void resizeEvent( QResizeEvent *event )
	{
		if( scene() )
			scene()->setSceneRect(QRect(QPoint(0,0), event->size()));
		QGraphicsView::resizeEvent( event );
	}
};

class GLScene : public QGraphicsScene
{
private:
	CustomGLWidget* m_glwidget; // CustomGLWidget defined in SDMVisVolumeRenderer.h
public:
	GLScene()
		: m_glwidget(0)
	{}

	void setCustomGLWidget( CustomGLWidget* glw ) { m_glwidget = glw; }

protected:
	void drawBackground( QPainter* painter, const QRectF& )
	{
		//if( painter->paintEngine()->type() != QPaintEngine::OpenGL )
		//{
		//	qWarning("GLScene: drawBackground needs a QGLWidget to be set "
		//		     "as viewport on the graphics view");
		//	return;
		//}

		if( !m_glwidget )
			return;

		m_glwidget->paint_();
	}
};

#include "BarPlotWidget.h"
BarPlotWidget* createBarPlotWidget()
{
	QVector<double> values;
	values.push_back(1.23); 
	values.push_back(-1.23); 
	values.push_back(2.3); 
	values.push_back(0.1);

	QStringList labels;
	labels << "One" << "Two" << "Three" << "Four";

	BarPlotWidget* barplotTest = new BarPlotWidget();
	barplotTest->resize( 500, 100 );
	barplotTest->move( 100, 500 );
	barplotTest->setValues( values );
	barplotTest->setLabels( labels );

	return barplotTest;
}

#endif // GRAPHICSVIEW_HACK


//=============================================================================
//	SDMVisMainWindow
//=============================================================================

SDMVisMainWindow::SDMVisMainWindow()
: m_syncTabs(true),m_expertMode(false),
  m_loadedConfigName("")
{
	setWindowTitle( APP_NAME );
	setWindowIcon( APP_ICON );

	// -- Widgets -------------------------------------------------------------

	m_traitDialog = new TraitDialog( this );
	m_batchDialog = new BatchProcessingDialog( this );
	m_batchDialog->setWindowIcon( APP_ICON );
	
	m_scatterPlotWidget = new ScatterPlotWidget();
	m_scatterPlotWidget->setWindowTitle( tr("%1 - PCA scatter plot matrix").arg(APP_NAME) );
	m_scatterPlotWidget->setWindowIcon( APP_ICON );
	
	m_volumeRenderer = new SDMVisVolumeRenderer();
	m_traitRenderer  = new SDMVisVolumeRenderer( 0, (QGLWidget*)m_volumeRenderer );
	m_datasetWidget  = new DatasetWidget();

	m_volumeRenderer->setModeString(tr("Eigenmode"));
	m_volumeRenderer->setColorMode( SDMVisVolumeRenderer::ColorPlain );
	m_traitRenderer->setModeString(tr("Trait"));
	m_traitRenderer->setColorMode( SDMVisVolumeRenderer::ColorCustom );

	m_volumeRenderer->getControlWidget()->getTraitSelector()->hide();
	m_volumeRenderer->getControlWidget()->getPlotWidget()->show();

	m_traitRenderer->getControlWidget()->getTraitSelector()->show();
	m_traitRenderer->getControlWidget()->getPlotWidget()->hide();
	m_traitRenderer->getControlWidget()->getTraitSelector()->setSelectionBox( m_traitRenderer->getControlWidget()->getPlotWidget()->getSelectionBox() );

	qDebug("Initialized trait renderer with shared GL context = %d", m_traitRenderer->isSharing());

#ifdef SDMVIS_VARVIS_ENABLED
	// setup VarVis Widgets 
	m_varvisWidget		= new VarVisWidget();
	m_varvisControls	= new VarVisControls();
	m_glyphControls     = new GlyphControls();
	m_varvisRender		= new VarVisRender();

	connect(m_varvisRender,SIGNAL(statusMessage(QString)),this,SLOT(statusMessage(QString)));
	m_varvisWidget->GetRenderWindow()->AddRenderer(m_varvisRender->getRenderer());
	m_varvisWidget->GetRenderWindow()->SetLineSmoothing( 1 );
	m_varvisWidget->setVren(m_varvisRender);

	m_varvisRender->getRenderer()->SetBackground(0.7,0.7,0.7);
	m_varvisRender->getRenderer()->SetBackground2(1,1,1);
	m_varvisRender->getRenderer()->SetGradientBackground( true );
	m_varvisRender->setRenderWindow(m_varvisWidget->GetRenderWindow());
	m_varvisRender->setWarpVis(0);	
	m_varvisRender->setPointSize(2.5);
	double radiusOfPoints=m_varvisRender->getSamplePointSize();
	
	m_varvisControls->setVRen(m_varvisRender);	// will not be used 
	m_glyphControls->setVarVisRenderer(m_varvisRender);

	// new VaVis Widget for error analysation 
	m_errorRender       = new VarVisRender();
	m_errorWidget		= new VarVisWidget();
	m_errorVisControls  = new VolumeControls();

	m_errorWidget->GetRenderWindow()->AddRenderer(m_errorRender->getRenderer());
	m_errorWidget->GetRenderWindow()->SetLineSmoothing( 1 );
	m_errorWidget->setVren(m_errorRender);
	m_errorWidget->setVolumeControls(m_errorVisControls);
	m_errorRender->getRenderer()->SetBackground(0.7,0.7,0.7);
	m_errorRender->getRenderer()->SetBackground2(1,1,1);
	m_errorRender->getRenderer()->SetGradientBackground( true );
	m_errorRender->setRenderWindow(m_errorWidget->GetRenderWindow());
	m_errorRender->setWarpVis(0);	

	connect(m_errorRender,SIGNAL(statusMessage(QString)),this,SLOT(statusMessage(QString)));
	m_errorVisControls->setVarVisRenderer(m_errorRender);	

	m_roiWidget		= new VarVisWidget();
	m_roiControls   = new RoiControls();
	m_roiRender		= new VarVisRender();
	m_roiRender->setMeToRoiRender(true);
	// use the same rendere as in VarVis -> CamSync ? 
	m_roiWidget->GetRenderWindow()->AddRenderer(m_roiRender->getRenderer());
	m_roiWidget->setVren(m_roiRender);
	m_roiControls->setVarVisRenderer(m_roiRender);	

	m_roiRender->getRenderer()->SetBackground(0.7,0.7,0.7);
	m_roiRender->getRenderer()->SetBackground2(1,1,1);
	m_roiRender->getRenderer()->SetGradientBackground( true );
	m_roiRender->setRenderWindow(m_roiWidget->GetRenderWindow());
	m_roiRender->setWarpVis(0);	
	connect(m_roiRender,SIGNAL(statusMessage(QString)),this,SLOT(statusMessage(QString)));

	// okay lets create a camera 
	VTKPTR<vtkCamera> masterCamera = VTKPTR<vtkCamera> ::New();
	m_roiRender->getRenderer()->SetActiveCamera(masterCamera);
	m_varvisRender->getRenderer()->SetActiveCamera(masterCamera);
#endif // SDMVIS_VARVIS_ENABLED

#ifdef SDMVIS_TENSORVIS_ENABLED
	m_tensorVis = new SDMTensorVisWidget();
	m_tensorOverviewVis = new SDMTensorOverviewWidget();

	m_tensorVis->getRenderer()->SetBackground (0.7,0.7,0.7);
	m_tensorVis->getRenderer()->SetBackground2(1,1,1);
	m_tensorVis->getRenderer()->SetGradientBackground( true );

	m_tensorOverviewVis->getRenderer()->SetBackground (0.7,0.7,0.7);
	m_tensorOverviewVis->getRenderer()->SetBackground2(1,1,1);
	m_tensorOverviewVis->getRenderer()->SetGradientBackground( true );

	connect( m_volumeRenderer, SIGNAL(pickedRay(RayPickingInfo)),
		     m_tensorVis, SLOT(setPickedRay(RayPickingInfo)) );

	connect( m_tensorVis, SIGNAL(statusMessage(QString)),
		this, SLOT(statusMessage(QString)) );
	connect( m_tensorOverviewVis, SIGNAL(statusMessage(QString)),
		this, SLOT(statusMessage(QString)) );
#endif

	// -- Control dock widget -------------------------------------------------

	// Put each control widget in its own scroll area
	QList<QScrollArea*> scrollAreas;
	
	// PCA raycaster controls
	scrollAreas.append( new QScrollArea() );
	scrollAreas.back()->setWidget( m_volumeRenderer->getControlWidget() );

	// Trait raycaster controls
	scrollAreas.append( new QScrollArea() );
	scrollAreas.back()->setWidget( m_traitRenderer->getControlWidget() );
	
#ifdef SDMVIS_VARVIS_ENABLED
	// ROI controls
	scrollAreas.append( new QScrollArea() );
	scrollAreas.back()->setWidget( m_roiControls );

	// Glyph controls
	scrollAreas.append( new QScrollArea() );
	scrollAreas.back()->setWidget( m_glyphControls );
#endif
#ifdef SDMVIS_OLDQUALITYVIS_ENABLED
	// Quality controls
	scrollAreas.append( new QScrollArea() );
	scrollAreas.back()->setWidget( m_errorVisControls );
#endif
#ifdef SDMVIS_TENSORVIS_ENABLED
	// Tensor vis controls
	scrollAreas.append( new QScrollArea() );
	scrollAreas.back()->setWidget( m_tensorVis->getControlWidget() );

	// Overview vis controls
	scrollAreas.append( new QScrollArea() );
	scrollAreas.back()->setWidget( m_tensorOverviewVis->getControlWidget() );
#endif
#ifdef SDMVIS_DATATABLE_ENABLED
	// Dataset controls
	scrollAreas.append( new QScrollArea() );
	scrollAreas.back()->setWidget( m_datasetWidget->getControlWidget() );
#endif
	
	// Set default scroll options
	for( int i=0; i < scrollAreas.size(); i++ )
	{
		scrollAreas[i]->setWidgetResizable( true );
		scrollAreas[i]->setAlignment( Qt::AlignCenter );
		scrollAreas[i]->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
		scrollAreas[i]->widget()->setMinimumWidth( 300 );
	}

	// Stacked control widget
	QStackedWidget* controlStack = new QStackedWidget;
	for( int i=0; i < scrollAreas.size(); i++ )
		controlStack->addWidget( scrollAreas[i] );
	controlStack->setMinimumWidth( 300 );

	// Control dock widget
	m_controlDock = new QDockWidget(tr("Controls"),this);
	m_controlDock->setObjectName( tr("ControlDockWidget") );
	m_controlDock->setWidget( controlStack );	
	addDockWidget( Qt::RightDockWidgetArea, m_controlDock );

	// -- Tab widgets ---------------------------------------------------------

	m_centralTabWidget = new QTabWidget();
#ifndef GRAPHICSVIEW_HACK
	m_centralTabWidget->addTab( m_volumeRenderer, tr("SDM")  ); // Raycaster
#else
	GraphicsView* gv = new GraphicsView();
	gv->setViewport( m_volumeRenderer );
	gv->setViewportUpdateMode( QGraphicsView::FullViewportUpdate );

	GLScene* gs = new GLScene();
	gs->setCustomGLWidget( m_volumeRenderer );
	gv->setScene( gs );

	BarPlotWidget* bpw = createBarPlotWidget();
	bpw->setWindowOpacity( 0.8 );
	gs->addWidget( bpw );

	m_centralTabWidget->addTab( gv              , tr("SDM"    ) );
#endif // GRAPHICSVIEW_HACK
#ifdef SDMVIS_TRAITVIS_ENABLED
	m_centralTabWidget->addTab( m_traitRenderer , tr("Trait"  ) ); // Raycaster
#endif
#ifdef SDMVIS_VARVIS_ENABLED
	m_centralTabWidget->addTab( m_roiWidget		, tr("ROI"    ) ); // VarVis
	m_centralTabWidget->addTab( m_varvisWidget  , tr("Vector fields" ) ); // VarVis
#endif
#ifdef SDMVIS_OLDQUALITYVIS_ENABLED	
	m_centralTabWidget->addTab( m_errorWidget	, tr("Quality") ); // VarVis
#endif
#ifdef SDMVIS_TENSORVIS_ENABLED
	m_centralTabWidget->addTab( m_tensorVis     , tr("Tensor fields" ) ); // TensorVis
	m_centralTabWidget->addTab( m_tensorOverviewVis,tr("Tensor Overview") );
#endif
#ifdef SDMVIS_DATATABLE_ENABLED
	m_centralTabWidget->addTab( m_datasetWidget , tr("Dataset") ); // Dataset table
#endif

	connect( m_centralTabWidget, SIGNAL(currentChanged(int)),
				controlStack, SLOT(setCurrentIndex(int)) );
	connect( m_centralTabWidget, SIGNAL(currentChanged(int)),
				this, SLOT(tabIndexChanged(int)) );

#ifdef SDMVIS_VTKVISWIDGET_ENABLED
	VTKVisWidget* vtkWidget = new VTKVisWidget();
	VTKVisWidget* vmn_vtkWidget = new VTKVisWidget();
	
	m_centralTabWidget->addTab( vtkWidget, tr("VTKVis") );
	m_centralTabWidget->addTab( vmn_vtkWidget, tr("V-Mann.VTKVis") );
#endif
	
	setCentralWidget( m_centralTabWidget );
	
	// -- Actions -------------------------------------------------------------

	QAction 
		*actOpenVolume,
		*actOpenNames,
		*actOpenConfig,
		*actOpenConfig2,
		*actSaveConfig,
		*actSaveConfigAs,
		*actGenerateConfig,
		*actLoadLookmark,
		*actSaveLookmark,
		*actSyncTabs,
		*actSelectWarps,
		*actSelectWarpsMatrix,
		*actSelectEigenwarpsMatrix,
		*actSelectPCAModel,
		*actQuit,
		*actAbout,
		*actHelp;

	actAbout = new QAction( tr("About %1").arg(APP_NAME), this );
	connect( actAbout, SIGNAL(triggered()), this, SLOT(about()) );

	actHelp = new QAction( tr("Help"), this );
	connect( actHelp, SIGNAL(triggered()), this, SLOT(help()) );

	actOpenVolume = new QAction( tr("&Load reference dataset (mean estimate)..."), this );
	actOpenVolume->setShortcut( tr("Ctrl+O") );
	actOpenVolume->setStatusTip( tr("Open reference volume dataset from disk on which the warpfields are applied.") );
	connect( actOpenVolume, SIGNAL(triggered()), this, SLOT(openVolumeDataset()) );

	actOpenNames = new QAction( tr("Load names..."), this );
	actOpenNames->setShortcut( tr("Ctrl+N") );
	actOpenNames->setStatusTip( tr("Open textfile containing dataset names (rows in textfile corresponding to columns in warps data matrix).") );
	connect( actOpenNames, SIGNAL(triggered()), this, SLOT(openNames()) );

	actOpenConfig = new QAction( tr("Open config..."), this );
	actOpenConfig->setIcon( QIcon(QPixmap(":/data/icons/document-open.png")) );
	connect( actOpenConfig, SIGNAL(triggered()), this, SLOT(openConfig()) );

	actOpenConfig2 = new QAction( tr("Open additional SDM config..."), this );
	connect( actOpenConfig2, SIGNAL(triggered()), this, SLOT(loadConfig2()) );

	actSaveConfigAs = new QAction( tr("Save config as..."), this );
	actSaveConfigAs->setIcon( QIcon(QPixmap(":/data/icons/document-save-as.png")) );
	connect( actSaveConfigAs, SIGNAL(triggered()), this, SLOT(saveConfigAs()) );

	actSaveConfig = new QAction( tr("&Save config"), this );
	actSaveConfig->setShortcut( tr("Ctrl+S") );
	actSaveConfig->setIcon( QIcon(QPixmap(":/data/icons/document-save.png")) );
	connect( actSaveConfig, SIGNAL(triggered()), this, SLOT(saveConfig()) );

	actGenerateConfig= new QAction( tr("Generate config..."), this );
	actGenerateConfig->setIcon( QIcon(QPixmap(":/data/icons/document-new.png")) );
	connect( actGenerateConfig, SIGNAL(triggered()), this, SLOT(generateConfig()) );

	actLoadLookmark = new QAction( tr("Load lookmark..."), this );
	connect( actLoadLookmark, SIGNAL(triggered()), this, SLOT(loadLookmark()) );

	actSaveLookmark = new QAction( tr("Save lookmark..."), this );
	connect( actSaveLookmark, SIGNAL(triggered()), this, SLOT(saveLookmark()) );

	actSyncTabs= new QAction( tr("Synchronize cameras"), this );
	actSyncTabs->setCheckable( true );
	actSyncTabs->setChecked( m_syncTabs );
	connect( actSyncTabs, SIGNAL(toggled(bool)), this, SLOT(setSyncTabs(bool)) );

	actSelectWarps = new QAction( tr("Select warps (eigenwarps or traitwarp)..."), this );
	actSelectWarps->setStatusTip( tr("Select warpfields for interavtice deformation of chosen reference (usually the eigenwarps of the SDM).") );
	connect( actSelectWarps, SIGNAL(triggered()), this, SLOT(selectWarps()) );

	actSelectWarpsMatrix = new QAction( tr("Select warps matrix (warpset.mat)..."), this );
	actSelectWarpsMatrix->setStatusTip( tr("Set the warpfield data matrix used in further analysis tasks (optional).") );
	connect( actSelectWarpsMatrix, SIGNAL(triggered()), this, SLOT(selectWarpsMatrix()) );

	actSelectEigenwarpsMatrix = new QAction( tr("Select eigenwarps matrix (eigenwarps.mat)..."), this );
	actSelectEigenwarpsMatrix->setStatusTip( tr("Set the eigenwarps data matrix used in further analysis tasks (optional).") );
	connect( actSelectEigenwarpsMatrix, SIGNAL(triggered()), this, SLOT(selectEigenwarpsMatrix()) );

	actSelectPCAModel = new QAction( tr("Select PCA model..."), this );
	connect( actSelectPCAModel, SIGNAL(triggered()), this, SLOT(selectPCAModel()) );

	actQuit = new QAction( tr("&Quit"), this );
	actQuit->setShortcut( tr("Ctrl+Q") );
	actQuit->setStatusTip( tr("Quit application.") );
	connect( actQuit, SIGNAL(triggered()), this, SLOT(close()) );

	// Scales dialog

	m_scalesDialog = new SDMVisScalesDialog( this );

	QAction* actScales = new QAction( tr("Specify eigenwarps scaling..."), this );
	connect( actScales, SIGNAL(triggered()), m_scalesDialog, SLOT(show()) );
	connect( m_scalesDialog, SIGNAL(scaleChanged(int,double)), this, SLOT(setScale(int,double)) );

	// Analysis actions

	QAction* actScatterPlot = new QAction( tr("Show scatter plot matrix"), this );
	connect( actScatterPlot, SIGNAL(triggered()), this, SLOT(showScatterPlot()) );

	QAction* actShowEigenvalues = new QAction( tr("Show eigenvalues"), this );
	connect( actShowEigenvalues, SIGNAL(triggered()), this, SLOT(showEigenvalues()) );

	// Debug / development actions

	QAction* actTestBatchProcessingDialog = new QAction( tr("Test BatchProcessingDialog"), this );
	connect( actTestBatchProcessingDialog, SIGNAL(triggered()), this, SLOT(testBatchProcessingDialog()) );

	QAction* actTestAutoScreenshots = new QAction( tr("Test AutoScreenshots"), this );
	connect( actTestAutoScreenshots, SIGNAL(triggered()), this, SLOT(testAutoScreenshots()) );

	QAction* actTestAutoScreenshots2 = new QAction( tr("Test AutoScreenshots 2"), this );
	connect( actTestAutoScreenshots2, SIGNAL(triggered()), this, SLOT(testAutoScreenshots2()) );

	QAction* actExpertMode = new QAction( tr("Expert mode"), this );
	actExpertMode->setCheckable( true );
	actExpertMode->setChecked( false );
	connect( actExpertMode, SIGNAL(toggled(bool)), this, SLOT(setExpertMode(bool)) );

	QAction* actOffscreen = new QAction( tr("Faster offscreen raycasting"), this );
	actOffscreen->setCheckable( true );
	actOffscreen->setChecked( true );
	m_volumeRenderer->toggleOffscreen( true );
	m_traitRenderer ->toggleOffscreen( true );
	connect( actOffscreen, SIGNAL(toggled(bool)), m_volumeRenderer, SLOT(toggleOffscreen(bool)) );
	connect( actOffscreen, SIGNAL(toggled(bool)), m_traitRenderer,  SLOT(toggleOffscreen(bool)) );

	QAction* actResizeWindow = new QAction( tr("Resize window"), this );
	connect( actResizeWindow, SIGNAL(triggered()), this, SLOT(customWindowResize()) );
	
	// -- Menu ----------------------------------------------------------------

	QMenu 
		*menuFile,
		*menuRaycaster,       // ifdef DEVELOPER_MODE
		*menuTraitRaycaster,  // ifdef DEVELOPER_MODE
		//m_menuVarVis		  // ifdef DEVELOPER_MODE && SDMVIS_VARVIS_ENABLED
		*menuAnalysis,        // ifdef DEVELOPER_MODE && SDMVIS_MANUAL_ANALYSIS
		*menuOptions,
		*menuWindows,
		*menuVTKVis,          // ifdef SDMVIS_VTKVISWIDGET_ENABLED
		*menuDebug,           // ifdef DEVELOPER_MODE
		*menuHelp;

	menuFile = menuBar()->addMenu( tr("&File") );
	menuFile->addAction( actOpenConfig );
	menuFile->addAction( actOpenConfig2 );
	menuFile->addAction( actGenerateConfig);
	menuFile->addAction( actSaveConfig );
	menuFile->addAction( actSaveConfigAs );
	menuFile->addSeparator();
	menuFile->addAction( actLoadLookmark );
	menuFile->addAction( actSaveLookmark );
	menuFile->addSeparator();
	menuFile->addAction( actOpenVolume );
	menuFile->addAction( actSelectWarps );
	menuFile->addAction( actScales );
	menuFile->addSeparator();
	menuFile->addAction( actQuit );

#ifdef DEVELOPER_MODE
	menuRaycaster = menuBar()->addMenu( tr("&Raycaster") );
	menuRaycaster->addAction( m_controlDock->toggleViewAction() );
	menuRaycaster->addSeparator();
	menuRaycaster->addActions( m_volumeRenderer->getActionsRenderer() );

	menuTraitRaycaster = menuBar()->addMenu( tr("&Trait Raycaster") );
	menuTraitRaycaster->addAction( m_controlDock->toggleViewAction() );
	menuTraitRaycaster->addSeparator();
	menuTraitRaycaster->addActions( m_traitRenderer->getActionsRenderer() );
	menuTraitRaycaster->setEnabled( false );

	m_menuRaycaster = menuRaycaster;
	m_menuTraitRaycaster = menuTraitRaycaster;

  #ifdef SDMVIS_VARVIS_ENABLED
	QAction *actVarVisOpen= new QAction( tr("Open volume dataset..."), this );
	actVarVisOpen->setStatusTip( tr("Open volume dataset from disk.") );	
	connect( actVarVisOpen,     SIGNAL(triggered()), this, SLOT(openVolumeDatasetVarVis()));
	
	QAction *actVarVisOpenWarp = new QAction( tr("Open warpfield dataset..."), this );
	actVarVisOpenWarp->setShortcut( tr("Ctrl+W") );	
	connect( actVarVisOpenWarp, SIGNAL(triggered()), this, SLOT(openWarpDatasetVarVis()));	

	m_menuVarVis = menuBar()->addMenu( tr("VarVis") );
	m_menuVarVis->addAction( actVarVisOpen );
	m_menuVarVis->addAction( actVarVisOpenWarp );

  #endif // SDMVIS_VARVIS_ENABLED

  #ifdef SDMVIS_TENSORVIS_ENABLED	
	m_tensorVisMenu = menuBar()->addMenu( tr("TensorVis") );
	m_tensorVisMenu->addActions( m_tensorVis->getEditActions() );
	m_tensorVisMenu->addSeparator();
	m_tensorVisMenu->addActions( m_tensorVis->getViewActions() );
	m_tensorVisMenu->addSeparator();
	m_tensorVisMenu->addActions( m_tensorVis->getFileActions() );
  #endif
#endif // DEVELOPER_MODE

#if defined(DEVELOPER_MODE) && defined(SDMVIS_MANUAL_ANALYSIS)
	menuAnalysis = menuBar()->addMenu( tr("Analysis") );

	QAction* actTraitDialog = new QAction( tr("Show trait dialog"), this );
	connect( actTraitDialog,  SIGNAL(triggered()), this, SLOT(showTraitDialog()) );

	QAction* actComputeTrait = new QAction( tr("Compute trait warpfield"), this );
	connect( actComputeTrait, SIGNAL(triggered()), this, SLOT(computeTraitWarpfield()) );

#ifdef SDMVIS_VARVIS_ENABLED
	QAction *actRefineROI= new QAction( tr("Refine ROI"), this );
	connect( actRefineROI,    SIGNAL(triggered()), this, SLOT(refineROI()));
#endif

	// manual specify what is now stored in config ini
	menuAnalysis->addAction( actOpenNames );
	menuAnalysis->addAction( actSelectWarpsMatrix );
	menuAnalysis->addAction( actSelectEigenwarpsMatrix );
	menuAnalysis->addAction( actSelectPCAModel );
	menuAnalysis->addSeparator();

	menuAnalysis->addActions( m_volumeRenderer->getActionsAnalysis() );
#ifdef SDMVIS_VARVIS_ENABLED
	menuAnalysis->addAction( actRefineROI );
#endif
	menuAnalysis->addSeparator();

	menuAnalysis->addAction( actTraitDialog );
	menuAnalysis->addAction( actComputeTrait );
	menuAnalysis->addSeparator();

	menuAnalysis->addAction( actShowEigenvalues );
	menuAnalysis->addAction( actScatterPlot );
#endif // DEVELOPER_MODE && SDMVIS_MANUAL_ANALYSIS

	menuOptions = menuBar()->addMenu( tr("&Options") );
	menuOptions->addAction( actSyncTabs);
	menuOptions->addAction( actOffscreen );
	menuOptions->addSeparator();
	menuOptions->addAction( actExpertMode);

	menuWindows = menuBar()->addMenu( tr("&Windows") );
	menuWindows->addAction( m_controlDock->toggleViewAction() );
	menuWindows->addSeparator();
	menuWindows->addAction( actShowEigenvalues );
	menuWindows->addAction( actScatterPlot );	
	menuWindows->addSeparator();
	menuWindows->addAction( actResizeWindow );

#ifdef SDMVIS_VTKVISWIDGET_ENABLED
	menuVTKVis = menuBar()->addMenu( tr("VTKVis") );
	
	menuVTKVis->addActions( vtkWidget->getActions() );
	QMenu *vmn_menuVTKVis = menuBar()->addMenu( tr("vmn_VTKVis") );
	vmn_menuVTKVis->addActions( vmn_vtkWidget->getActions() );
#endif

#ifdef DEVELOPER_MODE
	menuDebug = menuBar()->addMenu( tr("&Debug") );
	menuDebug->addAction( actTestBatchProcessingDialog );
	menuDebug->addAction( actTestAutoScreenshots );
	menuDebug->addAction( actTestAutoScreenshots2 );
	menuDebug->addSeparator();
	menuDebug->addActions( m_volumeRenderer->getActionsDebug() );
#endif

	menuHelp = menuBar()->addMenu( tr("&Help") );
	menuHelp->addAction( actAbout );
	menuHelp->addAction( actHelp );

	// -- Toolbar -------------------------------------------------------------

	// Meta actions
	QAction 
		*actToolTrackball  = new QAction( QIcon(":/data/icons/trackball.png"), 
			tr("SDM Camera"), this ),
		*actToolRubberband = new QAction( QIcon(":/data/icons/edit.png"), 
			tr("SDM Edit"), this ),
		*actToolTrait  = new QAction( QIcon(":/data/icons/trait2.png"), 
			tr("Trait"), this ),
		*actToolTensor = new QAction( QIcon(":/data/icons/tensor.png"), 
			tr("Tensor fields"), this ),
		*actToolROI    = new QAction( QIcon(":/data/icons/roi.png"),
			tr("ROI"), this ),
		*actToolGlyphs = new QAction( QIcon(":/data/icons/vectorfield.png"),
			tr("Vector fields"), this );

	actToolTrackball->setStatusTip(tr("Set interaction mode to camera control, "
		"switches to SDM raycaster.") );
	actToolTrackball->setToolTip(tr("Camera control"));
	actToolRubberband->setStatusTip(tr("Set interaction mode to rubber band, "
		"switches to SDM raycaster.") );
	actToolRubberband->setToolTip(tr("Rubber band"));
	actToolTrait->setStatusTip(tr("Switch to trait analysis."));
	actToolTrait->setToolTip(tr("Trait analysis"));
	actToolTensor->setStatusTip(tr("Switch to local covariance tensor field visualization."));
	actToolTensor->setToolTip(tr("Tensor field visualization"));
	actToolROI->setStatusTip(tr("Switch to region of interest selection and analysis."));
	actToolROI->setToolTip(tr("ROI analysis"));
	actToolGlyphs->setStatusTip(tr("Switch to vector field visualization."));
	actToolGlyphs->setToolTip(tr("Vector field visualization"));

	QToolBar* toolbar = addToolBar( tr("Toolbar") );
	toolbar->setObjectName( tr("Toolbar") );
	toolbar->addAction( actOpenConfig );
	toolbar->addSeparator();
	toolbar->addAction( actToolTrackball );
	toolbar->addAction( actToolRubberband );
	toolbar->addAction( actToolTrait );
#ifdef SDMVIS_VARVIS_ENABLED
	toolbar->addAction( actToolROI );
	toolbar->addAction( actToolGlyphs );
#endif
	toolbar->addAction( actToolTensor );

	toolbar->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
	toolbar->setIconSize( QSize(42,42) );


	connect( actToolTrackball, SIGNAL(triggered()), this, SLOT(onToolTrackball()) );
	connect( actToolRubberband,SIGNAL(triggered()), this, SLOT(onToolRubberband()) );
	connect( actToolTrait,     SIGNAL(triggered()), this, SLOT(onToolTrait()) );
	connect( actToolTensor,    SIGNAL(triggered()), this, SLOT(onToolTensor()) );
	connect( actToolROI,       SIGNAL(triggered()), this, SLOT(onToolROI()) );
	connect( actToolGlyphs,    SIGNAL(triggered()), this, SLOT(onToolGlyphs()) );

	//connect( actToolTrackball, SIGNAL(triggered()), m_volumeRenderer->set

	// Lookmark widget

	m_lookmarkWidget = new LookmarkWidget();

	QToolBar* toolbar2 = addToolBar( tr("Lookmark") );
	toolbar2->addWidget( m_lookmarkWidget );

	connect( m_lookmarkWidget, SIGNAL(setLookmarkFilename(QString)),
		this, SLOT(loadLookmark(QString)) );

	// -- Finish up -----------------------------------------------------------

	m_batchproc = new BatchProcessingDialog();
	readSettings();
#ifdef SDMVIS_VTKVISWIDGET_ENABLED
	vtkWidget->setBaseDir( m_baseDir );
#endif

#ifdef SDMVIS_VTK_ENABLED
	m_vtkCameraInterpolator = vtkCameraInterpolator::New();

	m_cameraAnimationTimer = new QTimer(this);
	connect( m_cameraAnimationTimer, SIGNAL(timeout()), this, SLOT(animateLookmark()) );
#endif

	setExpertMode( m_expertMode );

	statusMessage( tr("Ready") );
}

SDMVisMainWindow::~SDMVisMainWindow()
{
#ifdef SDMVIS_VTK_ENABLED
	m_vtkCameraInterpolator->Delete();
#endif
}

void SDMVisMainWindow::onToolGlyphs()
{
	m_centralTabWidget->setCurrentIndex( TabGlyphVis );
}

void SDMVisMainWindow::onToolROI()
{
	m_centralTabWidget->setCurrentIndex( TabSelectROI );
}

void SDMVisMainWindow::onToolTrackball()
{
	m_centralTabWidget->setCurrentIndex( TabRaycasterSDM );
	m_volumeRenderer->getActionModeTrackball()->trigger();
	m_volumeRenderer->update();
}

void SDMVisMainWindow::onToolRubberband()
{
	m_centralTabWidget->setCurrentIndex( TabRaycasterSDM );
	m_volumeRenderer->getActionModeRubberbandEditing()->trigger();
	m_volumeRenderer->update();
}

void SDMVisMainWindow::onToolTrait()
{
	m_centralTabWidget->setCurrentIndex( TabRaycasterTraitModel );
}

void SDMVisMainWindow::onToolTensor()
{
	m_centralTabWidget->setCurrentIndex( TabTensorVis );
}


void SDMVisMainWindow::customWindowResize()
{
	// Get difference window to viewport
	QRect window   = this->geometry();
	QRect viewport = this->centralWidget()->geometry();

	int dx = window.width() - viewport.width();
	int dy = window.height() - viewport.height();

	// Resize window such that viewport has the requested size
	int targetWidth = 1280,
	    targetHeight = 720;

	resize( targetWidth + dx, targetHeight + dy );

	// Sanity check
	viewport = this->centralWidget()->geometry();
	statusMessage( tr("Viewport resized to (%1,%2)")
						.arg(viewport.width())
						.arg(viewport.height()) );
}

void SDMVisMainWindow::about()
{
	QMessageBox::about( this, APP_NAME,
		tr("%1 %2 (Build %3)\n\n(c) %4")
		.arg(APP_NAME).arg(APP_VERSION/100.f).arg(__DATE__)
		.arg(APP_ORGANIZATION) );
}

void SDMVisMainWindow::help()
{
	QString helpPath = QApplication::applicationDirPath() + "/qthelpcollection.qhc";
	QFileInfo fi(helpPath);

	if( !fi.exists() )
	{
		qDebug() << "Help file not found in " << helpPath << "!";
		return;
	}
	qDebug() << "Help file path: " << fi.absoluteFilePath();

	QHelpEngine* he = new QHelpEngine(fi.absoluteFilePath());
	if( !he->setupData() )
	{
		qDebug() << "Help file setup failed!" << endl;
		qDebug() << "HelpEngine error state: " << he->error();
		return;
	}

	QTabWidget* helpNav = new QTabWidget();
	helpNav->addTab( he->contentWidget(), tr("Contents") );
	helpNav->addTab( he->indexWidget()  , tr("Index") );

	HelpBrowser* helpBrowser = new HelpBrowser( he );

	QSplitter* helpPanel = new QSplitter();
	helpPanel->addWidget( helpNav );
	helpPanel->addWidget( helpBrowser );
	helpPanel->setStretchFactor( 0, 1 );
	helpPanel->setStretchFactor( 1, 4 );

	he->contentWidget()->setModel( he->contentModel() );
	he->indexWidget  ()->setModel( he->indexModel  () );

	connect( he->contentWidget(), SIGNAL(linkActivated(const QUrl&)),
		     helpBrowser, SLOT(setSource(const QUrl&)) );

	helpPanel->setWindowTitle( tr("%1 - Help").arg(APP_NAME) );
	helpPanel->setWindowIcon( APP_ICON );
	helpPanel->show();
}

void SDMVisMainWindow::setExpertMode( bool expert )
{
#ifdef SDMVIS_VARVIS_ENABLED
	m_glyphControls->setExpertMode(expert);
	m_glyphControls->update();
	m_errorVisControls->setExpertMode(expert);
	m_errorVisControls->update();
	m_roiControls->setExpertMode(expert);
	m_roiControls->update();
#endif
#ifdef SDMVIS_TENSORVIS_ENABLED
	m_tensorVis->setExpertMode(expert);	
#endif
	m_expertMode=expert;
}

#ifdef SDMVIS_VTK_ENABLED
std::string getVTKCameraFilename( QString basename )
{
	return QString(basename + ".vtkcam").toStdString();
}

void SDMVisMainWindow::animateLookmark()
{
	double hz = (double)m_cameraAnimationTimer->interval() / 1000.0;
	m_cameraAnimationT += hz;

  #ifdef SDMVIS_TENSORVIS_ENABLED
	m_vtkCameraInterpolator->InterpolateCamera( m_cameraAnimationT, m_tensorVis        ->getRenderer()->GetActiveCamera() );
	m_vtkCameraInterpolator->InterpolateCamera( m_cameraAnimationT, m_tensorOverviewVis->getRenderer()->GetActiveCamera() );
	m_tensorVis        ->getRenderer()->GetRenderWindow()->Render(); m_tensorVis        ->repaint();
	m_tensorOverviewVis->getRenderer()->GetRenderWindow()->Render(); m_tensorOverviewVis->repaint();
  #endif
  #ifdef SDMVIS_VARVIS_ENABLED
 	m_vtkCameraInterpolator->InterpolateCamera( m_cameraAnimationT, m_varvisRender->getRenderer()->GetActiveCamera() );
	m_vtkCameraInterpolator->InterpolateCamera( m_cameraAnimationT, m_roiRender   ->getRenderer()->GetActiveCamera() );
	m_varvisRender->getRenderer()->GetRenderWindow()->Render(); m_varvisRender->repaint();
	m_roiRender   ->getRenderer()->GetRenderWindow()->Render(); m_roiRender   ->repaint();
  #endif

	if( m_cameraAnimationT > 1.0 )
		m_cameraAnimationTimer->stop();

	repaint();
	update();
}
#endif // SDMVIS_VTK_ENABLED

void SDMVisMainWindow::loadLookmark( QString filename )
{
	// Raycaster cameras
	m_volumeRenderer->loadCamera( filename );
	m_traitRenderer ->loadCamera( filename );

	// VTK cameras
#ifdef SDMVIS_VTK_ENABLED
	// Try to load VTK camera other visualizations as well	
	VTKPTR<vtkCamera> camera = VTKPTR<vtkCamera>::New();
	if( loadVTKCamera( camera, getVTKCameraFilename(filename).c_str() ) )
	{
		vtkCamera* lastCamera = m_tensorVis->getRenderer()->GetActiveCamera();

		m_vtkCameraInterpolator->Initialize();
		m_vtkCameraInterpolator->SetInterpolationTypeToSpline();
		m_vtkCameraInterpolator->AddCamera( 0.0, lastCamera );
		m_vtkCameraInterpolator->AddCamera( 1.0, camera );

		m_vtkCameraInterpolator->InterpolateCamera( 0.0, camera );

		m_cameraAnimationT = 0.0;
		m_cameraAnimationTimer->start( 33 );

	  #ifdef SDMVIS_TENSORVIS_ENABLED
		m_tensorVis        ->getRenderer()->SetActiveCamera( camera );
		m_tensorOverviewVis->getRenderer()->SetActiveCamera( camera );
	  #endif

	  #ifdef SDMVIS_VARVIS_ENABLED
		m_varvisRender->getRenderer()->SetActiveCamera( camera );
		m_roiRender   ->getRenderer()->SetActiveCamera( camera );
	  #endif
	}
#endif
}

void SDMVisMainWindow::loadLookmark()
{
	QString filename = QFileDialog::getOpenFileName( this,
		tr("sdmvis: Load camera lookmark..."), 
		m_baseDir, tr("Camera lookmark (*.cam)") );

	if( filename.isEmpty() )
		return;

	loadLookmark( filename );
}

void SDMVisMainWindow::saveLookmark()
{
	// save lookmark from "Trait Model" assuming cameras are synced
	
	QString filename = QFileDialog::getSaveFileName( this,
		tr("sdmvis: Save camera lookmark..."), 
		m_baseDir, tr("Camera lookmark (*.cam)") );

	if( filename.isEmpty() )
		return;

	m_traitRenderer->saveCamera( filename );

#if defined(SDMVIS_VTK_ENABLED) && defined(SDMVIS_TENSORVIS_ENABLED)
	// HACK: Save VTK camera of tensor visualization as well	
	saveVTKCamera( m_tensorVis->getRenderer()->GetActiveCamera(),
		getVTKCameraFilename(filename).c_str() );
#endif
}


#ifdef SDMVIS_VARVIS_ENABLED
void SDMVisMainWindow::clearVarVisBox()
{
	m_varvisControls->clearTraitSelection();
	m_glyphControls->clearTraitSelection();
}

void SDMVisMainWindow::openVolumeDatasetVarVis()
{
	QString filename;
	filename = QFileDialog::getOpenFileName( this,
		tr("Open volume dataset"),
		m_varvisBaseDir, tr("Volume description file (*.mhd)") );

	// cancelled?
	if( filename.isEmpty() )
		return;
	
	m_varvisRender->setGaussionRadius(m_varvisControls->getGaussionRadius());
	m_varvisRender->setNumberOfSamplingPoints(m_varvisControls->getSampleRange());
	m_varvisRender->setPointSize(m_varvisControls->getSampleRadius());

	if( !m_varvisRender->load_reference( filename.toAscii() ) )  // was: filename.c_str()
	{
		QMessageBox::warning( this, tr("Error setting volume"),
			tr("Error: Could not setup volume rendering correctly!") );
		
	}

	m_varvisRender->setVolumeVisibility(false);
	m_varvisRender->getRenderer()->ResetCamera();
	m_varvisRender->getRenderer()->GetActiveCamera()->Azimuth(-90);
	m_varvisRender->getRenderWindow()->Render(); // draw scene
}

void SDMVisMainWindow::openWarpDatasetVarVis()
{
	QString filename;
	filename = QFileDialog::getOpenFileName( this,
		tr("Open warpfield dataset"),
		m_varvisBaseDir, tr("Meta Image (*.mhd *.mha)") );

	// cancelled?
	if( filename.isEmpty() )
		return;

	// extract absolute path and filename
	QFileInfo info( filename );
	m_varvisBaseDir = info.absolutePath(); // name is info.fileName()	

	// replace current volume with temporary one
	m_varvisRender->setWarp(filename, m_varvisControls->get_BackwardsTr_State());
	m_varvisRender->getRenderWindow()->Render();

}
#endif // SDMVIS_VARVIS_ENABLED

void SDMVisMainWindow::workaround(int index)
{
	m_centralTabWidget->setCurrentIndex(index);
}

void SDMVisMainWindow::testBatchProcessingDialog()
{
	static BatchProcessingDialog bpd;
	BatchProcessingDialog::CommandList cmds;
	typedef BatchProcessingDialog::BatchCommand Command;
	cmds.push_back( Command( "foo", QStringList() << "bar", "this is foobar" ) );
	cmds.push_back( Command( "foo1",QStringList() << "bar1", "this is foobar1" ) );
	cmds.push_back( Command( "foo2",QStringList() << "bar2", "this is foobar2" ) );
	cmds.push_back( Command( "foo3",QStringList() << "bar3", "this is foobar3" ) );
	bpd.initBatch( cmds );
	bpd.show();
}

void SDMVisMainWindow::testAutoScreenshots2()
{
	if( m_config.getNames().empty() )
		openNames();

	QString inPath = QFileDialog::getExistingDirectory( this, tr("Directory with volume datasets"), m_baseDir );
	if( inPath.isEmpty() ) return;

	QString outPath = QFileDialog::getExistingDirectory( this, tr("Directory to store screenshots")	);
	if( outPath.isEmpty() ) return;

	qDebug() << "inPath = " << inPath;
	qDebug() << "outPath = " << outPath;

	int n = m_config.getNames().size();
	for( int i=0; i < n; i++ )
	{
		QString name = m_config.getNames().at(i);
		QString inFilename = inPath + QDir::separator() + name + ".mhd";
		qDebug() << "Taking screenshots for data item " << i+1 << "/" << n << ":" << name;

		if( openVolumeDataset( inFilename ) )
		{
			// Invoke render update
			m_volumeRenderer->updateGL();
			qApp->processEvents();
		
			// Make screenshot
			QString filename = outPath + QDir::separator() + name + ".png";
			m_volumeRenderer->makeScreenshot( filename );
		}
		else
			qDebug() << "Error: Could not open " << inFilename;
	}
}

void SDMVisMainWindow::testAutoScreenshots()
{
	QString path = QFileDialog::getExistingDirectory( this, tr("Directory to store screenshots") );
	if( path.isEmpty() ) return;

	SDMVisVolumeRenderer* vr = m_volumeRenderer;
	unsigned n   = m_sdm.getNumSamples(); // # individuals
	unsigned dim = n - 1;                 // # eigenmodes = PCA dimensionality
	for( unsigned i=0; i < n; i++ ) // i goes over individuals
	{
		QString name = m_config.getNames().at(i);
			// SDM names are not set correctly but show "Dataset1"...
			//   QString::fromStdString( m_sdm.getName(i) );
		qDebug() << "Taking screenshots for data item " << i+1 << "/" << n << ":" << name;

		// Set coefficients to reconstruct i-th individual
		QVector<double> lambdas;
		std::cout << "Coefficients = (";
		for( unsigned j=0; j < dim; j++ )
		{
			double lambda;
		  #if 0
			lambda = (double)m_sdm.getEigenvectors()(i,j);
		  #else
			lambda = (*m_config.getPlotterMatrix())(i,j);
		  #endif
			lambdas.push_back( lambda );
			std::cout << ((j<dim-1) ? ", " : "") << lambda;
		}
		std::cout << ")" << std::endl;
		vr->setLambdas( lambdas );

		// Invoke render update
		vr->updateGL();
		qApp->processEvents();
						
		// Make screenshot
		QString filename = path + "/" + name + ".png";
		vr->makeScreenshot( filename );
	}
}

void SDMVisMainWindow::closeEvent( QCloseEvent* event )
{
	writeSettings();
	QMainWindow::closeEvent( event );
}

void SDMVisMainWindow::setSyncTabs( bool enabled )
{
	m_syncTabs = enabled;	
}

void SDMVisMainWindow::tabIndexChanged( int index )
{
	static int lastTabIndex=0;

	// Synchronize raycaster cameras
	if( m_syncTabs )
	{
		if(lastTabIndex==TabRaycasterSDM)
			m_traitRenderer->setCamera(m_volumeRenderer->getCamera());

		if(lastTabIndex==TabRaycasterTraitModel)
			m_volumeRenderer->setCamera(m_traitRenderer->getCamera());
	}
	lastTabIndex = index;

	// Set control widget title
	if( index==TabRaycasterSDM )		// PCA raycaster
	{
		m_controlDock->setWindowTitle(tr("SDM Raycaster"));
	}
	else if( index==TabRaycasterTraitModel )  // Trait raycaster
	{
		m_controlDock->setWindowTitle(tr("Trait model"));
	}
#ifdef SDMVIS_VARVIS_ENABLED
	else if( index==TabGlyphVis )		// VarVis Glyph visualization
	{
		/* Several approaches to synchronize raycast camera with vtk Camera
								(NOT WORKING!)
		if(m_syncTabs) {
			vtkCamera* cam = m_varvisRender->getRenderWindow()->GetRenderers()->GetFirstRenderer()->GetActiveCamera();
			//QMatrix4x4 mat = m_volumeRenderer->cameraMatrix();
			QMatrix4x4 mat = m_volumeRenderer->getCamera().getModelviewMatrix();
			double mview[16];
			mat.copyDataTo( mview );
		
			// NOTE:
			// In VTK 5.8 we can call the following function:
			// cam->SetModelTransformMatrix( mview );

			//for( int i=0; i < 4; ++i )
			//	for( int j=0; j < 4; ++j )
			//		cam->GetViewTransformMatrix()->SetElement( i,j, mat(i,j) );

			//m_varvisRender->getRenderWindow()->GetInteractor()->Disable();
			//cam->GetViewTransformObject()->SetMatrix( mview );
			//m_varvisRender->getRenderWindow()->GetInteractor()->Enable();
			//m_varvisRender->getRenderWindow()->GetInteractor()->ReInitialize();

			//cam->SetViewUp( mview[1], mview[5], mview[9] );
			//cam->SetPosition( mview[12], mview[13], mview[14] );
		}
		*/

		m_controlDock->setWindowTitle(tr("Vectorfield visualization"));
	}
	else if( index==TabSelectROI )			// VarVis ROI Selection
	{
		m_controlDock->setWindowTitle(tr("Specify ROI"));
	}
	else if( index==TabQualityVis )			// VarVis Quality visualization
	{
		m_controlDock->setWindowTitle(tr("Quality visualization"));
	}
#endif
	else if( index==TabTensorVis )
	{
		m_controlDock->setWindowTitle(tr("Tensor visualization"));
	}
	else if( index==TabDatasetTable )		// Dataset table
	{
		m_controlDock->setWindowTitle(tr("Dataset table"));
	}

	// Adapt developer menu
	#ifdef DEVELOPER_MODE
		m_menuRaycaster     ->setEnabled(index==TabRaycasterSDM);
	  #ifdef SDMVIS_TRAITVIS_ENABLED
		m_menuTraitRaycaster->setEnabled(index==TabRaycasterTraitModel);
	  #endif
	  #ifdef SDMVIS_VARVIS_ENABLED
		m_menuVarVis        ->setEnabled(index==TabGlyphVis);
	  #endif
	  #ifdef SDMVIS_TENSORVIS_ENABLED
		m_tensorVisMenu     ->setEnabled(index==TabTensorVis);
	  #endif
	#endif
}

void SDMVisMainWindow::writeSettings()
{
	QSettings settings( APP_ORGANIZATION, APP_NAME );
	settings.setValue( "geometry"   , saveGeometry() );
	settings.setValue( "windowState", saveState()    );
	settings.setValue( "baseDir"    , m_baseDir      );
}

void SDMVisMainWindow::readSettings()
{
	QSettings settings( APP_ORGANIZATION, APP_NAME );
	m_baseDir = settings.value( "baseDir", QString("../data/") ).toString();
	restoreGeometry( settings.value("geometry")   .toByteArray() );
	restoreState   ( settings.value("windowState").toByteArray() );
	restoreDockWidget( m_controlDock );
}

void SDMVisMainWindow::openNames()
{
	using namespace std;

	QString filename = QFileDialog::getOpenFileName( this,
		tr("Open names..."),
		m_baseDir, tr("Texfile with newline separated dataset names (*.txt)") );

	if( filename.isEmpty() )
		return;

	ifstream f( filename.toAscii() );
	if( !f.is_open() )
	{		
		QMessageBox::warning( this, tr("sdmvis: Error loading names"),
			tr("Could not open %1!").arg(filename) );
		statusMessage( tr("Failed to load names \"%1\"").arg(filename) );
		return;
	}

	QStringList names;
	while( f.good() )
	{
		string line;
		getline( f, line );
		
		QString s = QString::fromStdString(line).trimmed();
		if( !s.isEmpty() && !s.startsWith("#") && !s.startsWith("//") )
			names.push_back( s );
	}
	f.close();

	if( names.size() > 0 )
		setNames( names );

	else
	{
		QMessageBox::warning( this, tr("sdmvis: Error loading names"),
			tr("No valid names found in %1!").arg(filename) );
		statusMessage( tr("Failed to load names \"%1\"").arg(filename) );
		return;
	}
	
	statusMessage( tr("Successfully loaded names \"%1\"").arg(filename) );
}

void SDMVisMainWindow::setNames( QStringList names )
{
	m_config.setNames( names );
	m_traitRenderer->getControlWidget()->getTraitSelector()->setNames(names);
}

void SDMVisMainWindow::setTraitScale( int traitID, double value )
{
//	m_config.traits.at(traitID).elementScale = value;
	m_traitRenderer->setScale( 0, value );
}

void SDMVisMainWindow::setScale( int idx, double value )
{
	if( idx >=0 && idx < m_config.getWarpfields().size() )
	{
		m_config.warpfields()[idx].elementScale = value;
		m_volumeRenderer->setScale( idx, value );		
	}
}

QString SDMVisMainWindow::getBasePath()
{
	return m_config.getBasePath().isEmpty() ? m_baseDir : m_config.getBasePath();
}

void SDMVisMainWindow::selectWarpsMatrix()
{
	QString filename = QFileDialog::getOpenFileName( this,
		tr("Select warps data matrix..."),
		getBasePath(), tr("Raw data matrix (*.mat)") );

	if( filename.isEmpty() )
		return;

	// update config

	QString relPathName=filename;
	relPathName.remove(m_baseDir+"/");

	m_config.setWarpsMatrix( relPathName );
	
	statusMessage( tr("Set warps data matrix \"%1\"").arg(filename) );
}

void SDMVisMainWindow::selectEigenwarpsMatrix()
{
	QString filename = QFileDialog::getOpenFileName( this,
		tr("Select eigenwarps data matrix..."),
		getBasePath(), tr("Raw data matrix (*.mat)") );

	if( filename.isEmpty() )
		return;

	// update config
	
	QString relPathName=filename;
	relPathName.remove(m_baseDir+"/");
	m_config.setEigenwarpsMatrix( relPathName );
	statusMessage( tr("Set eigenwarps data matrix \"%1\"").arg(filename) );
}

void SDMVisMainWindow::selectPCAModel()
{
	QString filename;
	int N = m_config.getNames().size();

	// names must be given to know dataset size
	if( m_config.getNames().isEmpty() )
	{
		QMessageBox::warning( this, "sdmvis: Warning",
			tr("Dataset names must be specified first!") );
		return;
	}
	
	// --- Scatter matrix ---

	filename = QFileDialog::getOpenFileName( this,
		tr("Select scatter matrix..."),
		getBasePath(), tr("Raw data matrix (*.mat)") );

	if( filename.isEmpty() )
		return;

	// update config
	if( !m_config.loadScatterMatrix( filename, N ) )
	{
		QMessageBox::warning( this, "sdmvis: Warning", m_config.getErrMsg() );
		return;
	}

	// --- PCA eigenvectors ---

	filename = QFileDialog::getOpenFileName( this,
		tr("Select PCA eigenvectors matrix..."),
		getBasePath(), tr("Raw data matrix (*.mat)") );
	
	if( filename.isEmpty() )
		return;

	// update config
	if( !m_config.loadPCAEigenvectors( filename, m_config.getScatterMatrix().size1() ) )
	{
		QMessageBox::warning( this, "sdmvis: Warning", m_config.getErrMsg() );
		return;
	}

		
	// --- PCA eigenvalues (lambda) ---

	filename = QFileDialog::getOpenFileName( this,
		tr("Select PCA eigenvalues vector..."),
		getBasePath(), tr("Raw data matrix (*.mat)") );
	
	if( filename.isEmpty() )
		return;

	// update config
	if( !m_config.loadPCAEigenvalues( filename, m_config.getPCAEigenvectors().size2() ) )
	{
		QMessageBox::warning( this, "sdmvis: Warning", m_config.getErrMsg() );
		return;
	}

	m_volumeRenderer->getControlWidget()->getPlotWidget()->setDisabled(false);	
//	this->m_traitRenderer->getControlWidget()->getTraitSelector()->setDisabled(false);
	setupConnections();
	loadWidgetsContents();// getPlotWidget() contents
}

void SDMVisMainWindow::selectWarps()
{
	QStringList filenames = QFileDialog::getOpenFileNames( this,
		tr("Open warpfields..."), 
		m_baseDir, tr("Volume description file (*.mhd)") );

	// user cancelled?
	if( filenames.empty() )
		return;

	QList<Warpfield> warpfields;
	for( int i=0; i < filenames.size(); ++i )
	{
		Warpfield w;
		w.mhdFilename  = filenames.at(i);
		w.elementScale = 1.f;
		warpfields.push_back( w );
	}
	
	setWarpfields( warpfields );
}

bool SDMVisMainWindow::setWarpfields( QList<Warpfield> warpfields )
{
	// Update config
	m_config.setEigenwarps( warpfields );

	// Update UI
	m_scalesDialog->setSDMVisConfig( &(this->m_config) );

	// Warpfields are set only for "PCA Model" raycaster
	if( !m_volumeRenderer->setWarpfields( warpfields ) )
	{
		statusMessage("Error in raycaster warpfield setup!");
		return false;
	}

#ifdef SDMVIS_VARVIS_ENABLED
	// Set image spacing and resolution

	// FIXME: getVolumeData() returns pointer to a warpfield which may have
	//        different resoltion and spacing than the loaded image volume.
	m_varvisRender->setElementSpacing(m_volumeRenderer->getVolumeData()->spacingX(),
									  m_volumeRenderer->getVolumeData()->spacingY(),
									  m_volumeRenderer->getVolumeData()->spacingZ());

	m_varvisRender->setResolution(m_volumeRenderer->getVolumeData()->resX(),
									  m_volumeRenderer->getVolumeData()->resY(),
									  m_volumeRenderer->getVolumeData()->resZ());

	m_roiRender->setElementSpacing(m_volumeRenderer->getVolumeData()->spacingX(),
									  m_volumeRenderer->getVolumeData()->spacingY(),
									  m_volumeRenderer->getVolumeData()->spacingZ());

	m_roiRender->setResolution(m_volumeRenderer->getVolumeData()->resX(),
									  m_volumeRenderer->getVolumeData()->resY(),
									  m_volumeRenderer->getVolumeData()->resZ());

	// UI bridging code (??)
	if (m_traitRenderer->getControlWidget()->getTraitSelector()->getPushToVarVis() &&
		m_traitRenderer->getControlWidget()->getTraitSelector()->getSelectionBox()->count()>0)
	{
		m_varvisControls->addTraitSelection(m_traitRenderer->getControlWidget()->getTraitSelector()->getSelectionBox());
		m_glyphControls ->addTraitSelection(m_traitRenderer->getControlWidget()->getTraitSelector()->getSelectionBox());
	}
	
	// Force VarVis to load warpfields
	//m_glyphControls->getEigenWarps(); // <- requires connections not available w/o config
	QStringList filenames;
	for( int i=0; i < warpfields.size(); i++ )
		filenames << warpfields.at(i).mhdFilename;
	m_glyphControls->setEigenwarps( filenames );
#endif

	statusMessage( tr("Set %1 warpfields").arg(m_config.warpfields().size()) );
	return true;
}

bool SDMVisMainWindow::setVolume( QString mhdFilename )
{	
	// Relative path name
	QString relPathName=mhdFilename;
	relPathName.remove(m_baseDir+"/");

	// Update config
	m_config.setReference( relPathName );

	// --- Setup raycaster(s) ---
	if( !m_volumeRenderer->loadVolume( mhdFilename.toAscii() ) )
	{
		QMessageBox::warning( this, tr("sdmvis: Error"),
			tr("Error: Could not load volume %1!").arg(mhdFilename) );
		return false;
	}
	if( !m_traitRenderer->loadVolume( mhdFilename.toAscii() ) )
	{
		QMessageBox::warning( this, tr("sdmvis: Error"),
			tr("Error: Could not load volume %1!").arg(mhdFilename) );
		return false;
	}	

	m_traitRenderer ->setWorkingDirectory( m_baseDir );
	m_volumeRenderer->setWorkingDirectory( m_baseDir );

#if 1  // DEBUGGING

#ifdef SDMVIS_VARVIS_ENABLED
	// --- Setup VTK renderer(s) ---

	// Load reference volume and set some UI defaults
	m_varvisRender->load_reference( mhdFilename.toAscii() );	
	m_varvisRender->setVolumeVisibility(false);

	// Initialize VarVisRenderer for ROI with same volume
	m_roiRender->clearRoi();
	m_roiRender->setVolume( m_varvisRender->getVolume(), mhdFilename ); 
	//m_roiRender->load_reference( mhdFilename.toAscii() );
	m_roiRender->setup();
	m_roiRender->setVolumeVisibility ( false );
	m_roiRender->setMeshVisibility   ( true  ); // ?
	m_roiRender->setRefMeshVisibility( true  );
#endif

#ifdef SDMVIS_TENSORVIS_ENABLED
  #ifdef SDMVIS_VARVIS_ENABLED
	// Re-use the volume from VarVis
	m_tensorVis->setVolume( m_varvisRender->getVolume() );
	m_tensorOverviewVis->setReference( m_varvisRender->getVolume() );
  #else
	// Load volume from disk
	assert(false); // NOT IMPLEMENTED YET
  #endif
#endif

#endif

	statusMessage( tr("Set reference volume \"%1\"").arg(relPathName) );
	return true;
}

void SDMVisMainWindow::openVolumeDataset()
{
	QString filename;
	filename = QFileDialog::getOpenFileName( this,
		tr("Open volume dataset"),
		m_baseDir, tr("Volume description file (*.mhd)") );

	// cancelled?
	if( filename.isEmpty() )
		return;

	openVolumeDataset( filename );
}

bool SDMVisMainWindow::openVolumeDataset( QString filename )
{
	// extract absolute path and filename
	QFileInfo info( filename );
	m_baseDir = info.absolutePath(); // name is info.fileName()	

	// load volume dataset
	statusMessage( tr("Loading volume dataset %1...").arg(info.fileName()) );

	// replace current volume with temporary one
	bool success = setVolume( filename );
	
	//updateActions();
	this->m_traitRenderer->getControlWidget()->getTraitSelector()->setPath(m_baseDir);
	// succesfully loaded volume
	if( success ) {
		statusMessage( tr("Successfully loaded %1").arg(info.fileName()) );
		setWindowTitle( APP_NAME + " - " + info.fileName() );
	} else {
		statusMessage( tr("Failed to load %1").arg(info.fileName()) );
		setWindowTitle( APP_NAME );
	}

	return success;
}

void SDMVisMainWindow::generateConfig()
{
	ConfigGenerator *cfgGen= new ConfigGenerator();
	cfgGen->setWindowIcon( APP_ICON );
	cfgGen->setWindowTitle( tr("%1 - Generate config wizard").arg(APP_NAME) );
	cfgGen->setConfigPath(m_baseDir);
	cfgGen->setConfig(&m_config);
	connect(cfgGen,SIGNAL(configGenerated(QString)),this,SLOT(loadConfig(QString)));
	cfgGen->show();
}

void SDMVisMainWindow::loadWarpfield(QString path)
{
	QList<Warpfield> tempList;
	Warpfield temp;
	temp.mhdFilename=m_config.getAbsolutePath(path);
	temp.elementScale=0.2f;
	tempList.push_back(temp);

	this->m_traitRenderer->setWarpfields(tempList);
}

void SDMVisMainWindow::loadConfig2()
{
	QString filename = QFileDialog::getOpenFileName( this, 
		tr("Load additional SDM config"), m_baseDir, tr("SDM config ini (*.ini)") );

	if( filename.isEmpty() )
		return;

	// Temporarily change working directory to chosen one
	QDir prevcwd = QDir::current();
	QFileInfo fi(filename);
	if( !QDir::setCurrent( fi.absoluteDir().path() ) )
	{
		QMessageBox::warning( this,
			tr("sdmvis"), 
			tr("Could not change directory to %1!").arg(fi.absoluteDir().path()) );
		return;
	}
		
	if( m_sdm.loadIni( filename.toStdString().c_str() ) )
	{
		m_volumeRenderer->setWarpfieldsFromSDM( 3 ); // FIXME: Hard coded number of modes.
	}
	else
	{
		QMessageBox::warning( this, 
			tr("sdmvis: Error loading SDM config"),
			tr("Could load config:\n%1\n")
			   .arg( filename ) );
	}

	// Restore working directory
	QDir::setCurrent( prevcwd.path() );
}

void SDMVisMainWindow::loadConfig(QString filename)
{
	statusMessage(tr("Loading config %1...").arg(filename));

	if (s_cfgPresent)
		unloadConfig();

	// read config from .ini file
	if( !m_config.readConfig( filename ) )
	{
		QMessageBox::warning( this, APP_NAME, m_config.getErrMsg() );
		statusMessage(
					tr("Failed to apply config %1").arg(filename) );
		return;
	}

	QFileInfo info( filename );
	m_baseDir = m_config.getBasePath();

	// check if there is an associated sdmproc ini file
	bool sdmini = false;
	if( !m_config.getAssociatedSDMProcIni().isEmpty() )
	{
		QString sdminifile = info.absolutePath() + "/" + m_config.getAssociatedSDMProcIni();
		qDebug() << "Trying to load additional sdm config from " << sdminifile;
		if( m_sdm.loadIni( sdminifile.toStdString().c_str(), 0 ) )
		{
			sdmini = true;
			qDebug() << "Associated SDMProc ini file setup";
		}
		else
			qDebug() << "Associated SDMProc ini file specified but could *not* be setup!";
	}

	// make sanity check on files 
	bool failed=false;
	if(!QFile::exists(m_config.getAbsolutePath(m_config.getReference())))
	{
		QMessageBox::warning( this, tr("sdmvis: Error applying new configuration"),
			tr("Could not load reference specified in config:\n%1\n"
			   "(Keep in mind that the config file works with relative paths.)")
			   .arg( m_config.getReference() ) );

		statusMessage(
			tr("Failed to apply config %1").arg(info.fileName()) );
		return;
	}
	
	// apply new configuration

	// Setup SDM structure
	bool warpfieldsSetFromSDM = false;
	if( sdmini )
	{
		statusMessage("Setup statistical deformation model...");

		// Load associated SDM ini

		// Temporarily change working directory to chosen one
		QDir prevcwd = QDir::current();
		QFileInfo fi(filename);
		if( !QDir::setCurrent( fi.absoluteDir().path() ) )
		{
			QMessageBox::warning( this,
				tr("sdmvis"), 
				tr("Could not change directory to %1!").arg(fi.absoluteDir().path()) );
			return;
		}

		// Load SDM ini
		if( m_sdm.loadIni(  m_config.getAssociatedSDMProcIni().toStdString().c_str() ) )
		{
			m_volumeRenderer->setSDM( &m_sdm );
		}
		else
		{
			QMessageBox::warning( this, 
				tr("sdmvis: Error loading SDM config"),
				tr("Could not load config:\n%1\n")
				   .arg( filename ) );
		}

		// Configure eigenwarps for scale parameter
		m_config.eigenwarps().clear();
		for( int i=0; i < std::max( (int)m_sdm.getNumSamples()-1, 5 ); i++ )
			m_config.eigenwarps().push_back( Warpfield() );

		// Restore working directory
		QDir::setCurrent( prevcwd.path() );
	}

	// reference volume is set for all raycasters
	statusMessage("Setup reference volume...");
	if( !setVolume( m_config.getReference() ) )
	{
		QMessageBox::warning( this, tr("sdmvis: Error applying new configuration"),
			tr("Could not setup reference specified in config:\n%1\n")
			   .arg( m_config.getReference() ) );

		statusMessage(
			tr("Failed to apply config %1").arg(info.fileName()) );
		return;
	}

	// warpfields are set only for "PCA Model" raycaster
	statusMessage("Setup warpfields...");
	if( sdmini )
	{
		// FIXME: Hard coded number of modes.
		warpfieldsSetFromSDM = 
			m_volumeRenderer->setWarpfieldsFromSDM( 
			std::min(5, (int)m_sdm.getEigenvalues().size()) );
	}

	if( !warpfieldsSetFromSDM )
	{
		if( !setWarpfields( m_config.getWarpfields() ) )
		{
			QMessageBox::warning( this, tr("sdmvis: Error applying new configuration"),
				tr("Could not setup all warpfields specified in config!\n") );

			statusMessage(
				tr("Failed to apply config %1").arg(info.fileName()) );
			return;
		}
	}

	// names are set for all SDMVisVolumeRenderer
	setNames( m_config.getNames() );


	if( !sdmini )
	{
		// Setup SDM from config
		unsigned numFields = m_config.getNames().size();
		//m_sdm.loadPrecomputedMatrices( numFields,
		//	m_config.getWarpsMatrix().toStdString().c_str(),
		//	m_config.getEigenwarpsMatrix().toStdString().c_str() );
		m_sdm.loadEigenmodes( numFields,
			m_config.getEigenwarpsMatrix().toStdString().c_str() );
		m_sdm.setScatterMatrix( m_config.getScatterMatrix() );
		m_sdm.setEigenvectors ( m_config.getPCAEigenvectors() );
		m_sdm.setEigenvalues  ( m_config.getPCAEigenvalues() );
		m_sdm.setResolution( 
			m_volumeRenderer->getAnyWarpfield()->resX(),
			m_volumeRenderer->getAnyWarpfield()->resY(),
			m_volumeRenderer->getAnyWarpfield()->resZ() );
		m_sdm.setSpacing(
			m_volumeRenderer->getAnyWarpfield()->spacingX(),
			m_volumeRenderer->getAnyWarpfield()->spacingY(),
			m_volumeRenderer->getAnyWarpfield()->spacingZ() );

		m_volumeRenderer->setSDM( &m_sdm );
	}

	m_volumeRenderer->setConfig( &m_config, false );


	setupConnections(); // connect slots for getPlotWidget() and m_TraitSelecor

	m_volumeRenderer->getControlWidget()->getPlotWidget()->setConfigName(info.fileName());
	m_volumeRenderer->getControlWidget()->getPlotWidget()->clearTraitSeletion();
	m_traitRenderer->getControlWidget()->getTraitSelector()->setConfigName(info.fileName());
	m_traitRenderer->getControlWidget()->getTraitSelector()->clearTrait();
	m_traitRenderer->getControlWidget()->getTraitSelector()->setDisabled(false);
	loadWidgetsContents(); // load all widgets contents (plotter and traitSelector)

#ifdef SDMVIS_VARVIS_ENABLED	
	clearVarVisBox();

	QString ref				= m_config.getReference(); 
	
	m_varvisRender  ->setPointRadius    ( m_config.getSamplePointSize() );
	m_varvisControls->setSamplePointSize( m_config.getSamplePointSize() );
	
	// FIXME: Deprecated
	// varvisBaseDir is only used by the deprecated VarVis functions
	// openVolumeDatasetVarVis() and openWarpDatasetVarVis()
	QFileInfo varVisInfo( m_config.getReference() );
	m_varvisBaseDir = varVisInfo.absolutePath();

	// Set directory where ROI and trait data resides (?)
	m_varvisControls->setPathToMask( m_baseDir+"/"+info.baseName() );
	m_roiControls   ->setPathToMask( m_baseDir+"/"+info.baseName() );
	
	// Add ROI Spheres from config
	for( int iA=0; iA < m_config.getRoiVector().size(); iA++ )
		m_roiRender->generateROI( iA, 
				m_config.getRoiVector().at(iA).radius,
				m_config.getRoiVector().at(iA).center[0],
				m_config.getRoiVector().at(iA).center[1],
				m_config.getRoiVector().at(iA).center[2]
		);	
	m_roiRender->getSelectionBox()->setCurrentIndex(0);
	m_roiRender->specifyROI(0);	

#endif // SDMVIS_VARVIS_ENABLED

#ifdef SDMVIS_TENSORVIS_ENABLED
	statusMessage("Setup tensor visualization...");
	m_tensorVis->setSDM( &m_sdm );
 // #if 0 //#ifdef SDMVIS_VARVIS_ENABLED  (This is not working yet correctly!)
	//// Use the reference volume from VarVis
	//m_tensorVis->setVolume( m_varvisRender->getVolume() );
 // #else
	//// Load reference volume from disk
	//m_tensorVis->setVolume( m_config.getReference() );
 // #endif
	m_tensorVis->setOpenBaseDir( m_baseDir );

	m_tensorVis->setProbes( m_config.getTensorProbes() );

	statusMessage("Setup tensor overview visualization...");
	m_tensorOverviewVis->setTensorData( m_tensorVis->getSDMTensorDataProvider() );
	m_tensorOverviewVis->setTensorVis ( m_tensorVis->getTensorVis() );
#endif
	
	statusMessage("Finalizing setup...");

	// WORKAROUND: Removed this call to setConfig() for trait renderer while 
	//   debugging a crash when using interactive editing in model renderer and
	//   switching afterwards to trait renderer.
	// m_traitRenderer ->setConfig( &m_config, false );

	// Set some defaults (maybe this should go in the config.ini)
	double isovalueRelative = 0.1;
	m_volumeRenderer->setIsovalueRelative( isovalueRelative );
	m_traitRenderer ->setIsovalueRelative( isovalueRelative );

	// Set first lookmark (if one given)
	if( !m_config.getLookmarks().isEmpty() )
	{
		QString lookmarkFilename = 
			m_config.getAbsolutePath( m_config.getLookmarks().at(0) );
		loadLookmark( lookmarkFilename );
	}

	m_volumeRenderer->update();
	m_traitRenderer ->update();

	// Update lookmarks widget
	m_lookmarkWidget->setConfig( m_config );

	// set active config name
	m_loadedConfigName = filename;	

	// succesfully loaded config
	statusMessage(tr("Successfully loaded config %1").arg(info.fileName()) );
	setWindowTitle( APP_NAME + " - " + info.fileName() );	
	s_cfgPresent=true;
}

void SDMVisMainWindow::statusMessage( QString message )
{
	statusBar()->showMessage(message);
	std::cout << "Status: " << message.toStdString() << "\n";
}

void SDMVisMainWindow::unloadConfig()
{
	// TODO clear MEMORY!
	QList<Trait> traitList=m_config.getTraits();	
	traitList.clear();
	m_traitRenderer->getControlWidget()->getTraitSelector()->clearTrait();
	m_volumeRenderer->getControlWidget()->getPlotWidget()->reset();
#ifdef SDMVIS_VARVIS_ENABLED
	m_glyphControls->clearEigenwarps();
#endif
	m_tempTraitList.clear();
	int newSize=traitList.size();
	bool traitEmpty=m_config.isTraitListEmpty();
	m_config.eigenwarps().clear();
	m_config.clearRoi();
	m_config.getLookmarks().clear();
	this->m_lookmarkWidget->clear();	
	// reset active config name
	m_loadedConfigName = "";
}

void SDMVisMainWindow::openConfig()
{
	QString filename;
	filename = QFileDialog::getOpenFileName( this,
		tr("Open sdmvis config file"),
		m_baseDir, tr("SDMVis configuration file (*.ini)") );

	if( filename.isEmpty() )
		return;
	
	loadConfig(filename);
}

void SDMVisMainWindow::loadWidgetsContents()
{
	// scatterPlotMatrix	
	m_scatterPlotWidget->prepareMatrix(
			m_config.getPlotterMatrix()->size1(),
			m_config.getPlotterMatrix()->size2(),
			m_config.getPlotterMatrix(),
			m_config.getPlotScaling(),
			m_config.getNames() );

	// volumeRenderer
	m_volumeRenderer->getControlWidget()->getPlotWidget()->setDisabled(false);
	
	m_volumeRenderer->getControlWidget()->getPlotWidget()->prepareMatrix(
		m_config.getPlotterMatrix()->size1(),
		m_config.getPlotterMatrix()->size2(),
		m_config.getPlotterMatrix(),
		m_config.getPlotScaling(),
		m_config.getNames(),
		m_baseDir,
		"");

	// traitRenderer
	m_traitRenderer->getControlWidget()->getTraitSelector()->setPath(m_baseDir);
	m_traitRenderer->getControlWidget()->getTraitSelector()->loadMatrix(m_config.getPlotterMatrix());
	m_traitRenderer->getControlWidget()->getTraitSelector()->clearTrait();

	bool isTraitEmpty=m_config.isTraitListEmpty();
	m_traitRenderer->getControlWidget()->getTraitSelector()->enable_TraitProperties(false);
#ifdef SDMVIS_VARVIS_ENABLED
	m_varvisControls->enableTraitsFromSDMVIS(false);
	m_glyphControls->enableTraitsFromSDMVIS(false);
	m_traitRenderer->getControlWidget()->getTraitSelector()->pushTraitToVarVis(false);
#endif
	if (!m_config.isTraitListEmpty())
	{
		// we do have a trait list 
		m_traitRenderer->setWarpfields(m_config.getFirstTraits());
		m_tempTraitList=m_config.getTraits();
		m_traitRenderer->getControlWidget()->getTraitSelector()->setTraitList(m_config.getTraits());
		m_traitRenderer->getControlWidget()->getTraitSelector()->setFirstValidation();
		
		m_traitRenderer->getControlWidget()->getTraitSelector()->setIndex(0);
		// set Trait list to Plot Widget
		m_volumeRenderer->getControlWidget()->getPlotWidget()->setTraitList(m_config.getTraits());
		
		
		Trait specificTrait=m_config.getTraits().at(0);
		specificTrait.computeNormal();

		m_traitRenderer->setTitle( specificTrait.identifier );

		m_volumeRenderer->getControlWidget()->getPlotWidget()->setTrait( specificTrait ); 
		m_scatterPlotWidget->setTrait( specificTrait );
		m_traitRenderer->getControlWidget()->getTraitSelector()->enable_TraitProperties(true);
#ifdef SDMVIS_VARVIS_ENABLED
		m_varvisControls->enableTraitsFromSDMVIS(true);
		m_glyphControls->enableTraitsFromSDMVIS(true);
#endif
		m_volumeRenderer->getControlWidget()->getPlotWidget()->setIndex(0);
		m_volumeRenderer->getControlWidget()->getPlotWidget()->drawTraits(true);
	}
	else
	{
		m_traitRenderer->setTitle(tr("(No traits available.)"));
		m_traitRenderer->getControlWidget()->getBarPlotWidget()->setDisabled(true);
	}
}

void SDMVisMainWindow::setupConnections()
{
	////////////////////////////////////////////////////////////////
	//  Disconnnect everything
	////////////////////////////////////////////////////////////////

	// first of all disconnect them 
	disconnect(m_volumeRenderer->getControlWidget()->getPlotWidget(), // plotter
			SIGNAL(sig_drawTraits(bool)),
			m_scatterPlotWidget,
			SLOT(drawTraitLine(bool)));

	disconnect(m_volumeRenderer->getControlWidget()->getPlotWidget(), // plotter
			SIGNAL(sig_drawSigma(bool)),
			m_scatterPlotWidget,
			SLOT(drawSigma(bool)));

	disconnect(m_volumeRenderer->getControlWidget()->getPlotWidget(), // plotter
			SIGNAL(changeItem(int,int,int,double,double)),
			m_volumeRenderer->getControlWidget()->getBarPlotWidget(), // bar plot widget
			SLOT(changeItem(int,int,int,double,double)));

	disconnect(m_volumeRenderer->getControlWidget()->getPlotWidget(),
			SIGNAL(traitChanged(int)),this,SLOT(loadNewTrait(int)));
		
	disconnect(m_volumeRenderer->getControlWidget()->getPlotWidget(),
			SIGNAL(changeItem(int,double)),
			m_volumeRenderer->getControlWidget()->getBarPlotWidget(),
			SLOT(changeItem(int,double)));

	disconnect(m_volumeRenderer->getControlWidget()->getBarPlotWidget(),
			SIGNAL(valueChanged(int,double)),
			m_volumeRenderer->getControlWidget()->getPlotWidget(),
			SLOT(updateMovingPointPosition(int,double)));

	// connections for getTraitSelector()
#ifdef SDMVIS_VARVIS_ENABLED
	disconnect(m_traitRenderer->getControlWidget()->getTraitSelector(),
			SIGNAL(clearVarVisBox()),
			this,
			SLOT(clearVarVisBox()));

	disconnect(m_glyphControls,
			SIGNAL(setwarpId(int)),
			this,
			SLOT(setNewTrait(int)));

	m_glyphControls->enableTraitsFromSDMVIS(true);
	disconnect(m_glyphControls,
			SIGNAL(SIG_getTraitsfromSDMVIS(bool)),
			m_traitRenderer->getControlWidget()->getTraitSelector(),
			SLOT(pushTraitToVarVis(bool)));

	disconnect(m_roiControls,SIGNAL(calculateROI()),this,SLOT(refineROI()));
	disconnect(m_roiControls,SIGNAL(statusMessage(QString)),this,SLOT(statusMessage(QString)));

#endif

	disconnect(m_traitRenderer->getControlWidget()->getTraitSelector(),
			SIGNAL(newTraitInTown(Trait,int)),this, SLOT(addTrait(Trait,int)));
	
	disconnect(m_traitRenderer->getControlWidget()->getTraitSelector(),
			SIGNAL(loadWarpfield(QString)),this, SLOT(loadWarpfield(QString)));	

	disconnect(m_traitRenderer->getControlWidget()->getBarPlotWidget(),
			SIGNAL(valueChanged(int,double)),
			m_traitRenderer->getControlWidget()->getTraitSelector(),
			SLOT(setBarValue(int,double)));

	disconnect(m_traitRenderer->getControlWidget()->getTraitSelector(),
			SIGNAL(loadBarValue(int,double)),
			m_traitRenderer->getControlWidget()->getBarPlotWidget(),
			SLOT(setBarValue(int,double)));

	disconnect(m_traitRenderer->getControlWidget()->getTraitSelector(),
			SIGNAL(computeWarpField(QString,QString)),
			this,
			SLOT(computeTraitWarpfieldFileName(QString,QString)));

	disconnect(m_traitRenderer->getControlWidget()->getTraitSelector(),
			SIGNAL(loadNewTrait(int)),this,SLOT(setNewTrait(int)));

	disconnect(m_traitRenderer->getControlWidget()->getTraitSelector(),
			SIGNAL(setTraitScale(int, double)),this,SLOT(setTraitScale(int, double)));

	disconnect(m_traitRenderer->getControlWidget()->getTraitSelector(),
			SIGNAL(unloadWarpfield()),this,SLOT(unloadWarpfield()));

	disconnect(m_traitRenderer->getControlWidget()->getTraitSelector(),
			SIGNAL(updateTraitList(QList<Trait>,int)),this,SLOT(updateTraitList(QList<Trait>,int)));


	disconnect(m_batchDialog,SIGNAL(finished()),
		m_traitRenderer->getControlWidget()->getTraitSelector(),
		SLOT(warpfieldGenerated()));


	////////////////////////////////////////////////////////////////
	//  Connect
	////////////////////////////////////////////////////////////////

	// connections for getPlotWidget()	
	connect(m_volumeRenderer->getControlWidget()->getPlotWidget(), // plotter
			SIGNAL(sig_drawTraits(bool)),
			m_scatterPlotWidget,
			SLOT(drawTraitLine(bool)));

	connect(m_volumeRenderer->getControlWidget()->getPlotWidget(), // plotter
			SIGNAL(sig_drawSigma(bool)),
			m_scatterPlotWidget,
			SLOT(drawSigma(bool)));

	connect(m_volumeRenderer->getControlWidget()->getPlotWidget(), // plotter
			SIGNAL(changeItem(int,int,int,double,double)),
			m_volumeRenderer->getControlWidget()->getBarPlotWidget(), // bar plot widget
			SLOT(changeItem(int,int,int,double,double)));

	connect(m_volumeRenderer->getControlWidget()->getPlotWidget(),
			SIGNAL(traitChanged(int)),this,SLOT(loadNewTrait(int)));
		
	connect(m_volumeRenderer->getControlWidget()->getPlotWidget(),
			SIGNAL(changeItem(int,double)),
			m_volumeRenderer->getControlWidget()->getBarPlotWidget(),
			SLOT(changeItem(int,double)));

	connect(m_volumeRenderer->getControlWidget()->getBarPlotWidget(),
			SIGNAL(valueChanged(int,double)),
			m_volumeRenderer->getControlWidget()->getPlotWidget(),
			SLOT(updateMovingPointPosition(int,double)));	


	// connections for getTraitSelector()
#ifdef SDMVIS_VARVIS_ENABLED
	connect(m_traitRenderer->getControlWidget()->getTraitSelector(),
			SIGNAL(clearVarVisBox()),
			this,
			SLOT(clearVarVisBox()));

	connect(m_glyphControls,
			SIGNAL(setwarpId(int)),
			this,
			SLOT(setNewTrait(int)));

	m_glyphControls->enableTraitsFromSDMVIS(true);
	connect(m_glyphControls,
			SIGNAL(SIG_getTraitsfromSDMVIS(bool)),
			m_traitRenderer->getControlWidget()->getTraitSelector(),
			SLOT(pushTraitToVarVis(bool)));

	connect(m_roiControls,SIGNAL(calculateROI()),this,SLOT(refineROI()));
	connect(m_roiControls,SIGNAL(statusMessage(QString)),this,SLOT(statusMessage(QString)));

#endif

	connect(m_traitRenderer->getControlWidget()->getTraitSelector(),
			SIGNAL(newTraitInTown(Trait,int)),this, SLOT(addTrait(Trait,int)));
	
	connect(m_traitRenderer->getControlWidget()->getTraitSelector(),
			SIGNAL(loadWarpfield(QString)),this, SLOT(loadWarpfield(QString)));	

	connect(m_traitRenderer->getControlWidget()->getBarPlotWidget(),
			SIGNAL(valueChanged(int,double)),
			m_traitRenderer->getControlWidget()->getTraitSelector(),
			SLOT(setBarValue(int,double)));

	connect(m_traitRenderer->getControlWidget()->getTraitSelector(),
			SIGNAL(loadBarValue(int,double)),
			m_traitRenderer->getControlWidget()->getBarPlotWidget(),
			SLOT(setBarValue(int,double)));

	connect(m_traitRenderer->getControlWidget()->getTraitSelector(),
			SIGNAL(computeWarpField(QString,QString)),
			this,
			SLOT(computeTraitWarpfieldFileName(QString,QString)));

	connect(m_traitRenderer->getControlWidget()->getTraitSelector(),
			SIGNAL(loadNewTrait(int)),this,SLOT(setNewTrait(int)));

	connect(m_traitRenderer->getControlWidget()->getTraitSelector(),
			SIGNAL(setTraitScale(int, double)),this,SLOT(setTraitScale(int, double)));

	connect(m_traitRenderer->getControlWidget()->getTraitSelector(),
			SIGNAL(unloadWarpfield()),this,SLOT(unloadWarpfield()));

	connect(m_traitRenderer->getControlWidget()->getTraitSelector(),
			SIGNAL(updateTraitList(QList<Trait>,int)),this,SLOT(updateTraitList(QList<Trait>,int)));


	connect(m_batchDialog,SIGNAL(finished()),
		m_traitRenderer->getControlWidget()->getTraitSelector(),
		SLOT(warpfieldGenerated()));
}

void SDMVisMainWindow::updateTraitList(QList<Trait> updatedList, int pos)
{
	m_tempTraitList=updatedList;
	int debug =m_tempTraitList.size();
	if (m_tempTraitList.size()>0){
		//set the warpfield to new warpfield
		QList<Warpfield> tempList;
			Warpfield temp;
			temp.elementScale=m_tempTraitList.at(pos).elementScale;
			temp.mhdFilename=m_baseDir+"/"+m_tempTraitList.at(pos).mhdFilename;
			tempList.push_back(temp);

		m_traitRenderer->setWarpfields(tempList);
		//update traitList in other widgets
		m_traitRenderer->getControlWidget()->getTraitSelector()->setTraitList(m_tempTraitList);
		
		m_traitRenderer->getControlWidget()->getTraitSelector()->setSpecificTrait(pos);
		//m_traitRenderer->getControlWidget()->getTraitSelector()->setIndex(pos);

		Trait specificTrait=m_tempTraitList.at(pos);
		specificTrait.computeNormal();
		m_volumeRenderer->getControlWidget()->getPlotWidget()->setTrait(specificTrait);
		m_volumeRenderer->getControlWidget()->getPlotWidget()->setTraitList(m_tempTraitList);
		m_traitRenderer->getControlWidget()->getPlotWidget()->setIndex(pos);
		loadNewTrait(pos);
	}	
	else
	{
		m_volumeRenderer->getControlWidget()->getPlotWidget()->reset();
		m_scatterPlotWidget->setEmptyClassVector();		
	}

	m_traitRenderer->getControlWidget()->getTraitSelector()->sanityCheck();
}


#ifdef SDMVIS_VARVIS_ENABLED
// ROI functionality now totally depends on VarVis module

//------------------------------------------------------------------------------
//	ROI computation
//------------------------------------------------------------------------------

void SDMVisMainWindow::refineROI()
{
	// dataset size is given implicitly by names array
	// get config options (config directory, warps matrix, output suffix)

	QString configPath = m_baseDir; // baseDir set trought load config 

	QFileInfo info(m_loadedConfigName);
	
	configPath.append("/"+info.baseName()+"/Roi"); // path where to save data

	QString savingPath= m_baseDir;
	savingPath.append("/"+info.baseName()+"/Roi-"+m_roiControls->getRoiName()+".ini");

	if (!QDir(configPath).exists())
		QDir().mkdir(configPath);

	if( configPath.isEmpty() )
		return;

	QString warpsFilename = m_config.getWarpsMatrix();
	if( warpsFilename.isEmpty() )
		return;

	int numEigenwarpsToExtract = m_config.getEigenwarps().size();

	const VolumeDataHeader* vol = m_volumeRenderer->getVolumeData();
	if( !vol )
	{
		QMessageBox::warning( this, tr("sdmvis: Warning"),
			tr("No valid reference volume could be found?!!") );
		return;
	}

	// save ROI   
	QString RoiFileName = m_roiControls->getFullRoiName();

	QFileInfo suffixNameInfo(RoiFileName);
	QString outSuffix = suffixNameInfo.baseName();	
	
	int numDatasets = m_config.getNames().size();
	RefineROIParameters parms( configPath, warpsFilename, outSuffix, 
								numDatasets, numDatasets, vol );
	parms.warpsFilename = warpsFilename;
	
	parms.ROIBasename=RoiFileName.remove(".mhd");



	// environment
	QString path = m_batchVoltoolsPath;

	// build batch
	
	typedef BatchProcessingDialog::BatchCommand Command;
	typedef BatchProcessingDialog::CommandList  CommandList;

	Command 
		cmdApplyROI( path+"mat_op",	QStringList()
			 << parms.sN << parms.sM 
			 << parms.warpsFilename
			 << parms.ROIBasename + QString(".raw")
			 << parms.outWarps,
			tr("Apply selection to warps datamatrix") ),
		
		cmdComputeScatterMat( path+"scattermat", QStringList()
			 << "-t" << parms.sN << parms.sM
			 << parms.outWarps
			 << parms.outScatter,
			tr("Compute scatter matrix") ),

		cmdPCA( path+"yapca", QStringList()
			 << "pcacov" << parms.sM << parms.sK
			 << parms.outScatter
			 << parms.outPCAEigenvectors
			 << parms.outPCACoefficients
			 << parms.outPCAEigenvalues,
			tr("PCA analysis") ),

		cmdPCA_csv( path+"yapca", QStringList()
			 << "pcacov" << parms.sM << parms.sK
			 << parms.outScatter
			 << (parms.outPCAEigenvectors + ".csv")
			 << (parms.outPCACoefficients + ".csv")
			 << (parms.outPCAEigenvalues  + ".csv"),
			tr("PCA analysis (.csv output)") ),

		cmdReconEigenwarpsMat( path+"matmult", QStringList()
			 << parms.sN << parms.sM << parms.sK
			 << parms.outWarps		 // *weighted* dataset
			 << parms.outPCAEigenvectors
			 << parms.outEigenwarps,
			tr("Reconstruct eigenwarp matrix (local)") ),
		
		cmdReconEigenwarpsMat_global( path+"matmult", QStringList()
			 << parms.sN << parms.sM << parms.sK
			 << parms.warpsFilename   // *original* unweighted dataset
			 << parms.outPCAEigenvectors
			 << parms.outEigenwarps_global,
			tr("Reconstruct eigenwarp matrix (global)") ),

		cmdNorm( path+"matnorm", QStringList()
			 << parms.sN << parms.sK
			 << parms.outEigenwarps,
			tr("Column norms (local eigenwarps)") ),

		cmdNorm_global( path+"matnorm", QStringList()
			 << parms.sN << parms.sK
			 << parms.outEigenwarps_global,
			tr("Column norms (global eigenwarps)") );

	CommandList batch;
	batch.push_back( cmdApplyROI );

	batch.push_back( cmdComputeScatterMat );
	batch.push_back( cmdPCA );
	batch.push_back( cmdPCA_csv );
	batch.push_back( cmdReconEigenwarpsMat );
	batch.push_back( cmdReconEigenwarpsMat_global );

	QStringList eigenWarpList;
	QStringList eigenWarpListLocal;

	// set spacing according to warpfields
	QString sSpacingX = QString::number(m_volumeRenderer->getVolumeData()->spacingX()),
		    sSpacingY = QString::number(m_volumeRenderer->getVolumeData()->spacingY()),
		    sSpacingZ = QString::number(m_volumeRenderer->getVolumeData()->spacingZ());

	for( int i=0; i < numEigenwarpsToExtract; ++i )
	{
		QString si = QString::number(i),
				sResX = QString::number(parms.resX),
				sResY = QString::number(parms.resY),
				sResZ = QString::number(parms.resZ),
		        outEigenwarp = configPath+"/" + "eigenwarp"+si+"_"+parms.sOutputSuffix+".raw",
				outEigenwarp_global = configPath+"/" + "global_eigenwarp"+si+"_"+parms.sOutputSuffix+".raw";
		batch.push_back( 
			Command( path+"matcol2raw", QStringList()
				 << parms.sN << parms.sK << parms.outEigenwarps << si 
				 << outEigenwarp
				 << "mhd" << sResX << sResY << sResZ << "3" << sSpacingX << sSpacingY << sSpacingZ,
				tr("Convert local eigenwarp %1 to MHD Volume").arg(i+1) )
			);
		batch.push_back( 
			Command( path+"matcol2raw", QStringList()
				 << parms.sN << parms.sK << parms.outEigenwarps_global << si 
				 << outEigenwarp_global
				 << "mhd" << sResX << sResY << sResZ << "3" << sSpacingX << sSpacingY << sSpacingZ,
				tr("Convert global eigenwarp %1 to MHD Volume").arg(i+1) )
			);
		eigenWarpList.push_back(configPath+"/" + "global_eigenwarp"+si+"_"+parms.sOutputSuffix+".mhd");
		eigenWarpListLocal.push_back(configPath+"/" + "eigenwarp"+si+"_"+parms.sOutputSuffix+".mhd");
	}
	batch.push_back( cmdNorm );
	batch.push_back( cmdNorm_global );
	
	
	// start batch processing
	
	m_batchproc->reset();
	m_batchproc->initBatch( batch );
	//m_batchproc->setLogFile( configPath + "/" + "log_"+parms.sOutputSuffix+".txt" );  --> TODO: Log file functionality
	m_batchproc->process()->setWorkingDirectory( m_baseDir );
	m_batchproc->show();
	//m_batchproc->start();  // --> started manually in dialog	

	connect( m_batchproc, SIGNAL(finished()), this, SLOT(batchROIfinished()) );
	
#ifdef SDMVIS_VARVIS_ENABLED
	// ROIs are specified via VarVis
	QVector<ROI> rois;
	for( int i=0; i < m_roiRender->getROI().size(); ++i )
	{
		ROI roi;
		double center[3];

		roi.radius = m_roiRender->getROI().at(i)->GetRadius();
		m_roiRender->getROI().at(i)->GetCenter( center );
		roi.center[0]=center[0];
		roi.center[1]=center[1];
		roi.center[2]=center[2];
		
		rois.push_back( roi );
	}
	parms.roiVector = rois;
#endif

	// set global ROI parms (for generating ROI config in batchROIfinished())
	parms.reference = m_config.getReference(); // use the same reference as current config
	parms.savingPath = savingPath; // for loading the global ROI version	
	parms.eigenWarpList = eigenWarpList;
	parms.eigenWarpListLocal = eigenWarpListLocal;
	m_roiParms = parms;
}

void SDMVisMainWindow::saveConfigROI( RefineROIParameters parms, bool local, QString reference )
{
	m_config.clearTraits();
	SDMVisConfig config = m_config; // new config is based on current one
	ConfigGenerator *cfgGen= new ConfigGenerator();
	cfgGen->setConfigPath( m_baseDir );
	cfgGen->setConfig( &config );
	config.clearTraits();
	config.clearRoi();

#ifdef SDMVIS_VARVIS_ENABLED
	config.setSamplePointSize(m_varvisRender->getSamplePointSize());		
	config.setMeshFilename("");
	config.setPointsFilename("");
#endif

	// defaults for global warps
	QString savingPath = parms.savingPath;
	QString outEigenwarps = parms.outEigenwarps_global;	
	QStringList eigenWarpList = parms.eigenWarpList;
	if( local )
	{		
		savingPath.remove(".ini");
		savingPath.append("_LOCAL.ini");
		
		outEigenwarps = parms.outEigenwarps;
		eigenWarpList = parms.eigenWarpListLocal;
	}		
	
	if( reference.isEmpty() )
	{
		reference = parms.reference;
	}
	
	cfgGen->auto_generateConfig( savingPath, reference,
		parms.outWarps, outEigenwarps, parms.outScatter,
		parms.outPCAEigenvectors, parms.outPCAEigenvalues, 
		eigenWarpList );

	config.setRoiVector( parms.roiVector );
	config.writeConfig( savingPath );	
}	

void SDMVisMainWindow::batchROIfinished()
{


	disconnect( m_batchproc, SIGNAL(finished()), this, SLOT(batchROIfinished()) );
	if( m_batchproc->hasCrashed() )
		return;

	// computeNormalization( m_roiParms );
	//--> FIXME: computeNormalization() crashs!

	RefineROIParameters parms = m_roiParms; // use ROI parms from refineROI()

	// save global config
	saveConfigROI( parms );

	// save local config (i.e. with masked warpfields )
	QString reference=parms.reference;	
  #ifdef SDMVIS_VARVIS_ENABLED
	if( m_roiRender->getUseLocalReference() )
		reference=m_roiRender->getLocalReference();
  #endif
	saveConfigROI( parms, true, reference );
	
	// load ROI config
	loadConfig( parms.savingPath );
}

#endif // SDMVIS_VARVIS_ENABLED

void SDMVisMainWindow::unloadWarpfield()
{
	QList<Warpfield> emptyWarp;
	emptyWarp.clear();
    this->m_traitRenderer->setWarpfields(emptyWarp);
	this->m_traitRenderer->getControlWidget()->getBarPlotWidget()->unload();
	this->m_traitRenderer->getControlWidget()->getBarPlotWidget()->setDisabled(true);
}

void SDMVisMainWindow::addTrait(Trait newTrait, int pos)
{
	if (!newTrait.fullPathFileName.isEmpty())
		newTrait.mhdFilename = m_config.getRelativePath(newTrait.fullPathFileName, m_baseDir);
	this->m_traitRenderer->getControlWidget()->getBarPlotWidget()->setDisabled(false);
	int sizeOfTraitList=m_tempTraitList.size();
	
	if (sizeOfTraitList==0)
		m_tempTraitList.push_back(newTrait);

	if (sizeOfTraitList==pos && sizeOfTraitList!=0)
		m_tempTraitList.append(newTrait);
	else // we have a modification of an existing trait
	{
		m_tempTraitList.removeAt(pos);
		m_tempTraitList.insert(pos,newTrait);
	
	}
	//set the warpfield to new warpfield
	// try to open the warpfield file
	if (m_tempTraitList.at(pos).mhdFilename!="empty" )
	{
		QList<Warpfield> tempList;
			Warpfield temp;
			temp.elementScale=m_tempTraitList.at(pos).elementScale;
			temp.mhdFilename=m_baseDir+"/"+m_tempTraitList.at(pos).mhdFilename;
			tempList.push_back(temp);

		this->m_traitRenderer->setWarpfields(tempList);
		this->m_traitRenderer->getControlWidget()->getBarPlotWidget()->setDisabled(false);
	}
	else
	{
		this->m_traitRenderer->getControlWidget()->getBarPlotWidget()->setDisabled(true);
	}
	//update traitList in other widgets
	this->m_traitRenderer->getControlWidget()->getTraitSelector()->setTraitList(m_tempTraitList);
	
	this->m_traitRenderer->getControlWidget()->getTraitSelector()->setSpecificTrait(pos);
	//this->m_traitRenderer->getControlWidget()->getTraitSelector()->setIndex(pos);

	Trait specificTrait=m_tempTraitList.at(pos);
	specificTrait.computeNormal();
	this->m_volumeRenderer->getControlWidget()->getPlotWidget()->setTrait(specificTrait);
	this->m_volumeRenderer->getControlWidget()->getPlotWidget()->setTraitList(m_tempTraitList);
	this->m_traitRenderer->getControlWidget()->getPlotWidget()->setIndex(pos);
	loadNewTrait(pos);
#ifdef SDMVIS_VARVIS_ENABLED
	// allow varvis to get Traits from TraitRender
	m_glyphControls->enableTraitsFromSDMVIS(true);
#endif
}

void SDMVisMainWindow::loadNewTrait(int index)
{
	if (m_tempTraitList.at(index).mhdFilename!="empty")
	{
		QList<Warpfield>temp_field;
			Warpfield temp;
			temp.mhdFilename=m_config.getAbsolutePath(m_tempTraitList.at(index).mhdFilename);
				temp.elementScale=m_tempTraitList.at(index).elementScale;
				temp_field.push_back(temp);

		m_traitRenderer->setWarpfields(temp_field);
		m_traitRenderer->getControlWidget()->getBarPlotWidget()->setDisabled(false);
	}
	else
		m_traitRenderer->getControlWidget()->getBarPlotWidget()->setDisabled(true);
	
	m_traitRenderer->getControlWidget()->getTraitSelector()->setSpecificTrait(index);
	m_traitRenderer->getControlWidget()->getTraitSelector()->sanityCheck();
	Trait specificTrait=m_tempTraitList.at(index);
	specificTrait.computeNormal();

	m_traitRenderer->setTitle( specificTrait.identifier );

	m_scatterPlotWidget->setTrait(specificTrait);
	m_volumeRenderer->getControlWidget()->getPlotWidget()->setTrait(specificTrait);
	m_volumeRenderer->getControlWidget()->getPlotWidget()->setIndex(index);
	
}

void SDMVisMainWindow::setNewTrait(int index)
{
	if (m_tempTraitList.at(index).mhdFilename!="empty")
	{
		QList<Warpfield>temp_field;
			Warpfield temp;
			temp.mhdFilename=m_config.getAbsolutePath(m_tempTraitList.at(index).mhdFilename);
				temp.elementScale=m_tempTraitList.at(index).elementScale;
				temp_field.push_back(temp);

		this->m_traitRenderer->setWarpfields(temp_field);
		this->m_traitRenderer->getControlWidget()->getBarPlotWidget()->setDisabled(false);
	}
	else
		this->m_traitRenderer->getControlWidget()->getBarPlotWidget()->setDisabled(true);
	this->m_traitRenderer->getControlWidget()->getTraitSelector()->setSpecificTrait(index);
	this->m_traitRenderer->getControlWidget()->getTraitSelector()->sanityCheck();
	Trait specificTrait=m_tempTraitList.at(index);
	specificTrait.computeNormal();

	this->m_scatterPlotWidget->setTrait(specificTrait);
	this->m_volumeRenderer->getControlWidget()->getPlotWidget()->setTrait(specificTrait);
	this->m_volumeRenderer->getControlWidget()->getPlotWidget()->setIndex(index);
#ifdef SDMVIS_VARVIS_ENABLED
	if (m_traitRenderer->getControlWidget()->getTraitSelector()->getPushToVarVis())
	{
		//m_varvisControls->addTraitSelection(m_traitRenderer->getControlWidget()->getTraitSelector()->getSelectionBox());
		m_glyphControls->addTraitSelection(m_traitRenderer->getControlWidget()->getTraitSelector()->getSelectionBox());
		if (m_tempTraitList.at(index).mhdFilename!="empty")
		{	
			QString warpfieldName=m_config.getAbsolutePath(m_tempTraitList.at(index).mhdFilename);
			m_varvisRender->setWarp(warpfieldName,false);			
		}		
	}
#endif
}

void SDMVisMainWindow::saveConfigAs( QString filename )
{
	m_config.clearTraits();
	m_config.setTraitList(m_tempTraitList);
	
	QFileInfo fileinfo( filename );
	QString basePath=fileinfo.absolutePath();

#ifdef SDMVIS_VARVIS_ENABLED
	// ROIs are specified via VarVis
	m_config.clearRoi();
	for (int iA=0;iA<m_roiRender->getROI().size();iA++)
	{
		ROI tempROI;
		tempROI.radius=m_roiRender->getROI().at(iA)->GetRadius();
		double center[3];
		m_roiRender->getROI().at(iA)->GetCenter(center);
		tempROI.center[0]=center[0];
		tempROI.center[1]=center[1];
		tempROI.center[2]=center[2];
		m_config.addRoi(tempROI);	
	}
	QString meshName=filename;
	meshName.remove(".ini");
	meshName.append("_MESH.msh");
	double isoVlaue = m_varvisRender->saveMesh(meshName);
	double radiusOfPoints=m_varvisRender->getSamplePointSize();
	if (m_varvisRender->getSamplePresent())
	{
		// when sample point data is present save it [mesh data always present!]
		QString pointsName=filename;
		pointsName.remove(".ini");
		pointsName.append("_POINTS.pts");
		int numberOfSamplingPoints= m_varvisRender->savePointSamples(pointsName);
		m_config.setPointsFilename(m_config.getRelativePath(pointsName));	
		m_config.setNumberOfSamplePoints(numberOfSamplingPoints);
		m_config.setSamplePointSize(m_varvisRender->getSamplePointSize());
	}
	m_config.setIsoValue(isoVlaue);
	m_config.setMeshFilename(m_config.getRelativePath(meshName));
#endif // SDMVIS_VARVIS_ENABLED

#ifdef SDMVIS_TENSORVIS_ENABLED
	// Tensor probes are stored via SDMTensorVisWidget
	m_config.setTensorProbes( m_tensorVis->getProbes() );
#endif // SDMVIS_TENSORVIS_ENABLED
	
	// write config to .ini file	
	m_config.setBasePath( fileinfo.absolutePath() );
	m_config.writeConfig( filename );

	// extract absolute path and filename
	QFileInfo info( filename );
	m_baseDir = info.absolutePath(); // name is info.fileName()	

	// set active config filename
	m_loadedConfigName = filename;

	statusMessage( 
		tr("Successfully saved config %1").arg(info.fileName()) );
	setWindowTitle( APP_NAME + " - " + info.fileName() );
}

void SDMVisMainWindow::saveConfig()
{
	if( !m_loadedConfigName.isEmpty() )
	{
		m_tempTraitList.clear();
		this->m_traitRenderer->getControlWidget()->getTraitSelector()->sanityCheck();
		m_tempTraitList=this->m_traitRenderer->getControlWidget()->getTraitSelector()->getTraitList();	
		saveConfigAs( m_loadedConfigName );
	}
}

void SDMVisMainWindow::saveConfigAs()
{
	if( m_loadedConfigName.isEmpty() )
		return;

	m_tempTraitList.clear();
	this->m_traitRenderer->getControlWidget()->getTraitSelector()->sanityCheck();
	m_tempTraitList=this->m_traitRenderer->getControlWidget()->getTraitSelector()->getTraitList();
	QString filename;
	filename = QFileDialog::getSaveFileName( this,
		tr("Save sdmvis config file"),
		m_baseDir, tr("SDMVis configuration file (*.ini)") );

	if( filename.isEmpty() )
		return;

	saveConfigAs( filename );	
}


//------------------------------------------------------------------------------
//	Trait computation
//------------------------------------------------------------------------------

/// execute batch processing to reconstruct trait warpfield from trait vector
void SDMVisMainWindow::computeTraitWarpfield_internal( ReconTraitParameters parms )
{
	// --- build batch -------------------------------------------------------
	
	QString path = m_batchVoltoolsPath;

	QString sN = QString::number(parms.N),
		    sM = QString::number(parms.M),
		outBasename = parms.configPath+"/"+ parms.sOutputSuffix,// changed
		outWarp    = outBasename +".mat",
		outWarpRaw = outBasename + ".raw",
			sResX = QString::number(parms.resX),
			sResY = QString::number(parms.resY),
			sResZ = QString::number(parms.resZ),
			sSX = QString::number(parms.spacingX),
			sSY = QString::number(parms.spacingY),
			sSZ = QString::number(parms.spacingZ);

	typedef BatchProcessingDialog::BatchCommand Command;
	typedef BatchProcessingDialog::CommandList  CommandList;

	Command 
		cmdReconTraitWarp( path+"matmult",	QStringList()
			 << sN << sM 
			 << QString::number( 1 )
			 << parms.warpsFilename
			 << parms.traitFilename
			 << outWarp,
			tr("Reconstruct warp corresponding to trait vector") ),

		cmdReconTraitWarpCUDA( path+"cuda_matmul",	QStringList()
			 << sN << sM 
			 << QString::number( 1 )
			 << parms.warpsFilename
			 << parms.traitFilename
			 << outWarp,
			tr("Reconstruct warp corresponding to trait vector") ),

		cmdMHD( path+"matcol2raw", QStringList()
			 << sN << QString::number(1) << outWarp << QString::number( 0 ) 
			 << outWarpRaw
			 << "mhd" << sResX << sResY << sResZ << "3" << sSX << sSY << sSZ,
			tr("Convert trait warp to MHD Volume") ),

		cmdNorm( path+"matnorm", QStringList()
			 << sN << QString::number( 1 )
			 << outWarp,
			tr("Column norms") );

#if 1
	// disable CUDA by default (let the user decide CUDA is applicable for now)
	cmdReconTraitWarpCUDA.run = false;
#else
	// enable CUDA for video and demos
	cmdReconTraitWarpCUDA.run = true;
	cmdReconTraitWarp.run = false;
#endif
	// disable norm computation by default 
	// (output goes only to console; irrelevant for sdmvis)
	cmdNorm.run = false;

	CommandList batch;
	batch.push_back( cmdReconTraitWarp );
	batch.push_back( cmdReconTraitWarpCUDA );
	batch.push_back( cmdMHD );
	batch.push_back( cmdNorm );
	
	// --- start batch processing --------------------------------------------

	m_batchDialog->reset();
	m_batchDialog->initBatch( batch );
	//m_batchproc->setLogFile( configPath + "/" + "log_"+parms.sOutputSuffix+".txt" );  
		// --> TODO: Log file functionality
	m_batchDialog->process()->setWorkingDirectory( m_baseDir );
	m_batchDialog->show();

	connect(m_traitRenderer->getControlWidget()->getTraitSelector(),
			SIGNAL(computeWarpField(QString,QString)),
			this,
			SLOT(computeTraitWarpfieldFileName(QString,QString)));

}

#ifdef SDMVIS_MANUAL_ANALYSIS
void SDMVisMainWindow::computeTraitWarpfield()
{
	// --- setup variables ---------------------------------------------------

	// needed stuff for trait computation

	// volume res./spacing (taken from first config's eigenwarp or reference)
	VolumeDataHeader* vol   = NULL;
	// dataset size is given implicitly by names array
	QStringList       names = m_config.getNames();
	// warps matrix
	QString   warpsFilename = m_config.getWarpsMatrix();
	
	if( names.empty() )
	{
		QMessageBox::warning( this, tr("sdmvis: Warning"),
			tr("The dataset names must be specified in the config (e.g. via File/Open Names)!") );
		return;
	}
	if(m_expertMode)
	{
		if( !m_config.getEigenwarpsMatrix().isEmpty() )
		{
			if( QMessageBox::question( this, tr("sdmvis: Compute trait warpfield"),
					tr("Use eigenwarps matrix instead of waprs matrix for reconstruction?"),
					QMessageBox::Yes | QMessageBox::No,  QMessageBox::Yes )
				== QMessageBox::Yes )
			{
				warpsFilename = m_config.getEigenwarpsMatrix();
			}
		}
	}
	else
	{
		warpsFilename = m_config.getEigenwarpsMatrix();
	}

	if( warpsFilename.isEmpty() )
	{
		QMessageBox::warning( this, tr("sdmvis: Warning"),
			tr("The warpfields matrix must be specified in the config (e.g. via File/Set WarpsMatrix)!") );
		return;
	}

	// mhd of eigenwarp (reference as fallback) for volume resolution/spacing
	QString volumeHeaderFilename;
	if( m_config.getEigenwarps().empty() )
	{
		if( m_config.getReference().isEmpty() )
		{
			QMessageBox::warning( this, tr("sdmvis: Warning"),
				tr("Eigenwarps or at least reference volume must be specified in the config!") );
			return;
		}
		// use reference volume info
		volumeHeaderFilename = m_config.getReference();
	}
	else
	{	
		// use first warpfield volume info
		volumeHeaderFilename = m_config.getEigenwarps().at(0).mhdFilename;
	}

	// load volume header
	vol = load_volume_header( volumeHeaderFilename.toAscii(), 3 );
	if( !vol )
	{
		QMessageBox::warning( this, tr("sdmvis: Error"),
			tr("Unexpected error loading volume %1!").arg(volumeHeaderFilename) );
		return;
	}


	// --- get options -------------------------------------------------------

	// TODO: config path relative to config base path (or even identical ?)
	QString configPath = QFileDialog::getExistingDirectory( this, 
		tr("Choose config path..."), m_baseDir );
	

	if( configPath.isEmpty() )
		return;

	QString traitFilename = QFileDialog::getOpenFileName( this,
		tr("Select trait vector (in V-space, computed via Trait Dialog)..."), 
		configPath, tr("Raw matrix file (*.mat)") );
	if( traitFilename.isEmpty() )
		return;

	bool ok;
	QString outSuffix = QInputDialog::getText( this,
		tr("Config options"), tr("Output suffix (important if config path not empty)"),
		QLineEdit::Normal, tr(""), &ok );
	if( !ok )
		return;
			
	// setup recon parameters
	ReconTraitParameters parms( outSuffix, names.size(), vol );
	parms.configPath    = configPath;
	parms.traitFilename = traitFilename;
	parms.warpsFilename = warpsFilename;

	// vol no longer needed
	delete vol; vol=NULL;

	computeTraitWarpfield_internal( parms );
}
#endif // SDMVIS_MANUAL_ANALYSIS

void SDMVisMainWindow::computeTraitWarpfieldFileName(QString mhdFilename,QString matFilename)
{
		disconnect(m_traitRenderer->getControlWidget()->getTraitSelector(),
			SIGNAL(computeWarpField(QString,QString)),
			this,
			SLOT(computeTraitWarpfieldFileName(QString,QString)));

	std::cout << "Reconstructing trait " << mhdFilename.toStdString() << ", " << matFilename.toStdString() << std::endl;

	// --- setup variables ---------------------------------------------------

	// volume res./spacing (taken from first config's eigenwarp or reference)
	VolumeDataHeader* vol   = NULL;
	// dataset size is given implicitly by names array
	QStringList       names = m_config.getNames();
	// warps matrix
	QString   warpsFilename = m_config.getWarpsMatrix();
	
	if( names.empty() )
	{
		QMessageBox::warning( this, tr("sdmvis: Warning"),
			tr("The dataset names must be specified in the config (e.g. via File/Open Names)!") );
		return;
	}
	
	if(m_expertMode)
	{
		if( !m_config.getEigenwarpsMatrix().isEmpty() )
		{
			if( QMessageBox::question( this, tr("sdmvis: Compute trait warpfield"),
					tr("Use eigenwarps matrix instead of waprs matrix for reconstruction?"),
					QMessageBox::Yes | QMessageBox::No,  QMessageBox::Yes )
				== QMessageBox::Yes )
			{
				warpsFilename = m_config.getEigenwarpsMatrix();
			}
		}
	}
	else
	{
		// default is not to use eigenwarps matrix!
	}

	if( warpsFilename.isEmpty() )
	{
		QMessageBox::warning( this, tr("sdmvis: Warning"),
			tr("The warpfields matrix must be specified in the config (e.g. via File/Set WarpsMatrix)!") );
		return;
	}

	// mhd of eigenwarp (reference as fallback) for volume resolution/spacing
	QString volumeHeaderFilename;
	if( m_config.getEigenwarps().empty() )
	{
		if( m_config.getReference().isEmpty() )
		{
			QMessageBox::warning( this, tr("sdmvis: Warning"),
				tr("Eigenwarps or at least reference volume must be specified in the config!") );
			return;
		}
		// use reference volume info
		volumeHeaderFilename = m_config.getReference();
	}
	else
	{	
		// use first warpfield volume info
		volumeHeaderFilename = m_config.getEigenwarps().at(0).mhdFilename;
	}

	// load volume header
	vol = load_volume_header( volumeHeaderFilename.toAscii(), 3 );
	if( !vol )
	{
		QMessageBox::warning( this, tr("sdmvis: Error"),
			tr("Unexpected error loading volume %1!").arg(volumeHeaderFilename) );
		return;
	}


	// --- get options -------------------------------------------------------

	if( matFilename.isEmpty() )
		return;

	QFileInfo info(mhdFilename);
	
	QString configPath=mhdFilename;
	configPath.remove(info.fileName());
	QString traitFilename=matFilename;
	
	QString outSuffix = info.fileName().remove(".mhd");		
	
	// setup recon parameters
	ReconTraitParameters parms( outSuffix, names.size(), vol );
	parms.configPath    = configPath;
	parms.traitFilename = traitFilename;
	parms.warpsFilename = warpsFilename;

	// vol no longer needed
	delete vol; vol=NULL;

	computeTraitWarpfield_internal( parms );	
}

void SDMVisMainWindow::showTraitDialog()
{	
	m_traitDialog->setNames( m_config.getNames() );
	m_traitDialog->setBaseDir( m_baseDir ); // TODO: or use m_config.getBasePath() ?
	if( m_config.getNames().empty() )
	{
		QMessageBox::warning( this, tr("sdmvis: Warning"),
			tr("The dataset names must be specified in the config (e.g. via File/Open Names)") );
		return;
	}
	else
		m_traitDialog->show();
}

void SDMVisMainWindow::showScatterPlot()
{	
	m_scatterPlotWidget->show();
	m_scatterPlotWidget->deselect();
}

void SDMVisMainWindow::showEigenvalues()
{
	QString msg;
	Vector lambda = m_config.getPCAEigenvalues();

	double sum = 0.0;
	for(unsigned int i=0; i < lambda.size(); ++i )
		sum += lambda[i];

#if 1 // barplot
	BarPlotWidget::Vector percent;
	QStringList labels;
	for(unsigned  int i=0; i < lambda.size(); ++i ) {		
		double val =  lambda[i] / sum;
		percent.push_back( val );
		labels << tr("PC %1").arg(i+1);

		std::cout << "PC " << i << ": " << (lambda[i]/sum) << "% (" << lambda[i] << ")" << std::endl;
	}

	BarPlotWidget* bp = new BarPlotWidget();
	bp->setLabels( labels );
	bp->setOnlyPositiveValues( true );
	bp->setValuesChangeable( false );
	bp->setValues( percent );
	bp->setFrameStyle( QFrame::StyledPanel );
	bp->setEnabled( false );

	QLabel* title = new QLabel(tr("Percentage of explained variance:"));

	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget( title, 0 );
	layout->addWidget( bp, 5 );

	QWidget* w = new QWidget();
	w->setWindowTitle( tr("%1 - PCA model eigenvalues").arg(APP_NAME) );
	w->setWindowIcon( APP_ICON );
	w->setLayout( layout );
	w->show();
	
#else // simple message box
	for(unsigned  int i=0; i < lambda.size(); ++i )
	{
		msg = msg + QString::number(i) + ": " 
			+ QString::number( lambda[i] ) 
			+ "  (" + QString::number( lambda[i] / sum ) + ") \n";
	}
	QMessageBox::information( this, tr("sdmvis - Eigenvalues"), msg );
#endif
}
