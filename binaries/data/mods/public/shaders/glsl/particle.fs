#version 110

#include "common/fog.h"
#include "common/fragment.h"
#include "common/los_fragment.h"

uniform sampler2D baseTex;

varying vec2 v_tex;
varying vec4 v_color;

uniform vec3 sunColor;

void main()
{
	vec4 color = texture2D(baseTex, v_tex) * vec4((v_color.rgb + sunColor)/2.0,v_color.a);

	color.rgb = applyFog(color.rgb);
	color.rgb *= getLOS();

	OUTPUT_FRAGMENT_SINGLE_COLOR(color);
}
