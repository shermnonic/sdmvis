// raycast - plain vertex shader
varying vec3 pos;
varying vec4 color;
varying vec2 tc;
void main(void)
{
	tc = gl_MultiTexCoord0.st;
	pos = gl_Vertex.xyz;
	color = gl_Color;
	gl_Position = ftransform();
}
	