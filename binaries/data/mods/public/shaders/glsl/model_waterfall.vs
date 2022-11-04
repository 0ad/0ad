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

attribute vec3 a_vertex;
attribute vec3 a_normal;
attribute vec2 a_uv0;
attribute vec2 a_uv1;

varying vec4 worldPos;
varying vec4 v_tex;
varying vec3 v_half;
varying vec3 v_normal;
varying float v_transp;
varying vec3 v_lighting;

void main()
{
	worldPos = instancingTransform * vec4(a_vertex, 1.0);

	v_tex.xy = a_uv0 + sim_time * translation;
	v_transp = a_uv1.x;

	calculatePositionInShadowSpace(worldPos);

	calculateLOSCoordinates(worldPos.xz);

	vec3 eyeVec = cameraPos.xyz - worldPos.xyz;
	vec3 sunVec = -sunDir;
	v_half = normalize(sunVec + normalize(eyeVec));

	mat3 normalMatrix = mat3(instancingTransform[0].xyz, instancingTransform[1].xyz, instancingTransform[2].xyz);
	v_normal = normalMatrix * a_normal;
	v_lighting = max(0.0, dot(v_normal, -sunDir)) * sunColor;

	OUTPUT_VERTEX_POSITION(transform * worldPos);
}
