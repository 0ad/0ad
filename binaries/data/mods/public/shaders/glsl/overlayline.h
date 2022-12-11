#include "common/stage.h"

BEGIN_DRAW_TEXTURES
	TEXTURE_2D(0, baseTex)
	TEXTURE_2D(1, maskTex)
#if !IGNORE_LOS
	TEXTURE_2D(2, losTex)
#endif
END_DRAW_TEXTURES

BEGIN_DRAW_UNIFORMS
	UNIFORM(mat4, transform)
#if USE_OBJECTCOLOR
	UNIFORM(vec4, objectColor)
#endif
	UNIFORM(vec2, losTransform)
END_DRAW_UNIFORMS

VERTEX_OUTPUT(0, vec2, v_tex);
#if !USE_OBJECTCOLOR
VERTEX_OUTPUT(1, vec4, v_color);
#endif
#if !IGNORE_LOS
VERTEX_OUTPUT(2, vec2, v_los);
#endif
