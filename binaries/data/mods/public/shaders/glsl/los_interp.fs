#version 110

#include "common/fragment.h"

varying vec2 v_tex;

uniform sampler2D losTex1, losTex2;

uniform float delta;


void main(void)
{
	vec4 los2 = texture2D(losTex1, v_tex).rrrr;
	vec4 los1 = texture2D(losTex2, v_tex).rrrr;

	OUTPUT_FRAGMENT_SINGLE_COLOR(mix(los1, los2, clamp(delta, 0.0, 1.0)));
}

