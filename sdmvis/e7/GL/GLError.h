#ifndef GL_GLERROR_H
#define GL_GLERROR_H

#include "GLConfig.h"
#include <iostream>
#include <string>

#ifdef GL_NAMESPACE
namespace GL
{
#endif

	/// Returns false if an OpenGL error was signalled.
	/// The error message including a debug hint will be printed by default.
	/// Error checking is disabled in release mode (i.e. if define NDEBUG if set).
	bool checkGLError( std::string hint="", std::ostream& os=std::cerr );

	/// Provided for compatibility, internally calls \a checkGLError()
	bool CheckGLError( std::string hint="" );

	/// Translate OpenGL error code to descriptive string
	std::string getGLErrorString( GLenum errcode );

#ifdef GL_NAMESPACE
};
#endif

#endif
