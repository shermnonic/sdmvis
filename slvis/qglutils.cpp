#include "qglutils.h"
#include "glbase.h"
#include <QDebug>
#include <QGLFormat>

namespace qglutils {

QString getProfileDescription( QGLFormat& glf )
{
	// Profile
	QGLFormat::OpenGLContextProfile profile = glf.profile();
	QString sProfile("Unknown");
	switch( profile )
	{
	case QGLFormat::CoreProfile:           sProfile=QString("Core");          break;
	case QGLFormat::CompatibilityProfile:  sProfile=QString("Compatibility"); break;
	case QGLFormat::NoProfile:             sProfile=QString("No");            break;
	}

	// Version
	QGLFormat::OpenGLVersionFlags flags = glf.openGLVersionFlags();
	QString version("Unknown");
	if( flags & 0x00000000 ) version  = QString("OpenGL Version None");
	if( flags & 0x00000001 ) version  = QString("OpenGL Version 1.1");
	if( flags & 0x00000002 ) version  = QString("OpenGL Version 1.2");
	if( flags & 0x00000004 ) version  = QString("OpenGL Version 1.3");
	if( flags & 0x00000008 ) version  = QString("OpenGL Version 1.4");
	if( flags & 0x00000010 ) version  = QString("OpenGL Version 1.5");
	if( flags & 0x00000020 ) version  = QString("OpenGL Version 2.0");
	if( flags & 0x00000040 ) version  = QString("OpenGL Version 2.1");
	if( flags & 0x00001000 ) version  = QString("OpenGL Version 3.0");
	if( flags & 0x00002000 ) version  = QString("OpenGL Version 3.1");
	if( flags & 0x00004000 ) version  = QString("OpenGL Version 3.2");
	if( flags & 0x00008000 ) version  = QString("OpenGL Version 3.3");
	if( flags & 0x00010000 ) version  = QString("OpenGL Version 4.0");
	//if( flags & 0x00000100 ) version  = QString("OpenGL ES CommonLite Version 1 0"); else
	//if( flags & 0x00000080 ) version  = QString("OpenGL ES Common Version 1 0"); else
	//if( flags & 0x00000400 ) version  = QString("OpenGL ES CommonLite Version 1 1"); else
	//if( flags & 0x00000200 ) version  = QString("OpenGL ES Common Version 1 1"); else
	//if( flags & 0x00000800 ) version  = QString("OpenGL ES Version 2 0"); else

	QString summary;
	summary += "Using " + version + " " + sProfile + " Profile";
	return summary;
}

QString getOpenGLDescription()
{
	QString 
		vendor   = (const char*)glGetString( GL_VENDOR ),
		renderer = (const char*)glGetString( GL_RENDERER ),
		version  = (const char*)glGetString( GL_VERSION ),
		glsl     = (const char*)glGetString( GL_SHADING_LANGUAGE_VERSION );

	QString summary;
	summary += " Vendor  : " + vendor   + "\n";
	summary += " Renderer: " + renderer + "\n";
	summary += " Version : " + version  + "\n";
	summary += " GLSL    : " + glsl     + "\n";

	return summary;
}

QGLFormat setupGLFormat()
{
	// Set OpenGL profile.
	// Set multisampling option for OpenGL.
	// Note that it still has to enabled in OpenGL via glEnable(GL_MULTISAMPLE)
    QGLFormat glf = QGLFormat::defaultFormat();
	glf.setProfile( QGLFormat::CompatibilityProfile );
    glf.setSampleBuffers(true);
    glf.setSamples(4);	
    QGLFormat::setDefaultFormat(glf);
	return glf;
}

bool setupGLEW()
{
	// Setup GLEW
	glewExperimental = GL_TRUE;	
	GLenum glew_err = glewInit();
	if( glew_err != GLEW_OK )
	{
		qDebug() << "GLEW Error: " << glewGetErrorString(glew_err);
		qDebug() << "Could not setup GLEW OpenGL extension manager!";
		return false;
	}
	
	// We require at least OpenGL 1.3
	qDebug() << "Using GLEW " << (const char*)glewGetString( GLEW_VERSION );
	if( !glewIsSupported("GL_VERSION_1_3") )
	{
		qDebug() << "GLEW Error: " << glewGetErrorString(glew_err);
		qDebug() << "OpenGL 1.3 not supported by current graphics hardware/driver!";
		return false;
	}
	
	return true;
}

bool initializeGL()
{
	bool ok = true;

	QGLFormat glf = setupGLFormat();
	qDebug() << qPrintable(getProfileDescription( glf ));

	ok = setupGLEW();
	qDebug() << qPrintable(getOpenGLDescription());

	return ok;
}

} // namespace qglutils