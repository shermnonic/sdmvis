#ifndef SDMTENSOROVERVIEWWIDGET_H
#define SDMTENSOROVERVIEWWIDGET_H

#include <QWidget>

#include <vtkRenderer.h>
#include <vtkImageData.h>
#ifndef VTKPTR
#include <vtkSmartPointer.h>
#define VTKPTR vtkSmartPointer
#endif

#include "LocalCovarianceStatistics.h"
#include "VolVis.h"

class SDMTensorDataProvider;
class TensorVisBase;
class QVTKWidget;
class QSpinBox;
class QCheckBox;
class QLabel;
class QComboBox;

class SDMTensorOverviewWidget : public QWidget
{
	Q_OBJECT

signals:
	void statusMessage( QString );	
	
public:
	SDMTensorOverviewWidget( QWidget* parent=NULL );
	~SDMTensorOverviewWidget();

	QWidget* getControlWidget()
	{
		return m_controlWidget;
	}

	vtkRenderer* getRenderer() { return m_renderer; }
	
	void setTensorData( SDMTensorDataProvider* tdata );
	void setTensorVis( TensorVisBase* tvis );

	void setReference( VTKPTR<vtkImageData> img );
	
protected slots:
	void compute();
	bool load();
	bool load( const char* basepath );
	void selectMeasure( int );
	void selectIntegrator( int );
	void setOpacityScale( double scale );
	void setContourVisible( bool );
	void setContourIsovalue( double );
	void setContourOpacity( double );
	void redraw();
	void updateTransferFunction();

protected:
	QWidget* createControlWidget();

private:
	//typedef LocalCovarianceStatistics::ImageCollection ImageCollection;
	LocalCovarianceStatistics m_covarStat;
	
	QWidget*    m_controlWidget;
	QVTKWidget* m_vtkWidget;
	QSpinBox*   m_spinStep;
	QLabel*     m_labelRange;
	QComboBox*  m_comboMeasure;
	QCheckBox*  m_chkTFInvert;
	QCheckBox*  m_chkTFZeroBased;
	
	VolVis      m_volvis;
	VolVis      m_volvisReference;

	VTKPTR<vtkRenderer> m_renderer;
};

#endif // SDMTENSOROVERVIEWWIDGET_H
