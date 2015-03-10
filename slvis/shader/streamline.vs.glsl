// slvis streamline - vertex shader
#version 150

uniform mat4 Modelview;
uniform mat4 Projection;

in vec3 Position;

out VertexAttrib
{
	vec4 color;
} vertex;

void main()
{
	// Points will be transformed in geometry shader
	gl_Position = vec4(Position,1.0);
	
	// Dummy color, will be replaced in geometry shader
	vertex.color = vec4(1.0);
}
