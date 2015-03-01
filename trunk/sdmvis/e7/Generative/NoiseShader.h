// Max Hermann, August 7, 2010
#ifndef NOISESHADER_H
#define NOISESHADER_H

#include <GL/GLSLProgram.h>
#include <GL/GLError.h>
#include <GL/GLTexture.h>
#include <string>

//-----------------------------------------------------------------------------
//	class NoiseShader
//-----------------------------------------------------------------------------

/// Perlin Noise (simple GPU implementation)
class NoiseShader
{
	static std::string s_vshader, s_fshader;

public:
	NoiseShader();
	~NoiseShader();

	bool init();
	void deinit();

	void bind( GLuint unit_permtex=0, GLuint unit_gradtex=1 );
	void release();

private:
	GL::GLSLProgram *m_shader;
	GL::GLTexture    m_permtex,
		             m_gradtex;
	GLint m_loc_permtex,
	      m_loc_gradtex;
};

#endif // NOISESHADER_H
