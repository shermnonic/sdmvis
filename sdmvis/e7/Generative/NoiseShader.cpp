// Max Hermann, August 7, 2010
#include "NoiseShader.h"
#include "PerlinNoise.h"
#include <iostream>

//-----------------------------------------------------------------------------
//	GLSL Shaders
//-----------------------------------------------------------------------------

#define NOISESHADER_SHADERSTRING(S) #S

//"void main() { gl_Position = ftransform(); }";  // minimal vertex shader
//#version 120 \n
std::string NoiseShader::s_vshader = NOISESHADER_SHADERSTRING(
	varying vec3 pos;
	varying vec4 color;
	varying vec3 tc;
	void main(void)
	{
		tc = gl_MultiTexCoord0.xyz;
		pos = gl_Vertex.xyz;
		color = gl_Color;
		gl_Position = ftransform();
	}	
);

//"void main() { gl_FragColor = vec4(0,1,0,1); }"; // test fragment shader
//#version 120 \n
//  / 256.0 ).x * 256.0; }
std::string NoiseShader::s_fshader = NOISESHADER_SHADERSTRING(
	varying vec3 pos;
	varying vec4 color;
	varying vec3 tc;

	uniform sampler1D permtex;
	uniform sampler1D gradtex;

	vec3  fade( vec3 t )  { return t*t*t * (6.0*t*t - 15.0*t + 10.0); }
	float hash( float x ) { return texture1D( permtex, x ).x; }
	float grad( float x, vec3 p )
	{		
		x = x*16.0; /*same effect as: x = mod( x*256.0, 16.0 );*/
		return dot( texture1D(gradtex,x).xyz, p );
	}

	/* unoptimized 1:1 Perlin noise implementation */
	float inoise( vec3 p )
	{
		vec3 P = mod( floor(p), 256.0 );
		p -= floor(p);
		vec3 f = fade(p);

		P = P / 256.0;
		const float one = 1.0 / 256.0;

		float A = hash(P.x    )+P.y; float AA = hash(A)+P.z; float AB = hash(A+one)+P.z;
		float B = hash(P.x+one)+P.y; float BA = hash(B)+P.z; float BB = hash(B+one)+P.z;

		return mix( mix( mix( grad(hash(AA    ), p               ), 
		                      grad(hash(BA    ), p+vec3(-1.0, 0.0, 0.0)), f.x ),
		                 mix( grad(hash(AB    ), p+vec3( 0.0,-1.0, 0.0)),
						      grad(hash(BB    ), p+vec3(-1.0,-1.0, 0.0)), f.x ), f.y ),
		            mix( mix( grad(hash(AA+one), p+vec3( 0.0, 0.0,-1.0)), 
		                      grad(hash(BA+one), p+vec3(-1.0, 0.0,-1.0)), f.x ),
		                 mix( grad(hash(AB+one), p+vec3( 0.0,-1.0,-1.0)),
						      grad(hash(BB+one), p+vec3(-1.0,-1.0,-1.0)), f.x ), f.y ),
		            f.z );
	}

	float turbulence( vec3 p, int octaves, float lacunarity = 2.0, float gain = 0.5 )
	{
		float sum = 0.0;
		float freq = 1.0;
		float amp = 1.0;
		for( int i=0; i < octaves; ++i )
		{
			sum += abs(inoise(p*freq))*amp;
			freq *= lacunarity;
			amp *= gain;
		}
		return sum;
	}

	float fBm( vec3 p, int octaves, float lacunarity = 2.0, float gain = 0.5 )
	{
		float sum = 0.0;
		float freq = 1.0;
		float amp = 1.0;
		for( int i=0; i < octaves; ++i )
		{
			sum += inoise(p*freq)*amp;
			freq *= lacunarity;
			amp *= gain;
		}
		return sum;
	}

	// Ridged multifractal
	// See "Texturing & Modeling, A Procedural Approach", Chapter 12
	float ridge( float h, float offset )
	{
		h = abs(h);
		h = offset - h;
		h = h * h;
		return h;
	}

	float ridgedmf( vec3 p, int octaves, float lacunarity = 2.0, float gain = 0.5,
		            float offset = 1.0 )
	{
		float sum = 0.0;
		float freq = 1.0;
		float amp = 0.5;
		float prev = 1.0;
		for( int i=0; i < octaves; ++i )
		{
			float n = ridge( inoise(p*freq), offset );
			sum += n*amp*prev;
			prev = n;
			freq *= lacunarity;
			amp *= gain;
		}
		return sum;
	}

	void main()
	{
		float n = ridgedmf( tc, 3 );
			// fBm( tc, 5 )*0.5+0.5;
			// turbulence( tc, 2 );  
			// inoise( tc )*0.5 + 0.5;
		vec3 g = texture1D( gradtex, tc.x ).xyz;
		float c = texture1D( permtex, tc.x ).x;
		if( tc.y < -0.5 )
			gl_FragColor = vec4( c,c,c, 1.0 );
		else
		if( tc.y <  0.0 )
		{
			gl_FragColor = vec4( g*.5 + vec3(.5,.5,.5), 1.0 );			
		}
		else
			gl_FragColor = vec4( n,n,n, 1.0 );
	}
);
/*
	Following code fails with SHADERSTRING macro!
		float A = hash(P.x  )+P.y,  AA = hash(A)+P.z,  AB = hash(A+1)+P.z,
			  B = hash(P.x+1)+P.y,  BA = hash(B)+P.z,  BB = hash(B+1)+P.z;
*/


//-----------------------------------------------------------------------------
//	Implementation
//-----------------------------------------------------------------------------

NoiseShader::NoiseShader()
: m_shader(NULL) 
{}

NoiseShader::~NoiseShader()
{
	deinit();
	
	// FIXME: When to destroy GLTexture objects?
}

void NoiseShader::bind( GLuint unit_permtex, GLuint unit_gradtex )
{
	if( !m_shader ) return;
#if 1
	m_permtex.Bind( unit_permtex );
	m_gradtex.Bind( unit_gradtex );

	m_shader->bind();
	glUniform1i( m_loc_permtex, unit_permtex );
	glUniform1i( m_loc_gradtex, unit_gradtex );
#else
	m_shader->bind();
#endif
}

void NoiseShader::release()
{
	if( !m_shader) return;
	m_shader->release();
}

void NoiseShader::deinit()
{
	if( m_shader) delete m_shader; m_shader=NULL;
}

bool NoiseShader::init()
{
	using namespace std;

	// release previous shader
	if( m_shader ) deinit();

	// create shader
	m_shader = new GL::GLSLProgram;
	if( !m_shader )
	{
		cerr << "Error: Creation of NoiseShader GLSL shader failed!" << endl;
		return false;
	}

	// load shader
	if( !m_shader->load( s_vshader, s_fshader ) )
	{
		cerr << "Error: Loading NoiseShader failed!" << endl;
		return false;
	}

	// get uniform locations
	m_loc_permtex = m_shader->getUniformLocation( "permtex" );
	m_loc_gradtex = m_shader->getUniformLocation( "gradtex" );

	// create textures
	if( !m_permtex.Create(GL_TEXTURE_1D) || !m_gradtex.Create(GL_TEXTURE_1D) )
	{
		cerr << "Error: Creation of 2D textures failed!" << endl;
		return false;
	}
	unsigned char* perm_ptr = PerlinNoise::s_permutation;
	float* grad_ptr = PerlinNoise::s_gradients;

	// rescale permutation entries from [0,255] to [0.f,1.f]
	float perm_data[256];
	for( int i=0; i < 256; ++i ) perm_data[i] = perm_ptr[i]/255.f;
	float grad_data[16*3];
	for( int i=0; i < 16*3;++i ) grad_data[i] = grad_ptr[i];

	// 8bit luminance texture sufficient for unsigned char permutation table
	m_permtex.Image( 0, GL_LUMINANCE, 256, 0, GL_LUMINANCE, GL_FLOAT, perm_data );
	// gradients stored for simplicity in real float texture
	m_gradtex.Image( 0, GL_RGBA32F, 16, 0, GL_RGB, GL_FLOAT, grad_data );
	m_permtex.SetWrapMode( GL_REPEAT );
	m_permtex.SetFilterMode( GL_NEAREST );
	m_gradtex.SetWrapMode( GL_REPEAT );
	m_gradtex.SetFilterMode( GL_NEAREST );

	return GL::checkGLError( "NoiseShader::init()" );
}
