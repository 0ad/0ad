#version 110

#include "canvas2d.h"

#include "common/fragment.h"

void main()
{
	vec4 colorTex = SAMPLE_2D(GET_DRAW_TEXTURE_2D(tex), v_uv);
	vec3 grayColor = vec3(dot(vec3(0.3, 0.59, 0.11), colorTex.rgb));
	OUTPUT_FRAGMENT_SINGLE_COLOR(clamp(mix(colorTex, vec4(grayColor, colorTex.a), grayscaleFactor) * colorMul + colorAdd, 0.0, 1.0));
}
