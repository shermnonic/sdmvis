// slvis streamline - geometry shader
#version 150

layout(points) in;
layout(line_strip, max_vertices=6) out;

uniform mat4 Modelview;
uniform mat4 Projection;

//uniform sampler3D voltex;
//uniform float     isovalue;
uniform sampler3D warpfield;
const vec3 voxelsize = vec3( 1.0/200.0, 1.0/200.0, 1.0/400.0 );

// Return vectorfield at world space coordinates
// Requires: warpfield, voxelsize
vec3 get_warp( vec3 x )
{
	// Convert to [0,1] texture coordinates
	x *= voxelsize;
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

// Geometry shader entry point
void main()
{
	mat4 MVP = Projection*Modelview;
	vec4 p0 = gl_in[0].gl_Position;	
	
	// Seed point
	gl_Position = MVP*p0;
	EmitVertex();
	
	// Trace streamline
	int i;
	vec3 x = p0.xyz;
	vec3 disp;
	for( i=0; i < 5; i++ )
	{	
		disp = get_warp(x); // 1 Euler step
		//disp = integrate_Euler( x, 1.0, 5 ); // 5 Euler steps
		//disp = integrate_RK4( x, 1.0, 4 ); // 4 RK4 steps
		x += disp;
		gl_Position = MVP*vec4(x,1.0);
		EmitVertex();		
	}		

	EndPrimitive();	
}
