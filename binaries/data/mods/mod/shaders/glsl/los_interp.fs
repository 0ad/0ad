#version 110

#include "los_interp.h"

#include "common/fragment.h"

void main(void)
{
	vec4 los2 = SAMPLE_2D(GET_DRAW_TEXTURE_2D(losTex1), v_tex).rrrr;
	vec4 los1 = SAMPLE_2D(GET_DRAW_TEXTURE_2D(losTex2), v_tex).rrrr;

	OUTPUT_FRAGMENT_SINGLE_COLOR(mix(los1, los2, clamp(delta, 0.0, 1.0)));
}

