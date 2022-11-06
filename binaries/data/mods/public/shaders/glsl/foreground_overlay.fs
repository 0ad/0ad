#version 110

#include "common/fragment.h"

uniform sampler2D baseTex;
uniform vec4 colorMul;
varying vec2 v_tex;

void main()
{
	OUTPUT_FRAGMENT_SINGLE_COLOR(texture2D(baseTex, v_tex) * colorMul);
}
