#ifndef SDMTENSORVISWIDGET_H
#define SDMTENSORVISWIDGET_H

#include "QTensorVisWidget.h"
#include "e7/VolumeRendering/RayPickingInfo.h"
#include "EditLocalCovariance.h"
#include "LinearLocalCovariance.h"
#include "StatisticalDeformationModel.h"
#include "TensorDataStatistics.h"
#include "TensorDataAdaptor.h"
#include "TensorVis2.h"
#include "SDMTensorProbe.h"

#include "VTKVisPrimitives.h" // SphereMarker

#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkInteractorStyleTrackballActor.h>
#ifndef VTKPTR
#include <vtkSmartPointer.h>
#define VTKPTR vtkSmartPointer
#endif

#include <QList>

class QWidget;
class QDoubleSpinBox;
class QLabel;
class QTimer;
class QPaintEngine;
class SDMTensorProbeWidget;


class SDMTensorVisWidget : public QTensorVisWidget
{
	Q_OBJECT

public:
	SDMTensorVisWidget( QWidget* parent=NULL );
	~SDMTensorVisWidget();

	void setSDM( StatisticalDeformationModel* sdm );

	virtual QWidget* getControlWidget() 
	{ 
		return (QWidget*)m_extendedOptionsWidget; 
	}

	SDMTensorDataProvider* getSDMTensorDataProvider() { return m_curTensorDataProvider; }

	// Forwards to TensorProbeWidget
	void setProbes( QList<SDMTensorProbe> probes );
	QList<SDMTensorProbe> getProbes() const;

public slots:
	void updateTensorVis();
	void setPickedRay( RayPickingInfo );
	/// Toggle visibility of UI elements marked as advanced options
	void setExpertMode( bool );

protected slots:
	void updateUI();
	void updateMarkerLabel();
	void setTensorMethod( int method );
	void setTensorType  ( int type );
	void toggleDragMarker( bool b );
	void setMarkerToEdit();
	void onChangedMarkerRadius(double);
	void computeOverviewVis();
	void toggleHalo( bool show );
	void toggleMarker(bool);
	void resetMarker();
	void setHaloSize( double );

	void onAppliedProbe(int);
	void onCapturedProbe(int);

protected:
	void setupUI();
	void setupScene();
	void setupInteraction();

	void synchronizeHalo();

	// Specialize QTensorVisWidget
	void setTensorDataProvider( TensorDataProvider* tdata );

	//// Override QVTKWidget paint engine functions to enable custom overdraw
	//QPaintEngine* paintEngine() const;
	//void paintEvent( QPaintEvent* e );

private:
	StatisticalDeformationModel* m_sdm;
	RayPickingInfo               m_rayPickingInfo;

	QWidget*                     m_extendedOptionsWidget;
	SDMTensorProbeWidget*        m_probeWidget;

	QList<QWidget*>              m_advancedOptions;

	SDMTensorDataProvider*       m_curTensorDataProvider;
	EditLocalCovariance          m_editLocalCovariance;
	LinearLocalCovariance        m_linearLocalCovariance;

	/// The last non-SDM tensor field set by the super class, e.g. a tensor
	/// file from disk.
	TensorDataProvider*          m_superTensorDataProvider;

	TensorDataStatistics m_tdataStatistics;

	TensorVis2        m_halo;
	TensorDataAdaptor m_haloAdaptor;

	SphereMarker      m_sphereMarker;

	QDoubleSpinBox* m_spinGamma;
	QDoubleSpinBox* m_spinRadius;
	QLabel*         m_labelPosition;
	QLabel*         m_labelStatistics;

	QTimer*         m_interactorUpdateTimer;

	VTKPTR<vtkRenderWindowInteractor>         m_interactor;
	VTKPTR<vtkInteractorStyleTrackballCamera> m_interactorCamera;
	VTKPTR<vtkInteractorStyleTrackballActor>  m_interactorManipulate;
};

#endif // SDMTENSORVISWIDGET_H
