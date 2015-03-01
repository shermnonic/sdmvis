#ifndef OVERDRAWQVTKWIDGET2_H
#define OVERDRAWQVTKWIDGET2_H

#include "OverdrawInterface.h"
#include <QVTKWidget2.h>

/// A QVTKWidget2 with support for QPainter overdraw
class OverdrawQVTKWidget2 : public QVTKWidget2, OverdrawInterface
{
public:
	OverdrawQVTKWidget2( QWidget* parent=0 );

protected:
	// Interface
	virtual void overdraw( QPainter& painter ) = 0;
	
	// Override QVTKWidget2 implementation of QGLWidget
	void paintGL();
	void resizeGL( int w, int h );

	// Combined VTK and overdraw rendering
	void paintEvent( QPaintEvent* e );

	// Callback to connect a VTK RenderEvent to QWidget::update()
	void callbackRenderEvent( vtkObject* caller,
                    long unsigned int eventId,
                    void* callData  );
};

#endif // OVERDRAWQVTKWIDGET2_H
