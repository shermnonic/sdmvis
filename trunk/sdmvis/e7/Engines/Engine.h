#ifndef ENGINE_H
#define ENGINE_H

#include <iostream>
#include <string>
#include <cassert>
#include <exception>

class Engine
{
public:
	class InitException : public std::exception
	{
		std::string m_desc;
	public:
		InitException( std::string desc ): m_desc(desc) {}
		const char* what() const throw() { return m_desc.c_str(); }
	};

public:
	Engine()
		: m_winTitle("e7")
		, m_winWidth (512)
		, m_winHeight(512)
	{ 
		std::cout << "Engine() c'tor" << std::endl; 
	};
	virtual ~Engine() { std::cout << "Engine d'tor" << std::endl; };;

	virtual void run( int argc, char* argv[] )=0;
	
	virtual void set_window_title( const char* text ) { m_winTitle = text; }
	virtual void set_window_size ( int w, int h )     { m_winWidth=w; m_winHeight=h; }

	virtual int getMouseX() { return -1; }
	virtual int getMouseY() { return -1; }
	
protected:
	virtual void onKeyPressed( unsigned char key ) {};

	virtual void render ()               =0;
	virtual void idle   ()               =0;
	virtual void reshape( int w, int h ) =0;

	/// init() is called with valid OpenGL context
	virtual bool init( int argc, char* argv[] ) =0;
	virtual void destroy()                      =0;

	/// Return milliseconds passed since last call to this function
	float get_time_delta() { assert(false); };

	std::string m_winTitle;
	int         m_winWidth, 
		        m_winHeight;

private:
};

#endif ENGINE_H





