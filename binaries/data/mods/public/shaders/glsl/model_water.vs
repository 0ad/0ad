#version 120

#include "common/los_vertex.h"
#include "common/shadows_vertex.h"
#include "common/vertex.h"

uniform mat4 transform;
uniform vec3 cameraPos;
uniform vec3 sunDir;
uniform vec3 sunColor;
uniform mat4 instancingTransform;

uniform float sim_time;
uniform vec2 translation;

VERTEX_INPUT_ATTRIBUTE(0, vec3, a_vertex);
VERTEX_INPUT_ATTRIBUTE(1, vec3, a_normal);
VERTEX_INPUT_ATTRIBUTE(2, vec2, a_uv0);
VERTEX_INPUT_ATTRIBUTE(3, vec2, a_uv1);

varying vec4 worldPos;
varying vec4 v_tex;

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

	calculateLOSCoordinates(worldPos.xz);

	OUTPUT_VERTEX_POSITION(transform * worldPos);
}
