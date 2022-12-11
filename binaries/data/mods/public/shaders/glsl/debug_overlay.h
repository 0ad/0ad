#include "common/stage.h"

BEGIN_DRAW_TEXTURES
#if DEBUG_TEXTURED
	TEXTURE_2D(0, baseTex)
#else
	NO_DRAW_TEXTURES
#endif
END_DRAW_TEXTURES

BEGIN_DRAW_UNIFORMS
	UNIFORM(mat4, transform)
#if DEBUG_TEXTURED
	UNIFORM(vec2, textureTransform)
#else
	UNIFORM(vec4, color)
#endif
END_DRAW_UNIFORMS

#if DEBUG_TEXTURED
VERTEX_OUTPUT(0, vec2, v_tex);
#endif
