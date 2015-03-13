// slvis streamline - vertex shader
#version 150
#extension GL_ARB_explicit_attrib_location : enable

uniform mat4 Modelview;
uniform mat4 Projection;

layout(location=0) in vec3 Position;
layout(location=1) in vec3 Normal;

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
