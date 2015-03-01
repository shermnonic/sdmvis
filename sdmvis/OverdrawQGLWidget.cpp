#include "OverdrawQGLWidget.h"

#define OVERDRAWQGLWIDGET_DEBUG_COUT( msg ) 
	// std::cout << "OverdrawQGLWidget: " << msg << std::endl;

//----------------------------------------------------------------------------
//	C'tor
//----------------------------------------------------------------------------
OverdrawQGLWidget
  ::OverdrawQGLWidget( QWidget* parent, const QGLWidget* shareWidget, 
	                   Qt::WindowFlags f )
  : QGLWidget( parent, shareWidget, f )
{
	// We manually swap buffers, only after overdraw
	setAutoBufferSwap( false );	
}

//----------------------------------------------------------------------------
//	paintEvent()
//----------------------------------------------------------------------------
void OverdrawQGLWidget
  ::paintEvent( QPaintEvent* e )
{
	OVERDRAWQGLWIDGET_DEBUG_COUT("paintEvent()");

	//////////////////////////////////////////////////////////////////////////
	// Overpainting as explained in the Qt demo at
	//   http://qt-project.org/doc/qt-5.0/qtopengl/overpainting.html
	// did not work for QVTKWidget2. As workaround we are using the native
	// painting approach described on the Qt blog
	//   http://blog.qt.digia.com/blog/2010/01/06/qt-graphics-and-performance-opengl/
	// although imposes some overhead on OpenGL state changes.
	//////////////////////////////////////////////////////////////////////////
	
	QPainter painter(this);

	// Let VTK perform its OpenGL rendering
	painter.beginNativePainting();
	paintGL();
	painter.endNativePainting();
	
	// Qt overpainting
	overdraw( painter );
	painter.end();

	swapBuffers();
}
