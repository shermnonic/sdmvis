// slvis streamline - vertex shader
#version 150

uniform mat4 Modelview;
uniform mat4 Projection;

in vec3 Position;
in vec3 Normal;

out VertexAttrib
{
	vec4 color;
	vec3 normal;
} vertex;

void main()
{
	// Points will be transformed in geometry shader
	gl_Position = vec4(Position,1.0);	
	
	// Attributes
	vertex.color = vec4(1.0); // Dummy color, is replaced in geometry shader	
	vertex.normal = Normal;
}
