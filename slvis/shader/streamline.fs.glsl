// slvis streamline - fragment shader
#version 120

in vec4 vertex_color;
in vec4 vertex_normal;

void main()
{
	gl_FragColor = vertex_color;
}
