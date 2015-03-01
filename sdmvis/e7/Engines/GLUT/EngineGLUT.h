#ifndef ENGINEGLUT_H
#define ENGINEGLUT_H

#define USE_MICHAELS_TRACKBALL    // this is default, so remove this define?
//#define USE_GLUI                // obsolete?

#include "../Engine.h"
#include <cstdlib> // atexit()
#ifdef USE_MICHAELS_TRACKBALL
#include <GL/glew.h>
#endif

class EngineGLUT : public Engine
{
public:
	EngineGLUT();
	virtual ~EngineGLUT();

	void run( int argc, char* argv[] );

protected:
	// FIXME: Are abstract base class members automatically passed through or do 
	//        we have to define them again here in the derived class as abstract?
	//virtual void render ()               =0;

	// default implementations
	virtual void idle();
	virtual void reshape( int w, int h );
	virtual bool init( int argc, char* argv[] );
	virtual void destroy();

	virtual int getMouseX() { return m_mousex; };
	virtual int getMouseY() { return m_mousey; };

	void update();
	void setFrequentUpdate( bool b ) { m_force_update = b; }

private:
	void internal_init    ( int argc, char* argv[] );
	// make destroy() public for atexit() hack (see EngineGLUT.cpp)
	void internal_destroy ();
	// GLUT callbacks
	void internal_idle    ();
	void internal_render  ();
	void internal_reshape ( int w, int h );
	void internal_keyboard( unsigned char key, int x, int y );
	void internal_mouse   ( int button, int state, int x, int y );
	void internal_motion  ( int x, int y );

	// C-function callback hack requires to call above private member functions
	friend void EngineGLUT_atexit_callback();
	friend void EngineGLUT_render  ();
	friend void EngineGLUT_reshape ( int w, int h );
	friend void EngineGLUT_keyboard( unsigned char key, int x, int y );
	friend void EngineGLUT_mouse   ( int button, int state, int x, int y );
	friend void EngineGLUT_motion  ( int x, int y );
	friend void EngineGLUT_idle();

#ifdef USE_MICHAELS_TRACKBALL
	struct TrackballInfo
	{
		TrackballInfo()
			: dXRot(0),dYRot(0),dZoom(-5),dTransX(0),dTransY(0),dTransZ(0),
			  dSpeed(10),iOldX(-1),iOldY(-1),iButton(-1)
			{}
		// camera states
		GLdouble dXRot, dYRot, dZoom, dTransX, dTransY, dTransZ, dSpeed;
		float aaf_rot_matrix[ 4 ][ 4 ];
		// mouse states
		int iOldX, iOldY, iButton;
		// viewport state
		int iViewWidth, iViewHeight;
	};
	TrackballInfo m_tbinfo;
#endif

	int m_mousex, m_mousey;
	bool m_force_update;
};

#endif ENGINEGLUT_H
