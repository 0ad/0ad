#include "common/shadows.h"
#include "common/stage.h"

BEGIN_DRAW_TEXTURES
	TEXTURE_CUBE(0, skyCube)
	TEXTURE_2D(1, normalMap)
	TEXTURE_2D(2, normalMap2)
#if USE_FANCY_EFFECTS
	TEXTURE_2D(3, waterEffectsTex)
#endif
#if USE_REFLECTION
	TEXTURE_2D(4, reflectionMap)
#endif
#if USE_REFRACTION
	TEXTURE_2D(5, refractionMap)
#if USE_REAL_DEPTH
	TEXTURE_2D(6, depthTex)
#endif
#endif
#if !IGNORE_LOS
	TEXTURE_2D(4, losTex)
#endif
#if USE_SHADOW
	SHADOWS_TEXTURES(5)
#endif
END_DRAW_TEXTURES

BEGIN_DRAW_UNIFORMS
	UNIFORM(mat4, transform)
END_DRAW_UNIFORMS

BEGIN_MATERIAL_UNIFORMS
	UNIFORM(mat4, reflectionMatrix)
	UNIFORM(mat4, refractionMatrix)
#if USE_REFRACTION && USE_REAL_DEPTH
	UNIFORM(mat4, projInvTransform)
	UNIFORM(mat4, viewInvTransform)
#endif
	UNIFORM(float, time)
	UNIFORM(float, repeatScale)
	UNIFORM(vec2, screenSize)
	UNIFORM(vec3, cameraPos)

	UNIFORM(float, waviness)			// "Wildness" of the reflections and refractions; choose based on texture
	UNIFORM(vec3, color)				// color of the water
	UNIFORM(vec3, tint)				// Tint for refraction (used to simulate particles in water)
	UNIFORM(float, murkiness)		// Amount of tint to blend in with the refracted color
	UNIFORM(float, windAngle)

	UNIFORM(vec4, waveParams1) // wavyEffect, BaseScale, Flattenism, Basebump
	UNIFORM(vec4, waveParams2) // Smallintensity, Smallbase, Bigmovement, Smallmovement

	// Environment settings
	UNIFORM(vec3, ambient)
	UNIFORM(vec3, sunDir)
	UNIFORM(vec3, sunColor)
	UNIFORM(mat4, skyBoxRot)

	UNIFORM(vec3, fogColor)
	UNIFORM(vec2, fogParams)
	UNIFORM(vec2, losTransform)
#if USE_SHADOW
	SHADOWS_UNIFORMS
#endif
END_MATERIAL_UNIFORMS

VERTEX_OUTPUT(0, vec3, worldPos);
VERTEX_OUTPUT(1, float, waterDepth);
VERTEX_OUTPUT(2, vec2, waterInfo);
VERTEX_OUTPUT(3, float, fwaviness);
VERTEX_OUTPUT(4, float, moddedTime);
VERTEX_OUTPUT(5, vec2, windCosSin);
VERTEX_OUTPUT(6, vec3, v_eyeVec);
VERTEX_OUTPUT(7, vec4, normalCoords);
#if USE_REFLECTION
VERTEX_OUTPUT(8, vec3, reflectionCoords);
#endif
#if USE_REFRACTION
VERTEX_OUTPUT(9, vec3, refractionCoords);
#endif
#if !IGNORE_LOS
VERTEX_OUTPUT(10, vec2, v_los);
#endif
#if USE_SHADOW
SHADOWS_VERTEX_OUTPUTS(11)
#endif
