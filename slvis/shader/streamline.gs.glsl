// slvis streamline - geometry shader
#version 150

//------------------------------------------------------------------------------
//  Config
//------------------------------------------------------------------------------
// Integrators
// 0 - Euler, 1 step
// 1 - Euler, 5 steps
// 2 - RK4, 4 steps
#define INTEGRATOR 0

// Projection
// 0 - None
// 1 - Surface parallel
// 2 - Surface parallel (Newton iter. to find closest isosurface)
#define PROJECTION 1

//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
layout(points) in;
layout(line_strip, max_vertices=6) out;

in VertexAttrib
{
	vec4 color;
} vertex[];

out vec4 vertex_color;

uniform mat4 Modelview;
uniform mat4 Projection;

uniform sampler3D warpfield;
uniform sampler3D voltex;
uniform float     isovalue;

// FIXME: voxelsize also serves here as domain transform!
const vec3 voxelsize = vec3( 1.0/200.0, 1.0/200.0, 1.0/400.0 );

//------------------------------------------------------------------------------
//  Scalar volume functions
//------------------------------------------------------------------------------
// Compute normal by central differences
vec3 get_normal( vec3 x )
{		
	x *= voxelsize;  // Convert to [0,1] texture coordinates	
	vec3 n = vec3(
	  texture(voltex,x - vec3(voxelsize.x,0,0)).r - texture(voltex,x + vec3(voxelsize.x,0,0)).r, 
	  texture(voltex,x - vec3(0,voxelsize.y,0)).r - texture(voltex,x + vec3(0,voxelsize.y,0)).r, 
	  texture(voltex,x - vec3(0,0,voxelsize.z)).r - texture(voltex,x + vec3(0,0,voxelsize.z)).r
	);	
	return normalize(n);
}

float get_scalar( vec3 x )
{
	x *= voxelsize;
	return texture(voltex,x).xyz;
}

float get_scalar_gradient( vec3 x, vec3 dir )
{
	x *= voxelsize;	// Convert position to [0,1] texture coordinates	
	dir *= voxelsize; // Scale normalized direction to length of one voxel
	return 0.5*(texture(voltex,x-dir).r - texture(voltex,x+dir).r);
}

// Returns point projected onto closest isosurface in given search direction
vec3 project_to_isosurface( vec3 x, vec3 searchdir, float iso )
{
	// Newton iteration
	float f, df;
	int i;
	for( i=0; i < 3; i++ ) // Use a fixed number of steps to avoid branching
	{
		f  = get_scalar( x );
		df = get_scalar_gradient( x, searchdir );
		x -= f / df;
	}
	return x;
}

//------------------------------------------------------------------------------
//  Vector field functions
//------------------------------------------------------------------------------
// Return vectorfield at world space coordinates
// Requires: warpfield, voxelsize
vec3 get_warp( vec3 x )
{
	x *= voxelsize;  // Convert to [0,1] texture coordinates
	return texture(warpfield,x).xyz;
}

// Return vectorfield exponential via Runge-Kutta(4) algorithm 
// Requires: get_warp()
vec3 integrate_RK4( vec3 x0, float sign, int steps )
{
	vec3 x = x0;
	
	// Stepsize
	float h = 1.0 / float(steps);
	
	// Runge-Kutta steps
	for( int k=0; k < steps; k++ )
	{
		vec3 k1 = h * sign*get_warp( x );
		vec3 k2 = h * sign*get_warp( x + 0.5*k1 );
		vec3 k3 = h * sign*get_warp( x + 0.5*k2 );
		vec3 k4 = h * sign*get_warp( x + k3 );
		x = x + k1/6.0 + k2/3.0 + k3/3.0 + k4/6.0;
	}	
	
	// Return displacement
	return (x-x0);	
}

// Return vectorfield exponential via Euler-step algorithm 
// Requires: get_warp()
vec3 integrate_Euler( vec3 x0, float sign, int steps )
{
	vec3 x = x0;
	
	// Stepsize
	float h = 1.0 / float(steps);
	
	// Euler steps
	for( int k=0; k < steps; k++ )
	{
		x = x + h * sign*get_warp( x );
	}
	
	// Return displacement
	return (x-x0);
}

//------------------------------------------------------------------------------
//  Utility functions
//------------------------------------------------------------------------------
// Transfer function blue-to-yellow
vec4 get_color( float t )
{
	const vec3 colA = vec3(0.0,0.0,1.0);
	const vec3 colB = vec3(1.0,1.0,0.0);
	vec3 col = mix( colA, colB, t );
	return vec4( col, 1.0 );
}

//------------------------------------------------------------------------------
//  Main
//------------------------------------------------------------------------------
// Geometry shader entry point
void main()
{
	mat4 MVP = Projection*Modelview;
	vec4 p0 = gl_in[0].gl_Position;	
	
	// Seed point
	gl_Position = MVP*p0;
	vertex_color = get_color(0.0); // vertex[0].color;
	EmitVertex();
	
	// Trace streamline
	vec3 x = p0.xyz;   // Position on streamline
	vec3 disp;         // Displacement
	vec3 px = x;       // Projected position (e.g. on surface)
	vec3 n;            // Normal (for projection)
	int i;	
	for( i=0; i < 5; i++ )
	{		
		// Integration
	  #if   INTEGRATOR == 2
		disp = integrate_RK4( x, 1.0, 4 ); // 4 RK4 steps
	  #elif INTEGRATOR == 1
		disp = integrate_Euler( x, 1.0, 5 ); // 5 Euler steps
	  #else
		disp = 0.25*get_warp(x); // 1 Euler step	
	  #endif		
		x += disp;
		
	  #if   PROJECTION == 1
		// Project onto surface
		n = get_normal( px );
		disp -= dot(n,disp)*n;
		px += disp;
	  #elif PROJECTION == 2
		n = get_normal( px );
		disp -= dot(n,disp)*n;
		px = project_to_isosurface( px+disp, -n, isovalue );
	  #else
		// No projection
		px = x;
	  #endif
		
		// Emit vertex
		gl_Position = MVP*vec4(px,1.0);
		vertex_color = get_color( float(i+1)/5.0 );
			//vec4(abs(n),1.0);
		EmitVertex();		
	}		

	EndPrimitive();	
}
