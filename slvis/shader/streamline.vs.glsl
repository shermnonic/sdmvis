// slvis streamline - vertex shader
#version 150

uniform mat4 Modelview;
uniform mat4 Projection;

in vec3 Position;

void main()
{
	gl_Position = Projection*Modelview*vec4(Position,1.0);	
}
