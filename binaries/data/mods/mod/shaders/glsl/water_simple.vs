#version 110

#include "water_simple.h"

#include "common/los_vertex.h"
#include "common/vertex.h"

VERTEX_INPUT_ATTRIBUTE(0, vec3, a_vertex);

void main()
{
	// Shift the texture coordinates by these amounts to make the water "flow"
	float tx = -mod(time, 81.0) / 81.0;
	float tz = -mod(time, 34.0) / 34.0;
	float repeatPeriod = 16.0;

	v_coords = a_vertex.xz / repeatPeriod + vec2(tx, tz);
	v_los = calculateLOSCoordinates(a_vertex.xz, losTransform);
	OUTPUT_VERTEX_POSITION(transform * vec4(a_vertex, 1.0));
}
