#version 110

#include "minimap.h"

#include "common/fragment.h"

void main()
{
#if MINIMAP_BASE
	OUTPUT_FRAGMENT_SINGLE_COLOR(SAMPLE_2D(GET_DRAW_TEXTURE_2D(baseTex), v_tex));
#endif

#if MINIMAP_LOS
	OUTPUT_FRAGMENT_SINGLE_COLOR(SAMPLE_2D(GET_DRAW_TEXTURE_2D(baseTex), v_tex).rrrr);
#endif

#if MINIMAP_POINT
	OUTPUT_FRAGMENT_SINGLE_COLOR(vec4(color, 1.0));
#endif
}
