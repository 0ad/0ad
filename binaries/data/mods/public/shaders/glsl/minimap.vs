#version 110

#include "minimap.h"

#include "common/vertex.h"

VERTEX_INPUT_ATTRIBUTE(0, vec2, a_vertex);
#if MINIMAP_BASE || MINIMAP_LOS
VERTEX_INPUT_ATTRIBUTE(1, vec2, a_uv0);
#endif
#if MINIMAP_POINT
VERTEX_INPUT_ATTRIBUTE(1, vec3, a_color);
#if USE_GPU_INSTANCING
VERTEX_INPUT_ATTRIBUTE(2, vec2, a_uv1);
#endif
#endif

void main()
{
#if MINIMAP_BASE || MINIMAP_LOS
	vec2 position = a_vertex;
	v_tex = mat2(textureTransform.xy, textureTransform.zw) * a_uv0 + translation.zw;
#endif

#if MINIMAP_POINT
#if USE_GPU_INSTANCING
	vec2 position = a_vertex * width + a_uv1;
#else
	vec2 position = a_vertex;
#endif
	color = a_color;
#endif // MINIMAP_POINT

	OUTPUT_VERTEX_POSITION(vec4(mat2(transform.xy, transform.zw) * position + translation.xy, 0.0, 1.0));
}
