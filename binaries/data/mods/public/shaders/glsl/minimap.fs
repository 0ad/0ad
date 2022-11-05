#version 110

#include "common/fragment.h"

#if MINIMAP_BASE || MINIMAP_LOS
uniform sampler2D baseTex;
#endif

#if MINIMAP_BASE || MINIMAP_LOS
varying vec2 v_tex;
#endif

#if MINIMAP_POINT
varying vec3 color;
#endif

void main()
{
#if MINIMAP_BASE
	OUTPUT_FRAGMENT_SINGLE_COLOR(texture2D(baseTex, v_tex));
#endif

#if MINIMAP_LOS
	OUTPUT_FRAGMENT_SINGLE_COLOR(texture2D(baseTex, v_tex).rrrr);
#endif

#if MINIMAP_POINT
	OUTPUT_FRAGMENT_SINGLE_COLOR(vec4(color, 1.0));
#endif
}
