// slvis streamline - vertex shader
#version 110

varying vec3 pos;
varying vec4 color;
varying vec2 tc;

void main()
{
	tc = gl_MultiTexCoord0.st;
	pos = gl_Vertex.xyz;
	color = gl_Color;
	gl_Position = ftransform();
}
