// slvis streamline - vertex shader
#version 150

uniform mat4 Modelview;
uniform mat4 Projection;

in vec3 Position;

void main()
{
	// Points will be transformed in geometry shader
	gl_Position = vec4(Position,1.0);	
}
