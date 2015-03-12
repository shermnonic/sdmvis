// slvis streamline - geometry shader
#version 150

//------------------------------------------------------------------------------
//  Config
//------------------------------------------------------------------------------
// Integrators
// 0 - Euler, 1 step
// 1 - Euler, 5 steps
// 2 - RK4, 4 steps
#define INTEGRATOR 2

// Projection
// 0 - None
// 1 - Surface parallel
// 2 - Surface parallel (Newton iter. to find closest isosurface)
#define PROJECTION 0

// Mode
// 0 - Streamline integration, input: seed pts, output: trajectory as line strip
// 1 - Triangle displacement, input: tri, output: displaced tri
#define MODE 1

//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
#if   MODE==0
layout(points) in;
layout(line_strip, max_vertices=6) out;
#elif MODE==1
layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;
#endif // MODE

in VertexAttrib
{
	vec4 color;
	vec3 normal;
} vertex[];

out vec4 vertex_color;
out vec3 vertex_normal;

uniform mat4 Modelview;
uniform mat4 Projection;

uniform sampler3D warpfield;
uniform sampler3D voltex;
uniform float     isovalue;
uniform float     displacement;

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
	return texture(voltex,x).r;
}

float get_scalar_gradient( vec3 x, vec3 dir )
{
	x *= voxelsize;	// Convert position to [0,1] texture coordinates	
	dir *= 1.5*voxelsize; // Scale normalized direction to length of one voxel
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
		if( abs(df) < 0.01 )
			break;
		x -= 0.1*(f / df);
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

// Return displacement vector for position x (after velocity integration)
vec3 integrate( vec3 x, float time )
{
	vec3 disp;
  #if   INTEGRATOR == 2
	disp = integrate_RK4( x, time, 4 ); // 4 RK4 steps
  #elif INTEGRATOR == 1
	disp = integrate_Euler( x, time, 5 ); // 5 Euler steps
  #else
	disp = get_warp(x); // 1 Euler step	
  #endif	
	return disp;
}


//------------------------------------------------------------------------------
//  Main
//------------------------------------------------------------------------------

// Main function for streamline tracing
void trace_streamline()
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
		disp = integrate(x,1.0);
		disp *= 5.0*0.2; // 1 / numSteps
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
		vertex_normal = vertex[0].normal;
		vertex_color = get_color( float(i+1)/5.0 );
			//vec4(abs(n),1.0);
			//vec4(vec3(get_scalar(px)*8.0),1.0);
		EmitVertex();		
	}		

	EndPrimitive();	
}

// Main function for mesh warping
void displace_triangle()
{
	mat4 MVP = Projection*Modelview;
	
	// Displace triangle vertices
	vec3 x;
	vec3 disp;
	for( int i=0; i < gl_in.length(); ++i )
	{
		// Integrate
		x = gl_in[i].gl_Position.xyz;		
		disp = integrate(x,1.0);
		x += disp;
		
		// Emit vertex
		gl_Position   = MVP*vec4(x,1.0);
		vertex_normal = vertex[i].normal;
		vertex_color  = vec4(0.7*vec3(abs(dot(vertex_normal,vec3(1.0,1.0,1.0)))),1.0);
			//vec4( vertex_normal, 1.0 );
			//vec4( voxelsize*gl_in[i].gl_Position.xyz, 1.0 );	
		EmitVertex();
	}
	EndPrimitive();	
}

// Geometry shader entry point
void main()
{
  #if   MODE==0
	trace_streamline();
  #elif MODE==1
	displace_triangle();
  #endif
}
