#include "common/stage.h"

BEGIN_DRAW_TEXTURES
	TEXTURE_2D(0, losTex1)
	TEXTURE_2D(1, losTex2)
END_DRAW_TEXTURES

BEGIN_DRAW_UNIFORMS
	UNIFORM(mat4, transform)
	UNIFORM(vec2, losTransform)
	UNIFORM(float, delta)
END_DRAW_UNIFORMS

VERTEX_OUTPUT(0, vec2, v_tex);
