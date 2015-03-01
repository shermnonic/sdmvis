#ifndef VARVISWIDGET_H
#define VARVISWIDGET_H

#include "QVTKWidget.h"
#include <QWidget>
#include <QMouseEvent>
#include <vtkSmartPointer.h>
#include <vtkActor.h>
class VarVisRender;
class VolumeControls;

class VarVisWidget : public QVTKWidget
{
   //Q_OBJECT  // not needed since no signals/slots used yet
public:
    VarVisWidget();
	void setVren(VarVisRender* vren){m_renderS=vren;}
	void setVolumeControls(VolumeControls *controls){m_volumeControls=controls;}

protected:
	// Overloaded Mouse Events for turning of the 
	// Silhouette when mouse is pressed , for performance ...
	virtual void mousePressEvent  ( QMouseEvent* e );
	virtual void mouseMoveEvent  ( QMouseEvent* e );
	virtual void mouseReleaseEvent( QMouseEvent* e );
	virtual void wheelEvent(QWheelEvent*);

	virtual void keyPressEvent(QKeyEvent *e);
	

private:
	VarVisRender   *m_renderS;
	VolumeControls *m_volumeControls;
	int oldX;
	int oldY;

	vtkSmartPointer<vtkActor>  left ;
	vtkSmartPointer<vtkActor>  right ;
	vtkSmartPointer<vtkActor>  up ;
	vtkSmartPointer<vtkActor>  down ;
};

#endif // VARVISWIDGET_H
