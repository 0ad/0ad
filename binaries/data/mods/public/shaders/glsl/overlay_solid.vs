#version 120

uniform mat4 transform;
uniform mat4 instancingTransform;

attribute vec3 a_vertex;

void main()
{
	vec4 worldPos = instancingTransform * vec4(a_vertex, 1.0);
	gl_Position = transform * worldPos;
}
