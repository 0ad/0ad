#version 110

#include "common/vertex.h"

uniform mat4 transform;

varying vec2 v_tex;

VERTEX_INPUT_ATTRIBUTE(0, vec3, a_vertex);
VERTEX_INPUT_ATTRIBUTE(1, vec2, a_uv0);

void main()
{
	OUTPUT_VERTEX_POSITION(transform * vec4(a_vertex, 1.0));
	v_tex = a_uv0;
}
