#version 110

#include "foreground_overlay.h"

#include "common/fragment.h"

void main()
{
	OUTPUT_FRAGMENT_SINGLE_COLOR(SAMPLE_2D(GET_DRAW_TEXTURE_2D(baseTex), v_tex) * colorMul);
}
