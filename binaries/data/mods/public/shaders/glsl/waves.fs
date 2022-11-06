#version 110

#include "common/fragment.h"

uniform sampler2D waveTex;
uniform sampler2D foamTex;

uniform float translation;
uniform float width;

varying float ttime;
varying vec2 normal;
varying vec2 v_tex;

void main()
{
	vec4 Tex = texture2D(waveTex, -v_tex.xy/8.0).rbga;

	Tex.rgb -= vec3(0.5,0.0,0.5);
	Tex.rb *= -1.0;
	
	float halfwidth = (width-1.0) / 2.0;
	
	float forceAlpha = min(1.0,2.0-abs(v_tex.x-halfwidth)/(halfwidth/2.0));
	float val = min(1.0,v_tex.y);
	forceAlpha *= val;
	
	float timeAlpha = 1.0 - clamp((ttime - 6.0)/4.0,0.0,1.0);
	timeAlpha *= clamp(ttime/2.0,0.0,1.0);
	
	float foamAlpha = Tex.a * forceAlpha * timeAlpha;
	
	Tex.a = forceAlpha * timeAlpha;
	
	vec2 norm = Tex.rb;
	
	Tex.r = norm.x * normal.x - norm.y * normal.x;
	Tex.b = norm.x * normal.y + norm.y * normal.y;
	
	vec3 foam = texture2D(foamTex, -v_tex.xy/vec2(2.5,7.0) + vec2(0.05,-0.3)*-cos(ttime/2.0)).rbg;
	foam *= texture2D(foamTex, -v_tex.xy/5.0 + vec2(0.8,-0.8) + vec2(-0.05,-0.25)*-cos(ttime/2.0)*1.2).rbg;
	Tex.g = foamAlpha * clamp(foam.r * 3.0, 0.0, 1.0) * 0.4;

	OUTPUT_FRAGMENT_SINGLE_COLOR(Tex);
}
