// Max Hermann, August 7, 2010
#include "PerlinNoise.h"
#include <cmath>

//linux/ unix specific includes
#ifndef WIN32
#include <stdlib.h>
#endif

unsigned char PerlinNoise::s_permutation[512] = { 151,160,137,91,90,15,
   131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
   190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
   88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
   77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
   102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
   135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
   5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
   223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
   129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
   251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
   49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
   138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
                                                  151,160,137,91,90,15,
   131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
   190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
   88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
   77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
   102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
   135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
   5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
   223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
   129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
   251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
   49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
   138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
   };

float PerlinNoise::s_gradients[3*16] = 
{
	1,1,0,   -1,1,0,   1,-1,0,   -1,-1,0,
	1,0,1,   -1,0,1,   1,0,-1,   -1,0,-1,
	0,1,1,   0,-1,1,   0,1,-1,   0,-1,-1,
	1,1,0,   0,-1,1,   -1,1,0,   0,-1,-1   // this line taken from GPUGems2
								           // (unclear why its not repeated in GPUGems2)
/*	
	1,1,0,   -1,1,0,   1,-1,0,   -1,-1,0   // repeat first 4 entries for 
	                                       // indexing using modulo arithmetic
*/
};

float PerlinNoise::fade( float t )	
{ 
	return t*t*t * (6*t*t - 15*t + 10);   // improved noise
	//return t*t * (3 - 2*t);             // classical noise
}

float PerlinNoise::noise( float x, float y, float z )
{
	static unsigned char* hash = s_permutation;

	// integer part for indexing hash table
	int X = (int)x & 255,
		Y = (int)y & 255,
		Z = (int)z & 255;
	// fractional part
	x -= (int)x;
	y -= (int)y;
	z -= (int)z;

	float u = fade(x),
		   v = fade(y),
		   w = fade(z);

	int A = hash[X  ]+Y,  AA = hash[A]+Z,  AB = hash[A+1]+Z,
		B = hash[X+1]+Y,  BA = hash[B]+Z,  BB = hash[B+1]+Z;

	return lerp(w, lerp(v, lerp(u, grad(hash[AA  ], x  , y  , z   ),
								   grad(hash[BA  ], x-1, y  , z   )),
						   lerp(u, grad(hash[AB  ], x  , y-1, z   ),
								   grad(hash[BB  ], x-1, y-1, z   ))),
				   lerp(v, lerp(u, grad(hash[AA+1], x  , y  , z-1 ),
								   grad(hash[BA+1], x-1, y  , z-1 )),
						   lerp(u, grad(hash[AB+1], x  , y-1, z-1 ),
								   grad(hash[BB+1], x-1, y-1, z-1 ))));
}

float PerlinNoise::turbulence( float x, float y, float z, 
						       int octaves, float lacunarity, float gain )
{
	float sum  = 0.0,
		  freq = 1.0,
		  amp  = 1.0;
	for( int i=0; i < octaves; ++i )
	{
		sum += fabs( noise(freq*x,freq*y,freq*z) )*amp;
		freq *= lacunarity;
		amp *= gain;
	}
	return sum;
}

float PerlinNoise::fBm( float x, float y, float z, 
				        int octaves, float lacunarity, float gain )
{
	float sum  = 0.0,
		  freq = 1.0,
		  amp  = 1.0;
	for( int i=0; i < octaves; ++i )
	{
		sum += noise(freq*x,freq*y,freq*z)*amp;
		freq *= lacunarity;
		amp *= gain;
	}
	return sum;
}

float PerlinNoise::ridge( float h, float offset )
{
	h = abs(h);
	h = offset - h;
	h = h * h;
	return h;
}

float PerlinNoise::ridgedmf( float x, float y, float z, 
					         int octaves, float lacunarity, float gain,
					         float offset )
{
	float sum  = 0,
		  freq = 1.0, 
		  amp  = 0.5,
		  prev = 1.0;
	for( int i=0; i < octaves; ++i ) 
	{
		float n = ridge(noise(freq*x,freq*y,freq*z), offset);
		sum += n*amp*prev;
		prev = n;
		freq *= lacunarity;
		amp *= gain;
	}
	return sum;
}

float PerlinNoise::lerp( float t, float a, float b ) { return a + t*(b-a); }

float PerlinNoise::grad( int hash, float x, float y, float z )
{
  #if 0
	// tricky bit-fiddling gradient dot-product lookup
	int h = hash & 15;                      // CONVERT LO 4 BITS OF HASH CODE
	double u = h<8 ? x : y,                 // INTO 12 GRADIENT DIRECTIONS.
		 v = h<4 ? y : h==12||h==14 ? x : z;
	return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
  #else
	int h = hash & 15;
	static float* g = s_gradients;
	return g[3*h+0]*x + g[3*h+1]*y + g[3*h+2]*z;
  #endif
}
