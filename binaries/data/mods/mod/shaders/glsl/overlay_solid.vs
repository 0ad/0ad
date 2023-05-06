#version 120

#include "overlay_solid.h"

#include "common/vertex.h"

VERTEX_INPUT_ATTRIBUTE(0, vec3, a_vertex);

void main()
{
	vec4 worldPos = vec4(a_vertex * instancingTransform.w + instancingTransform.xyz, 1.0);
	OUTPUT_VERTEX_POSITION(transform * worldPos);
}
