#version 120

#include "model_waterfall.h"

#include "common/los_vertex.h"
#include "common/shadows_vertex.h"
#include "common/vertex.h"

VERTEX_INPUT_ATTRIBUTE(0, vec3, a_vertex);
VERTEX_INPUT_ATTRIBUTE(1, vec3, a_normal);
VERTEX_INPUT_ATTRIBUTE(2, vec2, a_uv0);
VERTEX_INPUT_ATTRIBUTE(3, vec2, a_uv1);

void main()
{
	vec4 worldPos = instancingTransform * vec4(a_vertex, 1.0);

	v_tex.xy = a_uv0 + sim_time * translation;
	v_transp = a_uv1.x;

	calculatePositionInShadowSpace(worldPos);

#if !IGNORE_LOS
	v_los = calculateLOSCoordinates(worldPos.xz, losTransform);
#endif

	vec3 eyeVec = cameraPos.xyz - worldPos.xyz;
	vec3 sunVec = -sunDir;
	v_half = normalize(sunVec + normalize(eyeVec));

	mat3 normalMatrix = mat3(instancingTransform[0].xyz, instancingTransform[1].xyz, instancingTransform[2].xyz);
	v_normal = normalMatrix * a_normal;
	v_lighting = max(0.0, dot(v_normal, -sunDir)) * sunColor;

	OUTPUT_VERTEX_POSITION(transform * worldPos);
}
