#version 110

#include "common/vertex.h"

uniform mat4 transform;

VERTEX_INPUT_ATTRIBUTE(0, vec3, a_vertex);

void main()
{
	OUTPUT_VERTEX_POSITION(transform * vec4(a_vertex, 1.0));
}
