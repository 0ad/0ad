#version 110

uniform sampler2D waveTex;
uniform sampler2D infoTex;

uniform float time;
uniform float waviness;

void main()
{
	vec3 color = texture2D(waveTex, gl_TexCoord[0].st * vec2(2.0,4.0) - vec2(0.0,0.5 + time/5.0)).rgb;
	float split = abs(gl_TexCoord[0].x - 0.5);
	split = 0.48 - split;
	split *= 3.0;
	split = min(1.0,split);
	
	float opac = split*min(1.0, gl_TexCoord[0].y);
	opac *= 1.0 - max(0.0,gl_TexCoord[0].y-0.9)*10.0;
	color = mix(vec3(0.5,0.5,1.0),color, opac);
	
	gl_FragColor.rgb = mix(vec3(0.5,0.5,1.0), color, clamp(texture2D(infoTex,gl_TexCoord[0].zw).r,0.4,1.0));
	
	gl_FragColor.a = 1.0;
}
