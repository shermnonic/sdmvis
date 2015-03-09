// slvis streamline - geometry shader
#version 150

layout(points) in;
layout(line_strip, max_vertices=2) out;

uniform mat4 Modelview;
uniform mat4 Projection;

//uniform sampler3D voltex;
//uniform float     isovalue;
uniform sampler3D warpfield;
const vec3 voxelsize = vec3( 1.0/200.0, 1.0/200.0, 1.0/400.0 );

void main()
{
	mat4 MVP = Projection*Modelview;
	vec3 x = voxelsize * gl_in[0].gl_Position.xyz;
	
	gl_Position = MVP*gl_in[0].gl_Position;
	EmitVertex();
	
	vec3 disp = texture(warpfield,x).xyz + vec3(0.1);
	gl_Position = MVP*(gl_in[0].gl_Position + 2.5*vec4(disp,0.0));
	EmitVertex();
	
/*
	int i;
	for( i=0; i < 5; i++ )
	{	
		float d = float(i) / 4.0;
		gl_Position = gl_in[0].gl_Position + 20.0*d*vec4(1.0,1.0,1.0,0.0);
		EmitVertex();
	}
*/	
	EndPrimitive();	
}
