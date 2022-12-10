#version 110

#include "debug_overlay.h"

#include "common/fragment.h"

void main()
{
#if DEBUG_TEXTURED
	OUTPUT_FRAGMENT_SINGLE_COLOR(SAMPLE_2D(GET_DRAW_TEXTURE_2D(baseTex), v_tex));
#else
	OUTPUT_FRAGMENT_SINGLE_COLOR(color);
#endif
}
