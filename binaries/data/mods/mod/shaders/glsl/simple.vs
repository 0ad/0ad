#version 110

#include "common/vertex.h"

varying vec2 v_tex;

attribute vec3 a_vertex;
attribute vec2 a_uv0;

void main()
{
	OUTPUT_VERTEX_POSITION(vec4(a_vertex, 1.0));

	v_tex = a_uv0;
}
