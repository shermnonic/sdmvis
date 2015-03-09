// slvis streamline - geometry shader
#version 150

layout(points) in;
layout(points, max_vertices=1) out;

//uniform sampler3D voltex;
//uniform sampler3D warpfield;
//uniform float     isovalue;

void main()
{
	gl_Position = gl_in[0].gl_Position;
	EmitVertex();
	EndPrimitive();	
}
