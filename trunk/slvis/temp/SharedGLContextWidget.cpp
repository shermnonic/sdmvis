#include "SharedGLContextWidget.h"
#include <QGLFormat>
#include <QDebug>
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

QString SharedGLContextWidget::getProfileDescription()
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
	summary << "Using" << qPrintable(version) << qPrintable(sProfile) << "Profile";
}

SharedGLContextWidget::SharedGLContextWidget( QWidget* parent )
: QGLWidget( parent ),
  m_man(0)
{
	// Set OpenGL profile.
	// Set multisampling option for OpenGL.
	// Note that it still has to enabled in OpenGL via glEnable(GL_MULTISAMPLE)
    QGLFormat glf = QGLFormat::defaultFormat();
	glf.setProfile( QGLFormat::CompatibilityProfile );
    glf.setSampleBuffers(true);
    glf.setSamples(4);
    QGLFormat::setDefaultFormat(glf);

	// Print some infos on first instantiation
	static bool firstCall = true;
	if( firstCall )
	{
		firstCall = false;
		qDebug() << getProdileDescription();
	}
}

void SharedGLContextWidget::initializeGL()
{
	// At first creation of the (shared) GL context, we have to initialize GLEW		
	static bool glewInitialized = false;
	
	if( !glewInitialized )
	{
		// Setup GLEW
		glewExperimental = GL_TRUE;	
		GLenum glew_err = glewInit();
		if( glew_err != GLEW_OK )
		{
			cerr << "GLEW Error:\n" << glewGetErrorString(glew_err) << endl;
			QMessageBox::warning( this, tr("%1 Error").arg(APP_NAME),
				tr("Could not setup GLEW OpenGL extension manager!\n") );
		}
		cout << "Using GLEW " << glewGetString( GLEW_VERSION ) << endl;
		if( !glewIsSupported("GL_VERSION_1_3") )
		{
			cerr << "GLEW Error:\n" << glewGetErrorString(glew_err) << endl;
			QMessageBox::warning( this, tr("%1 Warning").arg(APP_NAME),
				tr("OpenGL 1.3 not supported by current graphics hardware/driver!") );
		}

		glewInitialized = true;

		// Print some infos
		cout << "------------\nOpenGL info:\n";
		cout << " Vendor  : " << glGetString( GL_VENDOR ) << "\n";
		cout << " Renderer: " << glGetString( GL_RENDERER ) << "\n";;
		cout << " Version : " << glGetString( GL_VERSION ) << "\n";;
		cout << " GLSL    : " << glGetString( GL_SHADING_LANGUAGE_VERSION ) << "\n";;
		//printf(" Extensions: %s\n",glGetString( GL_EXTENSIONS ));
		cout << "------------\n";
		
	}

	// Enable multisampling
	// Note that multisampling is configured via QGLFormat
	glEnable( GL_MULTISAMPLE );

	// OpenGL default states
	glClearColor(0.f,0.f,0.f,1.f);
	glColor4f(1.f,1.f,1.f,1.f);
}
