#ifndef SHAREDGLCONTEXTWIDGET_H
#define SHAREDGLCONTEXTWIDGET_H

#include "glbase.h"  // Platform indep. OpenlGL headers and GLEW extensions
#include <QGLWidget>

/**
	\class SharedGLContextWidget
	- Provide OpenGL context shared between previews and screens.
	- Setup GL extension manager GLEW in initializeGL().
	- Frequent update of RenderModule's of a ModuleManager in paintGL().
*/
class SharedGLContextWidget : public QGLWidget
{
	Q_OBJECT
public:
	SharedGLContextWidget( QWidget* parent );

	QString getProfileDescription();

protected:
	///@name QGLWidget implementation
	///@{ 
	void initializeGL();
    void resizeGL( int /*w*/, int /*h*/ ) {}
	void paintGL();
	///@}
};

#endif // SHAREDGLCONTEXTWIDGET_H
