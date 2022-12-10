#version 110

#include "canvas2d.h"

#include "common/vertex.h"

VERTEX_INPUT_ATTRIBUTE(0, vec2, a_vertex);
VERTEX_INPUT_ATTRIBUTE(1, vec2, a_uv0);

void main()
{
	v_uv = a_uv0;
	OUTPUT_VERTEX_POSITION(vec4(mat2(transform.xy, transform.zw) * a_vertex + translation, 0.0, 1.0));
}
