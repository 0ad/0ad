#version 110

#include "common/vertex.h"

VERTEX_INPUT_ATTRIBUTE(0, vec3, a_vertex);

uniform mat4 transform;

void main()
{
	OUTPUT_VERTEX_POSITION(transform * vec4(a_vertex, 1.0));
}
