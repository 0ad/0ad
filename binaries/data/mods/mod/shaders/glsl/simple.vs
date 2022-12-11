#version 110

#include "common/stage.h"
#include "common/vertex.h"

VERTEX_INPUT_ATTRIBUTE(0, vec3, a_vertex);
VERTEX_INPUT_ATTRIBUTE(1, vec2, a_uv0);

VERTEX_OUTPUT(0, vec2, v_tex);

void main()
{
	OUTPUT_VERTEX_POSITION(vec4(a_vertex, 1.0));

	v_tex = a_uv0;
}
