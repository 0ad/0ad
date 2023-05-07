#version 120

#include "common/fragment.h"
#include "common/stage.h"

BEGIN_DRAW_TEXTURES
	TEXTURE_2D(0, renderedTex)
	TEXTURE_2D(1, depthTex)
	TEXTURE_2D(2, blurTex2)
	TEXTURE_2D(3, blurTex4)
	TEXTURE_2D(4, blurTex8)
END_DRAW_TEXTURES

BEGIN_DRAW_UNIFORMS
	UNIFORM(float, width)
	UNIFORM(float, height)
	UNIFORM(float, zNear)
	UNIFORM(float, zFar)
	UNIFORM(float, brightness)
	UNIFORM(float, hdr)
	UNIFORM(float, saturation)
	UNIFORM(float, bloom)
END_DRAW_UNIFORMS

VERTEX_OUTPUT(0, vec2, v_tex);

float linearizeDepth(float depth)
{
	return -zFar * zNear / (depth * (zFar - zNear) - zFar);
}

void main(void)
{
	vec3 color = SAMPLE_2D(GET_DRAW_TEXTURE_2D(renderedTex), v_tex).rgb;

	vec3 blur2 = SAMPLE_2D(GET_DRAW_TEXTURE_2D(blurTex2), v_tex).rgb;
	vec3 blur4 = SAMPLE_2D(GET_DRAW_TEXTURE_2D(blurTex4), v_tex).rgb;
	vec3 blur8 = SAMPLE_2D(GET_DRAW_TEXTURE_2D(blurTex8), v_tex).rgb;

	float depth = SAMPLE_2D(GET_DRAW_TEXTURE_2D(depthTex), v_tex).r;

	float midDepth = SAMPLE_2D(GET_DRAW_TEXTURE_2D(depthTex), vec2(0.5, 0.5)).r;
	midDepth += SAMPLE_2D(GET_DRAW_TEXTURE_2D(depthTex), vec2(0.4, 0.4)).r;
	midDepth += SAMPLE_2D(GET_DRAW_TEXTURE_2D(depthTex), vec2(0.4, 0.6)).r;
	midDepth += SAMPLE_2D(GET_DRAW_TEXTURE_2D(depthTex), vec2(0.6, 0.4)).r;
	midDepth += SAMPLE_2D(GET_DRAW_TEXTURE_2D(depthTex), vec2(0.6, 0.6)).r;

	midDepth /= 5.0;


	float lDepth = linearizeDepth(depth);
	float lMidDepth = linearizeDepth(midDepth);
	float amount = abs(lDepth - lMidDepth);

	amount = clamp(amount / (lMidDepth * BLUR_FOV), 0.0, 1.0);

	color = (amount >= 0.0 && amount < 0.25) ? mix(color, blur2, (amount - 0.0) / (0.25)) : color;
	color = (amount >= 0.25 && amount < 0.50) ? mix(blur2, blur4, (amount - 0.25) / (0.25)) : color;
	color = (amount >= 0.50 && amount < 0.75) ? mix(blur4, blur8, (amount - 0.50) / (0.25)) : color;
	color = (amount >= 0.75 && amount <= 1.00) ? blur8 : color;

	OUTPUT_FRAGMENT_SINGLE_COLOR(vec4(color, 1.0));
}
