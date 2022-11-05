#version 110

#include "common/fragment.h"
#include "common/los_fragment.h"

uniform sampler2D baseTex;
uniform vec3 color;

varying vec2 v_coords;

void main()
{
	OUTPUT_FRAGMENT_SINGLE_COLOR(vec4(texture2D(baseTex, v_coords).rgb * color * getLOS(), 1.0));
}
