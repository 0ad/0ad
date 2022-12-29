#include "common/stage.h"

BEGIN_DRAW_TEXTURES
	NO_DRAW_TEXTURES
END_DRAW_TEXTURES

BEGIN_DRAW_UNIFORMS
	UNIFORM(mat4, transform)
	UNIFORM(vec4, instancingTransform)
	UNIFORM(vec4, color)
END_DRAW_UNIFORMS
