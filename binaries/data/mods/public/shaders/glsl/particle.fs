#version 110

#include "common/fog.h"

uniform sampler2D baseTex;
uniform sampler2D losTex;

varying vec2 v_tex;
varying vec2 v_los;
varying vec4 v_color;

uniform vec3 sunColor;

void main()
{
	vec4 color = texture2D(baseTex, v_tex) * vec4((v_color.rgb + sunColor)/2.0,v_color.a);
	
    float los = texture2D(losTex, v_los).a;
    los = los < 0.03 ? 0.0 : los;
    color.rgb *= los;

	color.rgb = applyFog(color.rgb);

	gl_FragColor = color;
}
