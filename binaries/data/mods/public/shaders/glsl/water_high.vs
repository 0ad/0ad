#version 110

#include "common/los_vertex.h"
#include "common/shadows_vertex.h"

uniform mat4 reflectionMatrix;
uniform mat4 refractionMatrix;
uniform float repeatScale;
uniform float windAngle;
// "Wildness" of the reflections and refractions; choose based on texture
uniform float waviness;
uniform vec3 sunDir;

uniform float time;

uniform mat4 transform;
uniform vec3 cameraPos;

varying float moddedTime;

varying vec3 worldPos;
varying float waterDepth;
varying vec2 waterInfo;

varying vec3 v_eyeVec;

varying vec4 normalCoords;
#if USE_REFLECTION
varying vec3 reflectionCoords;
#endif
#if USE_REFRACTION
varying vec3 refractionCoords;
#endif

varying float fwaviness;
varying vec2 WindCosSin;

attribute vec3 a_vertex;
attribute vec2 a_waterInfo;
attribute vec3 a_otherPosition;

void main()
{
	worldPos = a_vertex;
	waterInfo = a_waterInfo;
	waterDepth = a_waterInfo.g;

	WindCosSin = vec2(cos(-windAngle), sin(-windAngle));

	float newX = a_vertex.x * WindCosSin.x - a_vertex.z * WindCosSin.y;
	float newY = a_vertex.x * WindCosSin.y + a_vertex.z * WindCosSin.x;

	normalCoords = vec4(newX, newY, time, 0.0);
	normalCoords.xy *= repeatScale;
	// Projective texturing
#if USE_REFLECTION
	reflectionCoords = (reflectionMatrix * vec4(a_vertex, 1.0)).rga;
#endif
#if USE_REFRACTION
	refractionCoords = (refractionMatrix * vec4(a_vertex, 1.0)).rga;
#endif
	calculateLOSCoordinates(a_vertex.xz);

	calculatePositionInShadowSpace(vec4(a_vertex, 1.0));

	v_eyeVec = normalize(cameraPos - worldPos);

	moddedTime = mod(time * 60.0, 8.0) / 8.0;

	// Fix the waviness for local wind strength
	fwaviness = waviness * (0.15 + a_waterInfo.r / 1.15);

	gl_Position = transform * vec4(a_vertex, 1.0);
}
