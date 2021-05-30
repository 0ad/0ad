#version 110

uniform mat4 transform;

attribute vec2 a_vertex;
attribute vec2 a_uv0;

varying vec2 v_uv;

void main()
{
	v_uv = a_uv0;
	gl_Position = transform * vec4(a_vertex, 0.0, 1.0);
}
