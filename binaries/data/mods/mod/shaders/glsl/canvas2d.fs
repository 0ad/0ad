#version 110

#include "common/fragment.h"

uniform sampler2D tex;
uniform vec4 colorAdd;
uniform vec4 colorMul;
uniform float grayscaleFactor;

varying vec2 v_uv;

void main()
{
	vec4 colorTex = texture2D(tex, v_uv);
	vec3 grayColor = vec3(dot(vec3(0.3, 0.59, 0.11), colorTex.rgb));
	OUTPUT_FRAGMENT_SINGLE_COLOR(clamp(mix(colorTex, vec4(grayColor, colorTex.a), grayscaleFactor) * colorMul + colorAdd, 0.0, 1.0));
}
