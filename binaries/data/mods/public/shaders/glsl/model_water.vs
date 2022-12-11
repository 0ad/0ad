#version 120

#include "model_water.h"

#include "common/los_vertex.h"
#include "common/shadows_vertex.h"
#include "common/vertex.h"

VERTEX_INPUT_ATTRIBUTE(0, vec3, a_vertex);
VERTEX_INPUT_ATTRIBUTE(2, vec2, a_uv0);

vec4 fakeCos(vec4 x)
{
	vec4 tri = abs(fract(x + 0.5) * 2.0 - 1.0);
	return tri * tri *(3.0 - 2.0 * tri);
}

void main()
{
	worldPos = instancingTransform * vec4(a_vertex, 1.0);

	v_tex.xy = (a_uv0 + worldPos.xz) / 5.0 + sim_time * translation;

	v_tex.zw = a_uv0;

	calculatePositionInShadowSpace(worldPos);

#if !IGNORE_LOS
	v_los = calculateLOSCoordinates(worldPos.xz, losTransform);
#endif

	OUTPUT_VERTEX_POSITION(transform * worldPos);
}
