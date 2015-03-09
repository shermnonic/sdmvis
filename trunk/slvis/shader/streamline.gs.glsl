// slvis streamline - geometry shader
#version 150

layout(points) in;
layout(line_strip, max_vertices=5) out;

//uniform sampler3D voltex;
uniform sampler3D warpfield;
//uniform float     isovalue;

void main()
{
	//gl_Position = gl_in[0].gl_Position;
	//EmitVertex();
	
	int i;
	for( i=0; i < 5; i++ )
	{	
		float d = float(i) / 4.0;
		gl_Position = gl_in[0].gl_Position + 20.0*d*vec4(1.0,1.0,1.0,0.0);
		EmitVertex();
	}
	
	EndPrimitive();	
}
