#include "common/stage.h"

BEGIN_DRAW_TEXTURES
	TEXTURE_2D(0, baseTex)
#if !IGNORE_LOS
	TEXTURE_2D(1, losTex)
#endif
END_DRAW_TEXTURES

BEGIN_DRAW_UNIFORMS
	UNIFORM(mat4, transform)
	UNIFORM(float, time)
	UNIFORM(vec3, color)
END_DRAW_UNIFORMS

BEGIN_MATERIAL_UNIFORMS
	UNIFORM(vec3, fogColor)
	UNIFORM(vec2, fogParams)
	UNIFORM(vec2, losTransform)
END_MATERIAL_UNIFORMS

VERTEX_OUTPUT(0, vec2, v_coords);
#if !IGNORE_LOS
VERTEX_OUTPUT(1, vec2, v_los);
#endif