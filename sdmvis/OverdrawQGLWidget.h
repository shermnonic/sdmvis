#ifndef OVERDRAWQGLWIDGET_H
#define OVERDRAWQGLWIDGET_H

#include "OverdrawInterface.h"
#include <QGLWidget>

/// A QGLWidget with support for QPainter overdraw
class OverdrawQGLWidget : public QGLWidget, OverdrawInterface
{
public:
	OverdrawQGLWidget( QWidget* parent=0, const QGLWidget* shareWidget=0, 
	                   Qt::WindowFlags f=0 );

protected:
	// Interface
	virtual void overdraw( QPainter& painter )=0;

	// Combined OpenGL and overdraw rendering
	void paintEvent( QPaintEvent* e );
};

#endif // OVERDRAWQGLWIDGET_H
