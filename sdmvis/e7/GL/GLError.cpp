#include "GLError.h"

#ifdef GL_NAMESPACE
namespace GL {
#endif

std::string getGLErrorString( GLenum errcode )
{
	std::string s;

	switch( errcode )
	{
	case GL_NO_ERROR         : s += "GL_NO_ERROR";          break;
	case GL_INVALID_ENUM     : s += "GL_INVALID_ENUM";      break;
	case GL_INVALID_VALUE    : s += "GL_INVALID_VALUE";     break;
	case GL_INVALID_OPERATION: s += "GL_INVALID_OPERATION"; break;
	case GL_STACK_OVERFLOW   : s += "GL_STACK_OVERFLOW";    break;
	case GL_STACK_UNDERFLOW  : s += "GL_STACK_UNDERFLOW";   break;
	case GL_OUT_OF_MEMORY    : s += "GL_OUT_OF_MEMORY";     break;
	default:
		s += "UNKNOWN GL ERROR!";
	}

	return s;
}

#ifdef NDEBUG
bool checkGLError( std::string /*hint*/, std::ostream& /*os*/ ) 
{
	return true;
}
#else
bool checkGLError( std::string hint, std::ostream& os ) 
{
    GLenum errCode;

	if( (errCode = glGetError()) != GL_NO_ERROR ) 
	{
		os << "OpenGL Error: " << getGLErrorString(errCode);
		os << " (" << hint << ")" << std::endl;
		return false;
    }

	return true;
}
#endif

bool CheckGLError( std::string hint ) 
{ 
	return checkGLError( hint ); 
}

#ifdef GL_NAMESPACE
}; // namespace GL
#endif

