///////////////////////////////////////////////////////////////////////////////
// raycast - fragment shader
//
// Straight forward single-pass raycasting. Shader is initialized with
// traversal start and end positions in "fronttex" and "backtex". 
// In the easiest case you just render the front- respectively back-faces 
// of the volume bounding box to the front- and back-textures with canonical
// RGB coloring from (0,0,0) to (1,1,1).
//
// We use here custom preprocessor strings "__opt__XXX__".
// (Could think about default value definition in-code which would
//  be automatically set if preprocessor define is not set by host.
//  Could use something like "__opt__XXX__[default_value_here]__".)
//
// TODO:
// - allow optional "fronttex_depth" input sampler which can be used to
//   calculate the real depth value of the rendered isosurface voxel
//   to write it out to gl_FragDepth.
// 
// Max Hermann, March 22, 2010
///////////////////////////////////////////////////////////////////////////////

#version 120

// Debug modes:
// 1 = debug ray termination (red=left volume, green=opacity>=threshold)
// 2 = render ray length
// 3 = render texture coordinate
//#define DEBUG

// replace MIP rendering by Groeller's MIDA algorithm
// - performance loss because early-ray termination not applicable
// - most notable differences to DVR when LIGHTING turned on
// TODO: * check correctness of implementation 
//       * add gamma parameter to interpolate between DVR, MIDA and MIP
//#define MIDA_TEST

// Lighting for direct volume rendering (works, but pretty slow)
//#define LIGHTING

// Texture filtering (assuming texture mode is GL_LINEAR)
//   0 - simulate nearest neighbour
//   1 - hw linear filtering
#define FILTER 1

// Color mode (if warps are used)
// 0 - plain white
// 1 - color warp strength in Red channel
// 2 - custom warp strength coloring (see code below)
// 3 - fancy coloring (add displacement to phong color)
//#define COLORMODE <__opt_COLORMODE__>
#define COLORMODE 0

// Non-polygonal isosurface rendering with Phong shading
// and intersection refinement
#define ISOSURFACE <__opt_ISOSURFACE__>

// Maximum intensity projection (DVR, lighting supported but useless in general)
#define MIP        <__opt_MIP__>

// Simple silhouette shader (comparing abs(dot(n,view)) < eps)
#define SILHOUETTE <__opt_SILHOUETTE__>

// Numbner of channels per element (1=scalar field, 3=vector field)
#define CHANNELS   <__opt_CHANNELS__>

// Warp volume by using a second offset texture for displacement
#define WARP       <__opt_WARP__>

// Support for several warp volumes (new samplers and uniforms!), requires WARP
#define MULTIWARP  <__opt_MULTIWARP__>

// A mean warp volume can be added to warp offset
#define MEANWARP   <__opt_MEANWARP__>

// We support displacement and stationary velocity fields
//  0 - displacement field (use -u for inverse)
//  1 - velocity field (use exp(-u) for inverse), integrate with Euler
//  2 - velocity field (use exp(-u) for inverse), integrate with Midpoint
//  3 - velocity field (use exp(-u) for inverse), integrate with Runge Kutta 4
// Steps is the number of steps of the integrator, 2 should be sufficient for RK4
#define INTEGRATOR <__opt_INTEGRATOR__>
#define INTEGRATOR_STEPS <__opt_INTEGRATOR_STEPS__>

// Depth buffer texture for front geometry supplied?
#define DEPTHTEX   <__opt_DEPTHTEX__>

// Output configuration
#define NUMOUTPUTS <__opt_NUMOUTPUTS__>
#define OUTPUT1    <__opt_OUTPUT1__>
#define OUTPUT2    <__opt_OUTPUT2__>
#define OUTPUT3    <__opt_OUTPUT3__>
#define OUTPUT4    <__opt_OUTPUT4__>
#define OUTPUT5    <__opt_OUTPUT5__>

// input
uniform sampler3D voltex;
uniform sampler2D fronttex;
uniform sampler2D backtex;
uniform sampler1D luttex;
varying vec4 color;
varying vec2 tc;
#if WARP==1

<__auto_warp_uniforms__>

  #if MULTIWARP==1
  #else
	uniform vec3      warp_ofs; // custom offset (yet only used for single warptex)	
  #endif
	
  #if MEANWARP==1
	uniform sampler3D meanwarptex; // additional offset texture
  #endif	
#endif
#if DEPTHTEX==1
uniform sampler2D depthtex; // depth buffer of front geometry
#endif

uniform float isovalue;     // non-polygonal isosurface
uniform float alpha_scale;  // arbitrary alpha scaling for DVR

// size of a voxel (for finite differences)
// used for:
// - normal calculation
// - displacement vector scaling 
const vec3 voxelsize = vec3( 1.0/<__opt_WIDTH__>.0, 
                             1.0/<__opt_HEIGHT__>.0, 
							 1.0/<__opt_DEPTH__>.0 );
const vec3 voxelsizei = vec3( <__opt_WIDTH__>, 
                              <__opt_HEIGHT__>, 
							  <__opt_DEPTH__> );					 

// stepsize for ray traversal
uniform float stepsize;
// number of isosurface refinement steps
const int refinement_steps = 5;

// light/material definition                 FIXME: should be adjustable (use GL_LIGHT0?)
const vec3  light_pos = vec3(1,1,1);
#if COLORMODE == 1
// choose phong with higher contrast for red/blue colormode
const float mat_ka  = 0.10; //0.04;  // ambient  coeff.
const float mat_ks  = 0.16; //0.36;  // specular coeff.
const float mat_kd  = 0.65; //0.6;   // diffuse  coeff.
#else
const float mat_ka  = 0.23;  // ambient  coeff.
const float mat_ks  = 0.06;  // specular coeff.
const float mat_kd  = 0.7;   // diffuse  coeff.
#endif


//------------------------------------------------------------------------------
// Globals

vec3 g_displacement = vec3(0.0);

vec3 g_normal;


//------------------------------------------------------------------------------
// Transfer function and opacity correction

// 1 - online opacity correction  & pre-multiplied alpha (like GPU Gems)
// 2 - offline opacity correction & pre-multiplied alpha (like GPU Gems)
// 3 - offline opacity correction (VTK style, allows usage of Paraview XML presets)
#define OPACITY_CORRECTION 3

// Apply transfer function
vec4 transfer( float scalar )
{
#if 0	
	// Hard-coded TF to achieve nice results on mandible datasets	
	intensity *= (1.5 - 100.0*stepsize);
	vec4 src = vec4(intensity);
	src.a *= alpha_scale;
	src.rgb *= src.a; // Pre-multiplied alpha
	return src;
#else
	// Use lookup table
	vec4 src = texture1D( luttex, scalar );
#endif	
	
  #if   OPACITY_CORRECTION == 1
	// Opacity correction
	src.a = (1.0 - pow(1.0 - src.a, SR0/SR));
	
	// Opacity weighted color [Wittenbrink1998]
	src.rgb *= src.a;
	
  #elif OPACITY_CORRECTION == 2
	// Nothing to be done here, everything pre-computed
	
  #elif OPACITY_CORRECTION == 3	
	// VTK custom adaption (opacities somehow corrected, but no pre-mult.alpha)
	src.a *= 128.0;
	src.a = clamp(src.a,0.0,1.0);
	src.rgb *= src.a;	
  #endif
	
	return src;
}

// Undo transfer function mapping (e.g. for display of the original color map)
vec4 untransfer( float scalar )
{
	vec4 lut = texture1D( luttex, scalar );
	
  #if   OPACITY_CORRECTION == 1
	// Linear transfer function (sample rate corrected on-line above)
	float opacity = lut.a;
	vec3 color = lut.rgb;
	
  #elif OPACITY_CORRECTION == 2
	// Undo pre-computed opacity correction
	float opacity = 1.0 - pow(1.0 - lut.a,SR/SR0); // GPUGems
	// Undo pre-multiplied alpha
	vec3 color = lut.rgb / lut.a;
	
  #elif OPACITY_CORRECTION == 3
	// Undo pre-computed opacity correction
	float opacity = 1.0 - pow(1.0 - lut.a,1.0/stepsize); // VTK
	// No pre-multiplied alpha
	vec3 color = lut.rgb;	
  #endif	

	return vec4( color, opacity );
}

#if WARP == 1
//------------------------------------------------------------------------------
// Return displacement vector
// (Used in both displacement and stationary field setting)
vec3 get_warp( vec3 x )
{
	// Hard-coded shift-scale of vector field 3D textures
  #if 0	
	const float mode_scale = 200.0;
	const float mode_shift = 0.5;
  #else
	const float mode_scale = 40.0;
	const float mode_shift = 0.5;
  #endif
	
  #if MULTIWARP==1
	const float eps = 0.001;
	vec3 disp = vec3(0,0,0);

   #if 1
	// Automatically generated scaling code (enables variable number of modes)
<__auto_warp_routine__>
   #else
	// Hard-coded mixture of 5 modes
    float s = 0.1; // arbitrary scaling for debugging purposes
	if( abs(lambda0)>eps ) disp += s * lambda0 * mode_scale*(texture3D(warpmode0, x).rgb - mode_shift) * voxelsize;
	if( abs(lambda1)>eps ) disp += s * lambda1 * mode_scale*(texture3D(warpmode1, x).rgb - mode_shift) * voxelsize;
	if( abs(lambda2)>eps ) disp += s * lambda2 * mode_scale*(texture3D(warpmode2, x).rgb - mode_shift) * voxelsize;
	if( abs(lambda3)>eps ) disp += s * lambda3 * mode_scale*(texture3D(warpmode3, x).rgb - mode_shift) * voxelsize;
	if( abs(lambda4)>eps ) disp += s * lambda4 * mode_scale*(texture3D(warpmode4, x).rgb - mode_shift) * voxelsize;	
   #endif
   
  #else	
	// scale with inverse standard deviation 
	// = sqrt(eigenvalue)/norm(eigenvector) for principal warps	displacement
	vec3 disp = lambda0 * mode_scale*(texture3D( warpmode0, x + 0.1*warp_ofs ).rgb - mode_shift) * voxelsize;
  #endif // MULTIWARP
	
  #if MEANWARP == 1
	const float mean_scale = 20.0;
	const float mean_shift = 0.5;
	disp += (mean_scale*texture3D( meanwarptex, x ).rgb - mean_shift) * voxelsize;
  #endif
  
  	return disp;
}

//------------------------------------------------------------------------------
// Return simple negated field to approximate field
// (Displacement field setting)
vec3 get_inverse_warp( vec3 x )
{
	// Simple negation to approximate inverse
	return -get_warp( x );
}

//------------------------------------------------------------------------------
// Return vectorfield exponential via Midpoint algorithm 
// (Stationary velocity field setting)

#define M_PI 3.1415926535897932384626433832795

// Note that pade3() and expm() are not required here yet!

// Pade approximation to exp(A) for m=3
mat3 pade3( mat3 A )
{
	mat3 AA = A*A;
	mat3 AAA = AA*A;
	mat3 P = (1.0/120.0)*( mat3(1.0) + A ) + (1.0/240.0)*(AA/2.0) + (1.0/720.0)*(AAA/6.0);
	
	mat3 B=-A;
	mat3 BB=B*B;
	mat3 BBB=BB*B;
	mat3 Q = (1.0/120.0)*( mat3(1.0) + B ) + (1.0/240.0)*(BB/2.0) + (1.0/720.0)*(BBB/6.0);
	
	return P/Q;
}

// Scaling and squaring to integrate matrix logarithm M
mat3 expm( mat3 M )
{
	// Scaling
	const int N = 4;
	const float scaling = 1.0 / 16.0; // 1 / 2^N
	
	// Pade approximation
	M = pade3( scaling*M );	
	
	// Squaring N times
	M = M*M;
	M = M*M;
	M = M*M;
	M = M*M;
	
	return M;
}

//-----------------------------------------------------------------------------
// Reformation on Apodemus Flavicollis

float window( float edge0, float edge1, float delta, float x )
{	
	return smoothstep(edge0-delta,edge0+delta,x)*(1.0-smoothstep(edge1-delta,edge1+delta,x));
}

vec3 reform_flavi_ears( vec3 x )
{	
	// Local support
	float wz = window( 0.7,0.9,0.025, x.z );
	float wy = window( 0.5,0.9,0.125, x.y );	
	float w2 = wz*wy* smoothstep( 0.6, 1.0, x.x );
	float w1 = wz*wy* (1.0 - smoothstep( 0.0, 0.35, x.x ));		
	
	// Same rotation center for both trafos
	vec3 center = vec3(0.5,0.6,0.0);
	
	// Log affine trafos
	vec3 t = vec3(-0.3,0.0,0.0);
	mat4 L1= transpose(mat4(     
					   0.0,   -1.2217,      0.0,      t.x,
					1.2217,       0.0,      0.0,      t.y,
					   0.0,       0.0,      0.0,      t.z,
					   0.0,       0.0,      0.0,      0.0  ));
	mat4 L2= -L1;
	
	// Velocities
	vec3 v1 = L1[3].xyz + (L1*vec4(x-center,1.0)).xyz;
	vec3 v2 = L2[3].xyz + (L2*vec4(x-center,1.0)).xyz;
	
	// Blend velocities
	//return 0.3*lambda0*w1*(-v1) + 0.3*lambda1*w2*(-v2);
	return 0.9*w1*v1 + 0.6*w2*v2;	
}

vec3 reform_flavi_nose( vec3 x )
{
	vec3 center = vec3(0.4,0.2,0.1875);		
	float wz = window( -0.4,0.2,0.125, x.z );
	float logt = 1.5*0.872664625997165;
	mat4 L = transpose(mat4( // matrices are given column-major
					   0.0,       0.0,      0.0,      0.0,
					   0.0,       0.0,     logt,      0.0,
					   0.0,     -logt,      0.0,      0.0,
					   0.0,       0.0,      0.0,      0.0  ));
	vec3 v = (L*vec4(x-center,1.0)).xyz - L[3].xyz;
	//return 0.3*lambda0*wz*(-v); 
	return 0.3*wz*v; 
}

vec3 reform_flavi_teeth( vec3 x )
{
	vec3 center = vec3(0.5,0.3,0.5);
	vec3 t = vec3(0.0);
	mat4 L1= transpose(mat4(     
					   0.0,   -1.2217,      0.0,      t.x,
					1.2217,       0.0,      0.0,      t.y,
					   0.0,       0.0,      0.0,      t.z,
					   0.0,       0.0,      0.0,      0.0  ));
	mat4 L2= -L1;
	
	float wy = window(0.35,0.8,0.025,x.y),
	      wz = window(0.375,0.55,0.05,x.z),
	      w1 = wy*wz*window(0.19,0.5,0.025,x.x),
		  w2 = wy*wz*window(0.5,0.82,0.025,x.x);
	
	vec3 v1 = (L1*vec4(x-center,1.0)).xyz - L1[3].xyz,
	     v2 = (L2*vec4(x-center,1.0)).xyz - L2[3].xyz;
	
	return 0.4*(w1*v1 + w2*v2);
	//return 0.3*lambda0*w1*v1 + 0.3*lambda1*w2*v2;
}

vec3 get_warp2( vec3 x )
{
#define TEST 0
#if TEST==1
	// TESTING ON-THE-FLY GROUP MEAN SHAPE COMPUTATION
	
	float l = lambda0;
	
	// Sum v_i belonging not to group of interest
	lambda0=0.0;
	lambda1=0.0;
	lambda2=0.0;
	lambda3=0.0;
	lambda4=1.0;
	lambda5=1.0;
	lambda6=1.0;
	lambda7=1.0;
	lambda8=0.0;
	lambda9=0.0;
	lambda10=1.0;
	lambda11=1.0;
	lambda12=1.0;
	lambda13=0.0;
	lambda14=0.0;
	lambda15=1.0;
	vec3 va = (1.0/8.0)*get_warp( x );
	
	// Sum v_i belonging not to group of interest
	lambda0=1.0;
	lambda1=1.0;
	lambda2=1.0;
	lambda3=1.0;
	lambda4=0.0;
	lambda5=0.0;
	lambda6=0.0;
	lambda7=0.0;
	lambda8=1.0;
	lambda9=1.0;
	lambda10=0.0;
	lambda11=0.0;
	lambda12=0.0;
	lambda13=1.0;
	lambda14=1.0;
	lambda15=0.0;
	vec3 vb = (1.0/8.0)*get_warp( x );
	
	lambda0 = l;
	vec3 foo;
	if( l < 0 )
		foo = -l*va;
	else
		foo = l*vb;
	return foo;
#elif TEST==2
	// Log affine transformation (evaluated off-line)
    // Central rotation around x
	vec3 center = vec3(0.6,0.7,0.375);		
    float w = 1.0-smoothstep( 0.1, 0.24, length(x-center) );
	const vec3 t = vec3(0.0,-0.3272492,0.610865238);
	float logt = 1.5*lambda0*0.872664625997165;
	mat4 L = transpose(mat4( // matrices are given column-major
					   0.0,       0.0,      0.0,      0.0,
					   0.0,       0.0,     logt,      0.0,
					   0.0,     -logt,      0.0,      0.0,
					   0.0,       0.0,      0.0,      0.0  ));
	vec3 v = (L*vec4(x-center,1.0)).xyz - L[3].xyz;
	return w*(-v); // + get_warp(x);
#elif TEST==3
	return reform_flavi_teeth( x ) + reform_flavi_ears( x ) + reform_flavi_nose( x ) + get_warp( x );
#elif TEST==4
#else
	return get_warp( x );
#endif
}

vec3 integrate_RK4( vec3 x0, float sign, int steps )
{
	vec3 x = x0;
	
	// Stepsize
	float h = 1.0 / float(steps);
	
	// Runge-Kutta steps
	for( int k=0; k < steps; k++ )
	{
		vec3 k1 = h * sign*get_warp2( x );
		vec3 k2 = h * sign*get_warp2( x + 0.5*k1 );
		vec3 k3 = h * sign*get_warp2( x + 0.5*k2 );
		vec3 k4 = h * sign*get_warp2( x + k3 );
		x = x + k1/6.0 + k2/3.0 + k3/3.0 + k4/6.0;
	}	
	
	// Return displacement
	return (x-x0);	
}

//------------------------------------------------------------------------------
// Return vectorfield exponential via Midpoint algorithm 
// (Stationary velocity field setting)
vec3 integrate_midpoint( vec3 x0, float sign, int steps )
{
	vec3 x = x0;
	
	// Stepsize
	float h = 1.0 / float(steps);
	
	// Midpoint steps
	for( int k=0; k < steps; k++ )
	{
		vec3 k1 = h * sign*get_warp2( x );
		vec3 k2 = h * sign*get_warp2( x + 0.5*k1 );
		x = x + k2;
	}
	
	// Return displacement
	return (x-x0);	
}

//------------------------------------------------------------------------------
// Return vectorfield exponential via Euler-step algorithm 
// (Stationary velocity field setting)
vec3 integrate_euler( vec3 x0, float sign, int steps )
{
	vec3 x = x0;
	
	// Stepsize
	float h = 1.0 / float(steps);
	
	// Euler steps
	for( int k=0; k < steps; k++ )
	{
		x = x + h * sign*get_warp2( x );
	}
	
	// Return displacement
	return (x-x0);
}

//------------------------------------------------------------------------------
// Return inverse displacement field computed from velocity field
// (Stationary velocity field setting)
vec3 get_inverse_exp_warp( vec3 x )
{	
	// Vectorfield exponential results in deformation displacement
	// Initialize with -v to get inverse deformation
	int steps = INTEGRATOR_STEPS;
  #if   INTEGRATOR == 1
	return integrate_euler   ( x, -1.0, steps );
  #elif INTEGRATOR == 2
	return integrate_midpoint( x, -1.0, steps );
  #else 
	return integrate_RK4     ( x, -1.0, steps );
  #endif
}
#endif // WARP==1

//------------------------------------------------------------------------------
// Return scalar value for volume coordinate x
#if WARP == 1
vec3 get_inverse_displacement( vec3 x )
{
  #if INTEGRATOR == 0 // DISPLACEMENT VECTOR FIELD
	return get_inverse_warp( x );
  #else // STATIONARY VELOCITY FIELD
	return get_inverse_exp_warp( x );
  #endif
}
#endif

vec3 domain_transform( vec3 x )
{
	//return x;
	return 2.0*x - vec3(0.5);	
}

vec3 inverse_domain_transform( vec3 y )
{
	//return (y + vec3(0.5)) * 0.5;
	return y * 0.5;
}

float get_volume_scalar( vec3 x )
{
	x = domain_transform( x );
	
#if WARP == 1	
	// deform volume by offset texture
	vec3 d = get_inverse_displacement( x );
	x += d;
	g_displacement = inverse_domain_transform(d);
#endif
#if CHANNELS == 3
	// return magnitude of vectorfield entry
	return length( texture3D( voltex, x ).rgb );
#else
  #if FILTER == 1
	return texture3D( voltex, x ).w;
  #elif FILTER == 0	
	// simulate nearest neighbour filtering
	return texture3D( voltex, (floor(x*voxelsizei)+0.0)*voxelsize ).w;
  #endif
#endif
}

#if WARP == 1
//------------------------------------------------------------------------------
// Compute Jacobian by central differences

float det( mat3 A )
{
	// For recent GLSL versions:
	//   determinant(float)   since GLSL 1.50 
	//   determinant(double)  since GLSL 4.00
	// return determinant( J )
	
	// Sarrus rule
	return 
	  (A[0][0] + A[1][1] + A[2][2]) 
	+ (A[1][0] + A[2][1] + A[0][2]) 
	+ (A[2][0] + A[0][1] + A[1][2])
	- (A[0][2] + A[1][1] + A[2][0])
	- (A[1][2] + A[2][1] + A[0][0])
	- (A[2][2] + A[0][1] + A[1][0]);
}

mat3 get_jacobi_matrix( vec3 x )
{
	vec3 
		dx = vec3( voxelsize.x, 0.0, 0.0 ),
		dy = vec3( 0.0, voxelsize.y, 0.0 ),
		dz = vec3( 0.0, 0.0, voxelsize.z );
	
	vec3 hi = 1.0 / (2.0*voxelsize);
	
	mat3 J = transpose(mat3( 
		hi.x * get_inverse_displacement(x+dx) - get_inverse_displacement(x-dx),
		hi.y * get_inverse_displacement(x+dy) - get_inverse_displacement(x-dy),
		hi.z * get_inverse_displacement(x+dz) - get_inverse_displacement(x-dz) ));
		
	return J;
}

float get_jacobian( vec3 x )
{
	return det( get_jacobi_matrix(x) );
}
#endif // WARP == 1


//------------------------------------------------------------------------------
// Compute normal by central differences
vec3 get_normal( vec3 x )
{
  #if FILTER == 0
	// simulate nearest neighbour filtering
	x = (round(x*voxelsizei)-0.0)*voxelsize;
  #endif
	
	x = domain_transform(x);
	
	vec3 n = vec3(
	  texture3D(voltex,x - vec3(voxelsize.x,0,0)).w - texture3D(voltex,x + vec3(voxelsize.x,0,0)).w, 
	  texture3D(voltex,x - vec3(0,voxelsize.y,0)).w - texture3D(voltex,x + vec3(0,voxelsize.y,0)).w, 
	  texture3D(voltex,x - vec3(0,0,voxelsize.z)).w - texture3D(voltex,x + vec3(0,0,voxelsize.z)).w
	);
	
  #if 0 //WARP == 1
	mat3 J = get_jacobi_matrix( x );
	n = transpose(mat3(1.0)+J)*n;
  #endif
	
	return normalize(n);
}

//------------------------------------------------------------------------------
// Compute phong illumation (n==normal at x, eye==vector from x to viewer)
// TODO: check computation and constants, just copied from internet ;-)
float phong( vec3 n, vec3 eye, vec3 L )
{
	float I = 1.0;
	vec3 E = normalize( eye );

	vec3 R = reflect( -L, n );
	float diff = max( dot(L,n), 0.23 );
	float spec = 0.0;
	if( diff > 0.0 )
	{
		spec = max( dot(R,E), 0.7 );
		spec = pow( spec, 5.8 ); //1.8 ); //8.0 );
	}

	return mat_ka + mat_ks*spec + mat_kd*diff;
}

float phong( vec3 n, vec3 eye )
{
	return 0.7*phong( n, eye, 	normalize( eye ) ) + 0.3*phong( n, eye, light_pos );
	//vec3 L = light_pos + eye + vec3(-1,-0,-1);
}

//------------------------------------------------------------------------------
// Color code displacement
vec3 colorcode( vec3 disp )
{
	vec3 color = vec3(1.0);

#if WARP == 1	
	float disp_color_scale = 25.0 *1.75;
	
  #if   COLORMODE == 1	
	// encode warp strength in R channel
	float imp = length( disp*disp_color_scale );
	imp = clamp( imp, 0.0, 0.9 ); 
	color = vec3(.15+1.6*imp,0.8,1.3-imp);
	
  #elif COLORMODE == 2
	// project displacement vector onto surface normal
	// sign indicates if warp deforms surface in- or outwards
	float imp = dot( disp*disp_color_scale, g_normal );
	float imp_pos = clamp( imp, 0.0, 1.0 );
	float imp_neg = -clamp( imp, -1.0, 0.0 );
	// grey = no change
	// red  = outwards warp
	// blue = inwards warp	
	//vec3 cpos = mix(vec3(1.0),vec3(1.0,0.0,0.0),imp_pos);
	//vec3 cneg = mix(vec3(1.0),vec3(0.0,0.0,1.0),imp_pos);
	//color = mix( vec3(0.0,0.0,1.0), vec3(1.0,0.0,0.0), clamp(imp+0.5,0.0,1.0) );
	color = 2.0*vec3( .5+.5*(imp_neg-imp_pos), .5-.25*max(imp_pos,imp_neg), .5+.5*(imp_pos-imp_neg) );	
  #endif
#endif
	
	return color;
}

//------------------------------------------------------------------------------
// Convert fragment depth value to eye space z and vice versa
// where 
//   a = zFar / (zFar - zNear)
//   b = zFar*zNear / (zNear - zFar)
float z_to_depth( float z, float a, float b ) {	return a + b/z;	}
float depth_to_z( float d, float a, float b ) { return b * (1.0 / (d - a)); }

//------------------------------------------------------------------------------
void main(void)
{
	// ray start & end position
	vec3 ray_in  = texture2D( fronttex, tc ).xyz;
	vec3 ray_out = texture2D( backtex , tc ).xyz;

	// ray direction and traversal length
	vec3  dir = ray_out - ray_in;
	float len = length(dir)+0.001;
		  dir = dir / len;      // normalize direction

	// traversal step vector
	vec3 step = stepsize * dir;

	// initial ray position
	vec3 ray = vec3(0,0,0); //was: step

	// initial accumulated color and opacity
	vec4 dst = vec4(0,0,0,0);
	
#if MIP == 1
	float mip = 0.0;
	// HACK: alpha_scale not important for MIP?
	alpha_scale = 0.01;
#endif

	// ray traversal
	float samplingrate = 1.0/float(stepsize);
	int numsteps = int(samplingrate * 1.4142135); // *sqrt(2)
	for( int i=0; i < numsteps; ++i )
	{
		// ray termination
#ifdef MIDA_TEST
		if( length(ray) >= len )
		{
			break;
		}
#else
	#ifndef DEBUG
		if( length(ray) >= len || dst.a >= (1.0-0.0039) ) break; // 1/255=0.0039
	#else			
		if( length(ray) >= len ) 
		{ 
			dst = vec4(1,0,0,1);  // DEBUG break condition
			break; 
		}
		if( dst.a >= 0.99 ) 
		{ 
			dst.a = 1.0; 
			dst = vec4(0,1,0,1);  // DEBUG break condition
			break; 
		}
	#endif
#endif
		// Note: get_volume_scalar() implicitly computes g_displacement
		float intensity = get_volume_scalar( ray_in + ray );
		
		// Normal is required in every case
		g_normal = get_normal(ray_in+ray+g_displacement);		
		
		// Color coding of displacement (COLORMODE may require g_normal)
		vec3 color = colorcode( g_displacement );
		
#if ISOSURFACE == 1
		if( intensity > isovalue )
		{
			// intersection refinement
			float searchdir = -1.0;    // initial step is backwards
			vec3 ministep = step*.5;
			for( int j=0; j < refinement_steps; ++j )
			{
				// binary search
				ray += searchdir*ministep;
				
				intensity = get_volume_scalar( ray_in+ray );

				// move backwards until exact intersection overstepped
				// and vice versa
				if( intensity > isovalue )
					searchdir = -1.0;
				else
					searchdir = 1.0;

				// half stepsize
				ministep *= .5;
			}
			
			// update normal and color coding
			g_normal = get_normal(ray_in+ray+g_displacement);
			color = colorcode( g_displacement );
			
			// shading
			float li = phong( g_normal, -dir );
		
	#if SILHOUETTE == 1
			float edge = smoothstep( 0.5, 0.6, abs(dot(g_normal,-dir)) );
			dst.rgb = mix(vec3(0.0),vec3(1.0),edge);
	#else			
			dst.rgb = color*li;
	#endif
			dst.a   = 1.0;
			break;
		}
#else	
		// Transfer function
		vec4 src = transfer( intensity );
		
		// Consider color coding also for DVR
		src.rgb *= color;		

  #if MIP == 1
	 #ifdef MIDA_TEST
		// MIDA test
		float beta=1;
		if( intensity > mip )
		{
			beta = 1 - (intensity - mip);
			mip = intensity;
		}

		#ifdef LIGHTING
			float li = phong( g_normal, -dir );
		#else
			float li = 1.0;
		#endif

		// front-to-back compositing (w/ pre-multiplied alpha)
		if( src.a >= 0.0039 )
		{
			dst = dst + (1.0 - dst.a) * src;
		}
		// pre-multiplied alpha
		//dst.rgb = beta*src.rgb + (1 - beta*src.a) * intensity;
		//dst.a   = beta*src.a   + (1 - beta*src.a);
		//dst = beta*dst + (1 - beta*dst.a) * src;
		
	 #else
		// MIP
		float mipvalue = src.a; // was: intensity
		if(mipvalue > mip && src.a >= 0.0039) 
		{
			mip = mipvalue;
		#ifdef LIGHTING
			float li = phong( g_normal, -dir );
		#else
			float li = 1.0;
		#endif		
			dst = src*li;
			
			// was:	
			//dst.rgb = intensity*li;
			//dst.a   = sqrt(intensity)*(1+alpha_scale);  // hack (see break condition)
		}
	 #endif
  #else
	#ifdef LIGHTING
		float li = phong( g_normal, -dir );
		dst.rgb = dst.rgb + (1-dst.a)*src.rgb * li;
		dst.a   = dst.a   + (1-dst.a)*src.a;
	#else
		// front-to-back compositing (w/ pre-multiplied alpha)
		if( src.a >= 0.0039 )
		{
			dst = dst + (1.0 - dst.a) * src;
		}
		//dst = dst + pow((1.0 - dst.a),abs(get_jacobian(ray_in+ray+g_displacement))) * src;
	#endif // LIGHTING		
  #endif // MIP

#endif
		// advance ray position
		ray += step;
	}

	//------------------------------
	//  Output fragments
	//------------------------------	
	
#if NUMOUTPUTS >= 1
  #if      OUTPUT1 == 1		             // --- Output ray termination position
	gl_FragData[0] = vec4( ray_in + ray, dst.a );
  #elif    OUTPUT1 == 2	                 // --- Output normal
	gl_FragData[0] = vec4( g_normal    , dst.a );
  #elif    OUTPUT1 == 3	                 // --- Output ray start location
	gl_FragData[0] = vec4( ray_in      , dst.a );
  #elif    OUTPUT1 == 4	                 // --- Output ray end location
	gl_FragData[0] = vec4( ray_out     , dst.a );
  #else // OUTPUT1 == 0	                 // --- Output color
	gl_FragData[0] = vec4( dst.rgb     , dst.a );
  #endif
#endif
	
#if NUMOUTPUTS >= 2
  #if      OUTPUT2 == 1		             // --- Output ray termination position
	gl_FragData[1] = vec4( ray_in + ray, dst.a );
  #elif    OUTPUT2 == 2	                 // --- Output normal
	gl_FragData[1] = vec4( g_normal    , dst.a );
  #elif    OUTPUT2 == 3	                 // --- Output ray start location
	gl_FragData[1] = vec4( ray_in      , dst.a );
  #elif    OUTPUT2 == 4	                 // --- Output ray end location
	gl_FragData[1] = vec4( ray_out     , dst.a );
  #else // OUTPUT2 == 0	                 // --- Output color
	gl_FragData[1] = vec4( dst.rgb     , dst.a );
  #endif
#endif

#if NUMOUTPUTS >= 3
  #if      OUTPUT3 == 1		             // --- Output ray termination position
	gl_FragData[2] = vec4( ray_in + ray, dst.a );
  #elif    OUTPUT3 == 2	                 // --- Output normal
	gl_FragData[2] = vec4( g_normal    , dst.a );
  #elif    OUTPUT3 == 3	                 // --- Output ray start location
	gl_FragData[2] = vec4( ray_in      , dst.a );
  #elif    OUTPUT3 == 4	                 // --- Output ray end location
	gl_FragData[2] = vec4( ray_out     , dst.a );
  #else // OUTPUT3 == 0	                 // --- Output color
	gl_FragData[2] = vec4( dst.rgb     , dst.a );
  #endif
#endif

#if NUMOUTPUTS >= 4
  #if      OUTPUT4 == 1		             // --- Output ray termination position
	gl_FragData[3] = vec4( ray_in + ray, dst.a );
  #elif    OUTPUT4 == 2	                 // --- Output normal
	gl_FragData[3] = vec4( g_normal    , dst.a );
  #elif    OUTPUT4 == 3	                 // --- Output ray start location
	gl_FragData[3] = vec4( ray_in      , dst.a );
  #elif    OUTPUT4 == 4	                 // --- Output ray end location
	gl_FragData[3] = vec4( ray_out     , dst.a );
  #else // OUTPUT4 == 0	                 // --- Output color
	gl_FragData[3] = vec4( dst.rgb     , dst.a );
  #endif
#endif

#if NUMOUTPUTS >= 5
  #if      OUTPUT5 == 1		             // --- Output ray termination position
	gl_FragData[4] = vec4( ray_in + ray, dst.a );
  #elif    OUTPUT5 == 2	                 // --- Output normal
	gl_FragData[4] = vec4( g_normal    , dst.a );
  #elif    OUTPUT5 == 3	                 // --- Output ray start location
	gl_FragData[4] = vec4( ray_in      , dst.a );
  #elif    OUTPUT5 == 4	                 // --- Output ray end location
	gl_FragData[4] = vec4( ray_out     , dst.a );
  #else // OUTPUT5 == 0	                 // --- Output color
	gl_FragData[4] = vec4( dst.rgb     , dst.a );
  #endif
#endif


	//------------------------------	
	//  Debugging
	//------------------------------
	//gl_FragData[0] = vec4( tc.x ); 
	//gl_FragData[0] = texture1D( luttex, tc.x );
	

	//------------------------------
	//  Depth value (not available yet)
	//------------------------------
#if 0	
  #if DEPTHTEX==1	
	gl_FragColor = dst; 
		//was: gl_FragCoord.z + 0.5*length(ray); 
		//was: texture2D( depthtex, tc ).w;
  #endif
	//gl_FragDepth = FIXME: For depth value we only need depth of ray_start.
	//                      Via R2T its no problem to also render depthbuffer of front
	//                      facing volume bounding geometry to texture and pass it to
	//                      this shader!
	//gl_FragColor = vec4(color.rgb * len * 0.68, 1); // DEBUG ray length (scale by 1/sqrt(3))
	//gl_FragColor = vec4(1,tc.s,tc.t,1);             // DEBUG texture coordinates
#endif
}
