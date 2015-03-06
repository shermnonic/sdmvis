// slvis streamline - geometry shader
#version 120
#extension GL_ARB_geometry_shader4 : enable

uniform sampler3D voltex;
uniform sampler3D warpfield;
float             isovalue;

void main()
{
	// Pass thru
	for( int i=0; i < gl_VerticesIn; i++ )
	{
		gl_Position = gl_PositionIn[i];
		EmitVertex();
	}
	EndPrimitive();
}
