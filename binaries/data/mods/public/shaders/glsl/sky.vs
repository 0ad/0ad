#version 110

#include "common/vertex.h"

varying vec3 v_tex;

VERTEX_INPUT_ATTRIBUTE(0, vec3, a_vertex);
VERTEX_INPUT_ATTRIBUTE(1, vec3, a_uv0);

uniform mat4 transform;

void main()
{
	OUTPUT_VERTEX_POSITION(transform * vec4(a_vertex, 1.0));
	v_tex = a_uv0.xyz;
}
