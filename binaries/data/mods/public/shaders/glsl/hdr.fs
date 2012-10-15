#version 120

uniform sampler2D renderedTex;
uniform sampler2D depthTex;
uniform sampler2D blurTex2;
uniform sampler2D blurTex4;
uniform sampler2D blurTex8;

uniform float width;
uniform float height;

uniform float brightness;
uniform float hdr;
uniform float saturation;
uniform float bloom;

varying vec2 v_tex;


void main(void)
{


	vec3 colour = texture2D(renderedTex, v_tex).rgb;
	vec3 bloomv2 = texture2D(blurTex2, v_tex).rgb;
	vec3 bloomv4 = texture2D(blurTex4, v_tex).rgb;
	vec3 bloomv8 = texture2D(blurTex8, v_tex).rgb;

	bloomv2 = max(bloomv2 - bloom, vec3(0.0));
	bloomv4 = max(bloomv4 - bloom, vec3(0.0));
	bloomv8 = max(bloomv8 - bloom, vec3(0.0));

	vec3 bloomv = (bloomv2 + bloomv4 + bloomv8) / 3.0;

	colour = max(bloomv, colour);
	
	colour += vec3(brightness);

	colour -= vec3(0.5);
	colour *= vec3(hdr);
	colour += vec3(0.5);

	colour = mix(vec3(dot(colour, vec3(0.299, 0.587, 0.114))), colour, saturation);

	
	gl_FragColor.rgb = colour;
	gl_FragColor.a = 1.0;
}


