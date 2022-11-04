#version 110

#include "common/vertex.h"

uniform vec4 transform;
uniform vec2 translation;

attribute vec2 a_vertex;
attribute vec2 a_uv0;

varying vec2 v_uv;

void main()
{
	v_uv = a_uv0;
	OUTPUT_VERTEX_POSITION(vec4(mat2(transform.xy, transform.zw) * a_vertex + translation, 0.0, 1.0));
}
