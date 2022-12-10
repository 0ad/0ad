#include "common/stage.h"

BEGIN_DRAW_TEXTURES
	TEXTURE_2D(0, waveTex)
	TEXTURE_2D(1, foamTex)
END_DRAW_TEXTURES

BEGIN_DRAW_UNIFORMS
	UNIFORM(float, translation)
	UNIFORM(float, width)
	UNIFORM(float, time)
	UNIFORM(mat4, transform)
END_DRAW_UNIFORMS

VERTEX_OUTPUT(0, float, ttime);
VERTEX_OUTPUT(1, vec2, normal);
VERTEX_OUTPUT(2, vec2, v_tex);
