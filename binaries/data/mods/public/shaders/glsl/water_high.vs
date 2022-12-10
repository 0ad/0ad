#version 110

#include "water_high.h"

#include "common/los_vertex.h"
#include "common/shadows_vertex.h"
#include "common/vertex.h"

VERTEX_INPUT_ATTRIBUTE(0, vec3, a_vertex);
VERTEX_INPUT_ATTRIBUTE(1, vec2, a_waterInfo);

void main()
{
	worldPos = a_vertex;
	waterInfo = a_waterInfo;
	waterDepth = a_waterInfo.g;

	windCosSin = vec2(cos(-windAngle), sin(-windAngle));

	float newX = a_vertex.x * windCosSin.x - a_vertex.z * windCosSin.y;
	float newY = a_vertex.x * windCosSin.y + a_vertex.z * windCosSin.x;

	normalCoords = vec4(newX, newY, time, 0.0);
	normalCoords.xy *= repeatScale;
	// Projective texturing
#if USE_REFLECTION
	reflectionCoords = (reflectionMatrix * vec4(a_vertex, 1.0)).rga;
#endif
#if USE_REFRACTION
	refractionCoords = (refractionMatrix * vec4(a_vertex, 1.0)).rga;
#endif

#if !IGNORE_LOS
	v_los = calculateLOSCoordinates(a_vertex.xz, losTransform);
#endif

	calculatePositionInShadowSpace(vec4(a_vertex, 1.0));

	v_eyeVec = normalize(cameraPos - worldPos);

	moddedTime = mod(time * 60.0, 8.0) / 8.0;

	// Fix the waviness for local wind strength
	fwaviness = waviness * (0.15 + a_waterInfo.r / 1.15);

	OUTPUT_VERTEX_POSITION(transform * vec4(a_vertex, 1.0));
}
