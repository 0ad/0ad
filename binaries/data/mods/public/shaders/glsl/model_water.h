#include "common/shadows.h"
#include "common/stage.h"

BEGIN_DRAW_TEXTURES
	TEXTURE_2D(0, waterTex)
	TEXTURE_CUBE(1, skyCube)
#if !IGNORE_LOS
	TEXTURE_2D(2, losTex)
#endif
#if USE_SHADOW
	SHADOWS_TEXTURES(3)
#endif
END_DRAW_TEXTURES

BEGIN_DRAW_UNIFORMS
	UNIFORM(mat4, instancingTransform)
END_DRAW_UNIFORMS

BEGIN_MATERIAL_UNIFORMS
	UNIFORM(vec3, shadingColor)
	UNIFORM(vec3, ambient)

	UNIFORM(float, specularStrength)
	UNIFORM(float, waviness)
	UNIFORM(vec3, waterTint)
	UNIFORM(float, murkiness)
	UNIFORM(vec3, reflectionTint)
	UNIFORM(float, reflectionTintStrength)

	UNIFORM(mat4, transform)
	UNIFORM(vec3, cameraPos)
	UNIFORM(vec3, sunDir)
	UNIFORM(vec3, sunColor)

	UNIFORM(float, sim_time)
	UNIFORM(vec2, translation)

	UNIFORM(vec3, fogColor)
	UNIFORM(vec2, fogParams)
	UNIFORM(vec2, losTransform)
#if USE_SHADOW
	SHADOWS_UNIFORMS
#endif
END_MATERIAL_UNIFORMS

VERTEX_OUTPUT(0, vec4, worldPos);
VERTEX_OUTPUT(1, vec4, v_tex);
#if !IGNORE_LOS
VERTEX_OUTPUT(2, vec2, v_los);
#endif
#if USE_SHADOW
SHADOWS_VERTEX_OUTPUTS(3)
#endif
