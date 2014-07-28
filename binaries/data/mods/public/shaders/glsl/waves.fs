#version 110

uniform sampler2D waveTex;
uniform sampler2D foamTex;

uniform float translation;
uniform float width;

varying float ttime;
varying vec2 normal;

void main()
{
	vec4 Tex = texture2D(waveTex, -gl_TexCoord[0].xy/8.0).rbga;

	Tex.rgb -= vec3(0.5,0.0,0.5);
	Tex.rb *= -1.0;
	
	float halfwidth = (width-1.0) / 2.0;
	
	float forceAlpha = min(1.0,2.0-abs(gl_TexCoord[0].x-halfwidth)/(halfwidth/2.0));
	float val = min(1.0,gl_TexCoord[0].y);
	forceAlpha *= val;
	
	float timeAlpha = 1.0 - clamp((ttime - 6.0)/4.0,0.0,1.0);
	timeAlpha *= clamp(ttime/2.0,0.0,1.0);
	
	float foamAlpha = Tex.a * forceAlpha * timeAlpha;
	
	Tex.a = forceAlpha * timeAlpha;
	
	vec2 norm = Tex.rb;
	
	Tex.r = norm.x * normal.x - norm.y * normal.x;
	Tex.b = norm.x * normal.y + norm.y * normal.y;
	
	vec3 foam = texture2D(foamTex, -gl_TexCoord[0].xy/vec2(2.5,7.0) + vec2(0.05,-0.3)*-cos(ttime/2.0)).rbg;
	foam *= texture2D(foamTex, -gl_TexCoord[0].xy/5.0 + vec2(0.8,-0.8) + vec2(-0.05,-0.25)*-cos(ttime/2.0)*1.2).rbg;

	gl_FragData[0] = vec4(Tex);
	gl_FragData[1] = vec4(foam*3.0,foamAlpha);
	return;
}
