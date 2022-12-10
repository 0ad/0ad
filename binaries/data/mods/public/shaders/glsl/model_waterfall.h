#include "common/shadows.h"
#include "common/stage.h"

BEGIN_DRAW_TEXTURES
	TEXTURE_2D(0, baseTex)
#if !IGNORE_LOS
	TEXTURE_2D(1, losTex)
#endif
#if USE_SHADOW
	SHADOWS_TEXTURES(2)
#endif
END_DRAW_TEXTURES

BEGIN_DRAW_UNIFORMS
	UNIFORM(mat4, instancingTransform)
END_DRAW_UNIFORMS

BEGIN_MATERIAL_UNIFORMS
	UNIFORM(vec3, shadingColor)
	UNIFORM(vec3, ambient)

	UNIFORM(float, specularPower)
	UNIFORM(vec3, specularColor)

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

VERTEX_OUTPUT(0, vec4, v_tex);
VERTEX_OUTPUT(1, vec3, v_half);
VERTEX_OUTPUT(2, vec3, v_normal);
VERTEX_OUTPUT(3, float, v_transp);
VERTEX_OUTPUT(4, vec3, v_lighting);
#if !IGNORE_LOS
VERTEX_OUTPUT(5, vec2, v_los);
#endif
#if USE_SHADOW
SHADOWS_VERTEX_OUTPUTS(6)
#endif
