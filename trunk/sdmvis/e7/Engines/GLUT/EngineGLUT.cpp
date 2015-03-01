#include "EngineGLUT.h"
#include <GL/glew.h>
#include <GL/glut.h>
#include <iostream>
#ifdef USE_MICHAELS_TRACKBALL
	#include "MichaelsTrackball.inl"
	using namespace MichaelsTrackball;
#endif

//------------------------------------------------------------------------------
//	Callback hack
//------------------------------------------------------------------------------

// this pointer of current engine instance.
// Assumes that only one engine is will be instanciated.
static EngineGLUT* EngineGLUT_instance=NULL;

// atexit() callback hack
void EngineGLUT_atexit_callback()
{
	std::cout << "EngineGLUT_atexit_callback()" << std::endl;
	if( EngineGLUT_instance )
		EngineGLUT_instance->internal_destroy();
}

void EngineGLUT_render  ()
{
	if( EngineGLUT_instance )
		EngineGLUT_instance->internal_render();
}

void EngineGLUT_reshape ( int w, int h )
{
	if( EngineGLUT_instance )
		EngineGLUT_instance->internal_reshape( w, h );
}

void EngineGLUT_keyboard( unsigned char key, int x, int y )
{
	if( EngineGLUT_instance )
		EngineGLUT_instance->internal_keyboard( key, x, y );
}

void EngineGLUT_mouse   ( int button, int state, int x, int y )
{
	if( EngineGLUT_instance )
		EngineGLUT_instance->internal_mouse( button, state, x, y );
}

void EngineGLUT_motion  ( int x, int y )
{
	if( EngineGLUT_instance )
		EngineGLUT_instance->internal_motion( x, y );
}

void EngineGLUT_idle()
{
	if( EngineGLUT_instance )
		EngineGLUT_instance->internal_idle();
}


//------------------------------------------------------------------------------
// 	C'tor & D'tor
//------------------------------------------------------------------------------

EngineGLUT::EngineGLUT()
: m_mousex(-1),m_mousey(-1),m_force_update(true)
{ 
	std::cout << "EngineGLUT() c'tor" << std::endl;
	if( EngineGLUT_instance )
		// The atexit() hack currently only allows only a singleton!
		assert(false);
	EngineGLUT_instance = this;
};

EngineGLUT::~EngineGLUT() 
{ 
	std::cout << "EngineGLUT d'tor" << std::endl; 
	EngineGLUT_instance = NULL;
};


//------------------------------------------------------------------------------
// 	Default implementations of user func's
//------------------------------------------------------------------------------

bool EngineGLUT::init( int argc, char* argv[] )
{
	return true;
}
	
void EngineGLUT::destroy()
{
	std::cout << "EngineGLUT::destroy()" << std::endl;
}
	
void EngineGLUT::idle()
{}

void EngineGLUT::reshape ( int w, int h )
{
	float aspect = (float)w/(float)h;

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluPerspective( 45, aspect, 0.1, 42 );
	glMatrixMode( GL_MODELVIEW );
}

//------------------------------------------------------------------------------
// 	GLUT callbacks
//------------------------------------------------------------------------------

void EngineGLUT::update()
{
#ifdef USE_GLUI
	if ( glutGetWindow() != main_window )
	glutSetWindow(main_window);
	glutPostRedisplay();
#else
	internal_render();
#endif
}

void EngineGLUT::internal_idle()
{
	idle();

	// force frequent update
	if( m_force_update )
		update();
}

void EngineGLUT::internal_render()
{
#ifdef USE_MICHAELS_TRACKBALL
	// apply trackball camera
	glLoadIdentity( );
	normalize_quat( curquat );
	build_rotmatrix( m_tbinfo.aaf_rot_matrix, curquat );
	glTranslatef( 0.0, 0.0, m_tbinfo.dZoom );
	glMultMatrixf( &( m_tbinfo.aaf_rot_matrix[ 0 ][ 0 ] ) );
	glTranslatef( m_tbinfo.dTransX, m_tbinfo.dTransY, m_tbinfo.dTransZ );
#else
	// some odd defaults
	glLoadIdentity( );
	glTranslatef( 0,0,-3 );
#endif

	// call user render routine
	render();

	//	glFlush();
	//	glFinish();
	// -> Flush() and Finish() implicit in glutSwapBuffers()
	glutSwapBuffers();	
}

void EngineGLUT::internal_reshape ( int w, int h )
{
	glViewport( 0,0, w,h );
	set_window_size( w, h );
#ifdef USE_MICHAELS_TRACKBALL
	m_tbinfo.iViewWidth  = w;
	m_tbinfo.iViewHeight = h;
#endif
	reshape( w, h );
	update();
}

void EngineGLUT::internal_keyboard( unsigned char key, int x, int y )
{	
#if 1
	onKeyPressed( key );
#else
	switch( key )
	{
		case 27:
		case 'q':
			exit( 0 );
			break;

		//default:
			//toggle[key] = !toggle[key];
			
			// TODO: nice default key handling mechanism!
	};
#endif
}

void EngineGLUT::internal_mouse( int button, int state, int mousex, int mousey )
{
	// update internal state
	m_mousex = mousex;
	m_mousey = mousey;

	int mousekey;	
	if( state==GLUT_UP )
	{
		mousekey = 0;
#ifdef USE_MICHAELS_TRACKBALL
		m_tbinfo.iOldX = -1;
		m_tbinfo.iOldY = -1;
		m_tbinfo.iButton = -1;
#endif
	}
	else
	if( (button==GLUT_LEFT_BUTTON)&&(state==GLUT_DOWN) )
	{
		mousekey = 1;
#ifdef USE_MICHAELS_TRACKBALL
		m_tbinfo.iOldX = mousex;
		m_tbinfo.iOldY = mousey;
		m_tbinfo.iButton = mousekey;
#endif
	}
	else
	if( (button==GLUT_RIGHT_BUTTON)&&(state==GLUT_DOWN) )
	{
		mousekey = 2;
#ifdef USE_MICHAELS_TRACKBALL
		m_tbinfo.iOldX = mousex;
		m_tbinfo.iOldY = mousey;
		m_tbinfo.iButton = mousekey;
#endif
	}
	else
	if( (button==GLUT_MIDDLE_BUTTON)&&(state==GLUT_DOWN) )
	{
		mousekey = 3;
#ifdef USE_MICHAELS_TRACKBALL
		m_tbinfo.iOldX = mousex;
		m_tbinfo.iOldY = mousey;
		m_tbinfo.iButton = mousekey;
#endif
	}
}

void EngineGLUT::internal_motion( int mousex, int mousey )
{
	// update internal state
	m_mousex = mousex;
	m_mousey = mousey;

#ifdef USE_MICHAELS_TRACKBALL
	static GLfloat aaf_mat[4][4];
	
	switch( m_tbinfo.iButton )
	{
	case 1:
        m_tbinfo.dXRot += ( ( double ) ( m_tbinfo.iOldX - mousex ) ) / m_tbinfo.iViewWidth;
        m_tbinfo.dYRot += ( ( double ) ( m_tbinfo.iOldY - mousey ) ) / m_tbinfo.iViewHeight;
        trackball( lastquat,
                   0.5 * ( 2.0 * m_tbinfo.iOldX - m_tbinfo.iViewWidth ) / m_tbinfo.iViewWidth,
                   0.5 * ( m_tbinfo.iViewHeight - 2.0 * m_tbinfo.iOldY ) / m_tbinfo.iViewHeight,
                   0.5 * ( 2.0 * mousex - m_tbinfo.iViewWidth ) / m_tbinfo.iViewWidth,
                   0.5 * ( m_tbinfo.iViewHeight - 2.0 * mousey ) / m_tbinfo.iViewHeight );
        add_quats( lastquat, curquat, curquat );
		break;

	case 2:
        if( m_tbinfo.iOldX != -1 )
            m_tbinfo.dZoom = m_tbinfo.dZoom + ( mousey - m_tbinfo.iOldY ) * ( m_tbinfo.dSpeed - m_tbinfo.dZoom ) / 5000.0;
		break;

	case 3:
        build_rotmatrix( aaf_mat, curquat );

		m_tbinfo.dTransX -= aaf_mat[0][0]*( m_tbinfo.iOldX - mousex ) * ( m_tbinfo.dSpeed - m_tbinfo.dZoom ) / 10000.0;
        m_tbinfo.dTransY -= aaf_mat[1][0]*( m_tbinfo.iOldX - mousex ) * ( m_tbinfo.dSpeed - m_tbinfo.dZoom ) / 10000.0;
        m_tbinfo.dTransZ -= aaf_mat[2][0]*( m_tbinfo.iOldX - mousex ) * ( m_tbinfo.dSpeed - m_tbinfo.dZoom ) / 10000.0;
        m_tbinfo.dTransX += aaf_mat[0][1]*( m_tbinfo.iOldY - mousey ) * ( m_tbinfo.dSpeed - m_tbinfo.dZoom ) / 10000.0;
        m_tbinfo.dTransY += aaf_mat[1][1]*( m_tbinfo.iOldY - mousey ) * ( m_tbinfo.dSpeed - m_tbinfo.dZoom ) / 10000.0;
        m_tbinfo.dTransZ += aaf_mat[2][1]*( m_tbinfo.iOldY - mousey ) * ( m_tbinfo.dSpeed - m_tbinfo.dZoom ) / 10000.0;
		break;
	}

    m_tbinfo.iOldX = mousex;
	m_tbinfo.iOldY = mousey;
#endif

	update();
}

//------------------------------------------------------------------------------
// 	internal_init()
//------------------------------------------------------------------------------
void EngineGLUT::internal_init( int argc, char* argv[] )
{	
	// register exit() callback since GLUT can only be terminated via exit()
#if 0
	// Depends on stdlib implementation if "C++" function can be used
	// (definitely not supported under Visual Studio).
	void (EngineGLUT::*destroy)() = &EngineGLUT::internal_destroy;
#else
	// Workaround using global function
	void (*destroy)() = EngineGLUT_atexit_callback;
#endif
	if( atexit( destroy ) != 0 )
		throw InitException("Error in registering atexit() callback!");
	
#ifdef USE_MICHAELS_TRACKBALL
	// init trackball
    trackball( curquat, 0.0, 0.0, 0.0, 0.0 );
	m_tbinfo.iButton = -1;
#endif	
	
	//----------------------------------------------------------------
	// 	Init GLUT
	//----------------------------------------------------------------
	
	glutInit( &argc, argv );
	glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
	glutInitWindowSize( m_winWidth, m_winHeight );
	int main_window = glutCreateWindow( m_winTitle.c_str() );

	// callbacks
#if 0
	// member function pointers cannot be passe to standard GLUT implementation
	void (EngineGLUT::*reshape )( int, int ) 				= &EngineGLUT::internal_reshape;
	void (EngineGLUT::*keyboard)( unsigned char, int, int ) = &EngineGLUT::internal_keyboard;
	void (EngineGLUT::*mouse   )( int, int, int, int )      = &EngineGLUT::internal_mouse;
	void (EngineGLUT::*motion  )( int, int )                = &EngineGLUT::internal_motion;
#else
	void (*reshape )( int, int ) 				= EngineGLUT_reshape;
	void (*keyboard)( unsigned char, int, int ) = EngineGLUT_keyboard;
	void (*mouse   )( int, int, int, int )      = EngineGLUT_mouse;
	void (*motion  )( int, int )                = EngineGLUT_motion;
	void (*idle    )()                          = EngineGLUT_idle;
	void (*display )()                          = EngineGLUT_render;
#endif
	
#ifdef USE_GLUI
	GLUI_Master.set_glutReshapeFunc ( reshape );  
	GLUI_Master.set_glutKeyboardFunc( keyboard );
	GLUI_Master.set_glutSpecialFunc ( NULL );
	GLUI_Master.set_glutMouseFunc   ( mouse );
	glutMotionFunc( motion );
	std::cout << "Using GLUI " << GLUI_Master.get_version() << std::endl;
#else
	glutReshapeFunc ( reshape );
	glutKeyboardFunc( keyboard );
	glutMouseFunc   ( mouse );
	glutMotionFunc  ( motion );
	glutIdleFunc    ( idle );
#endif
	glutDisplayFunc ( display );
		
	//----------------------------------------------------------------
	// 	Init GLEW
	//----------------------------------------------------------------
		
	glewExperimental = GL_TRUE;
	
	GLenum glew_err = glewInit();
	if( glew_err != GLEW_OK )
	{
		std::cerr << "GLEW error:" << glewGetErrorString(glew_err) << std::endl;
		throw InitException( "Error on initializing GLEW library!" );
	}
	
	//----------------------------------------------------------------
	// 	User init
	//----------------------------------------------------------------	
	
	if( !init( argc, argv ) )
	{
		internal_destroy();
	}
	reshape( m_winWidth, m_winHeight );
}

//------------------------------------------------------------------------------
// 	internal_destroy()
//------------------------------------------------------------------------------
void EngineGLUT::internal_destroy()
{
	// call user func
	destroy();	

	// HACK: call own destructor (since exit() ends the program)
	this->~EngineGLUT();
}

//------------------------------------------------------------------------------
// 	run()
//------------------------------------------------------------------------------
void EngineGLUT::run( int argc, char* argv[] )
{
	internal_init( argc, argv );
	glutMainLoop();	
	// never reached, glutMainLoop() is terminated via an exit() call
}
