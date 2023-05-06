#include "common/stage.h"

BEGIN_DRAW_TEXTURES
	TEXTURE_2D(0, baseTex)
#if !IGNORE_LOS
	TEXTURE_2D(1, losTex)
#endif
END_DRAW_TEXTURES

BEGIN_DRAW_UNIFORMS
	UNIFORM(mat4, modelViewMatrix)
END_DRAW_UNIFORMS

BEGIN_MATERIAL_UNIFORMS
	UNIFORM(mat4, transform)
	UNIFORM(vec3, sunColor)
	UNIFORM(vec3, fogColor)
	UNIFORM(vec2, fogParams)
	UNIFORM(vec2, losTransform)
END_MATERIAL_UNIFORMS

VERTEX_OUTPUT(0, vec2, v_tex);
VERTEX_OUTPUT(1, vec4, v_color);

#if !IGNORE_LOS
VERTEX_OUTPUT(2, vec2, v_los);
#endif
