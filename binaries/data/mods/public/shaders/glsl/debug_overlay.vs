#version 110

uniform mat4 transform;
uniform mat4 textureTransform;

attribute vec3 a_vertex;
attribute vec3 a_uv0;

varying vec2 v_tex;

void main()
{
	v_tex = (textureTransform * vec4(a_uv0, 1.0)).xy;
	gl_Position = transform * vec4(a_vertex, 1.0);
}
