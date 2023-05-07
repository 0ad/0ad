#version 120

#include "overlayline.h"

#include "common/los_vertex.h"
#include "common/vertex.h"

VERTEX_INPUT_ATTRIBUTE(0, vec3, a_vertex);
VERTEX_INPUT_ATTRIBUTE(1, vec2, a_uv0);
#if !USE_OBJECTCOLOR
VERTEX_INPUT_ATTRIBUTE(2, vec4, a_color);
#endif

void main()
{
	v_tex = a_uv0;
#if !IGNORE_LOS
	v_los = calculateLOSCoordinates(a_vertex.xz, losTransform);
#endif
#if !USE_OBJECTCOLOR
	v_color = a_color;
#endif
	OUTPUT_VERTEX_POSITION(transform * vec4(a_vertex, 1.0));
}
