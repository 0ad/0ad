#version 110

attribute vec3 a_vertex;
attribute vec2 a_uv0;

uniform float mapSize;

void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * vec4(a_vertex, 1.0);
	gl_TexCoord[0].st = a_uv0;
	gl_TexCoord[0].zw = vec2(a_vertex.xz)/mapSize;
}
