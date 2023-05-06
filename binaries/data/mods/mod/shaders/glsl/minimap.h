#include "common/stage.h"

BEGIN_DRAW_TEXTURES
#if MINIMAP_BASE || MINIMAP_LOS
	TEXTURE_2D(0, baseTex)
#else
	NO_DRAW_TEXTURES
#endif
END_DRAW_TEXTURES

BEGIN_DRAW_UNIFORMS
	UNIFORM(vec4, transform)
	UNIFORM(vec4, translation)
	UNIFORM(vec4, textureTransform)
#if MINIMAP_POINT && USE_GPU_INSTANCING
	UNIFORM(float, width)
#endif
END_DRAW_UNIFORMS

#if MINIMAP_BASE || MINIMAP_LOS
VERTEX_OUTPUT(0, vec2, v_tex);
#endif

#if MINIMAP_POINT
VERTEX_OUTPUT(0, vec3, color);
#endif
