#include "common/stage.h"

BEGIN_DRAW_TEXTURES
	TEXTURE_2D(0, tex)
END_DRAW_TEXTURES

BEGIN_DRAW_UNIFORMS
	UNIFORM(vec4, transform)
	UNIFORM(vec2, translation)
	UNIFORM(vec4, colorAdd)
	UNIFORM(vec4, colorMul)
	UNIFORM(float, grayscaleFactor)
END_DRAW_UNIFORMS

VERTEX_OUTPUT(0, vec2, v_uv);
