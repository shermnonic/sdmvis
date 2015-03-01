#include "OverdrawQVTKWidget2.h"

#include <vtkCommand.h>
#include <vtkGenericOpenGLRenderWindow.h> // vtkRenderWindow specialization
#include <vtkRenderWindowInteractor.h>

#define OVERDRAWQVTKWIDGET2_DEBUG_COUT( msg ) 
	// std::cout << "OverdrawQVTKWidget2: " << msg << std::endl;

//----------------------------------------------------------------------------
//	C'tor
//----------------------------------------------------------------------------
OverdrawQVTKWidget2
  ::OverdrawQVTKWidget2( QWidget* parent )
  : QVTKWidget2( parent )
{
	// We manually swap buffers, only after overdraw
	setAutoBufferSwap( false );

	//////////////////////////////////////////////////////////////////////////
	// We follow here points 2 to 5 of the guide given in QVTKWidget2.cxx:
	//
	// if you want paintGL to always be called for each time VTK renders
	// 1. turn off EnableRender on the interactor,
	// 2. turn off SwapBuffers on the render window,
	// 3. add an observer for the RenderEvent coming from the interactor
	// 4. implement the callback on the observer to call updateGL() on this widget
	// 5. overload QVTKWidget2::paintGL() to call mRenWin->Render() instead iren->Render()
	//
	// Turning the interactor rendering off (step 1) will force paintGL()
	// to not render anything. To follow step 1, one thus can not call the 
	// super implementation of paintGL() but has to explicitly invoke
	// GetRenderWindow()->Render().
	//
	//////////////////////////////////////////////////////////////////////////
	GetRenderWindow()->SwapBuffersOff();
	GetRenderWindow()->GetInteractor()->EnableRenderOn();
	GetRenderWindow()->GetInteractor()->AddObserver( vtkCommand::RenderEvent, 
							 this, &OverdrawQVTKWidget2::callbackRenderEvent );	
}

//----------------------------------------------------------------------------
//	callbackRenderEvent()
//----------------------------------------------------------------------------
void OverdrawQVTKWidget2
  ::callbackRenderEvent( vtkObject* caller,
                         long unsigned int eventId,
                         void* callData  )
{
	OVERDRAWQVTKWIDGET2_DEBUG_COUT("callbackRenderEvent()");
	update();
}

//----------------------------------------------------------------------------
//	paintGL()
//----------------------------------------------------------------------------
void OverdrawQVTKWidget2
  ::paintGL()
{
	OVERDRAWQVTKWIDGET2_DEBUG_COUT("paintGL()")
	
	// Call VTK implementation
	QVTKWidget2::paintGL();	
}

//----------------------------------------------------------------------------
//	resizeGL()
//----------------------------------------------------------------------------
void OverdrawQVTKWidget2
  ::resizeGL( int w, int h )
{
	OVERDRAWQVTKWIDGET2_DEBUG_COUT("resizeGL()")
	
	// Call VTK implementation
	QVTKWidget2::resizeGL( w, h );
}

//----------------------------------------------------------------------------
//	paintEvent()
//----------------------------------------------------------------------------
void OverdrawQVTKWidget2
  ::paintEvent( QPaintEvent* e )
{
	OVERDRAWQVTKWIDGET2_DEBUG_COUT("paintEvent()");

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
		// When interactor rendering is turned off, one has to call instead: 
		//  QVTKWidget2::initializeGL();
		//  GetRenderWindow()->Render();
	painter.endNativePainting();
	
	// Qt overpainting
	overdraw( painter );
	painter.end();

	swapBuffers(); // required, although it should be invoked in painter.end()?
}
