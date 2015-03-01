// Max Hermann, August 7, 2010
#ifndef PERLINNOISE_H
#define PERLINNOISE_H

//-----------------------------------------------------------------------------
//	class PerlinNoise
//-----------------------------------------------------------------------------

/// Perlin Noise (reference software implementation)
/// Java reference implementation adapted from http://mrl.nyu.edu/~perlin/noise/.
/// Some functions adapted from GPUGems2 noise chapter.
/// Used value type float for comparison w/ GPU noise shader.
class PerlinNoise
{
public:
	static unsigned char s_permutation[512];
	static float s_gradients[3*16];

	static float noise( float x, float y, float z );
	
	/// turbulence (same as fBm w/ abs)
	static float turbulence( float x, float y, float z, 
	                         int octaves, float lacunarity=2.0, float gain=0.5);

	/// fractal sum (same as turbulence w/o abs)
	static float fBm( float x, float y, float z, 
	                  int octaves, float lacunarity=2.0, float gain=0.5 );

	/// ridged multifractal
	/// See "Texturing & Modeling, A Procedural Approach", Chapter 12
	static float ridge( float h, float offset );

	static float ridgedmf( float x, float y, float z, 
	                       int octaves, float lacunarity=2.0, float gain=0.5,
						   float offset=1.0 );

	/// fade curve
	static float fade( float t );
	
	/// linear interpolation
	static float lerp( float t, float a, float b );
	
	/// dot product between gradient and fractional position	
	static float grad( int hash, float x, float y, float z );
};

#endif // PERLINNOISE_H
