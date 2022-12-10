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
	UNIFORM(float, brightness)
	UNIFORM(float, hdr)
	UNIFORM(float, saturation)
	UNIFORM(float, bloom)
END_DRAW_UNIFORMS

VERTEX_OUTPUT(0, vec2, v_tex);

void main(void)
{
	vec3 color = SAMPLE_2D(GET_DRAW_TEXTURE_2D(renderedTex), v_tex).rgb;
	vec3 bloomv2 = SAMPLE_2D(GET_DRAW_TEXTURE_2D(blurTex2), v_tex).rgb;
	vec3 bloomv4 = SAMPLE_2D(GET_DRAW_TEXTURE_2D(blurTex4), v_tex).rgb;
	vec3 bloomv8 = SAMPLE_2D(GET_DRAW_TEXTURE_2D(blurTex8), v_tex).rgb;

	bloomv2 = max(bloomv2 - bloom, vec3(0.0));
	bloomv4 = max(bloomv4 - bloom, vec3(0.0));
	bloomv8 = max(bloomv8 - bloom, vec3(0.0));

	vec3 bloomv = (bloomv2 + bloomv4 + bloomv8) / 3.0;

	bloomv = mix(bloomv, color, bloom/0.2);

	color = max(bloomv, color);

	color += vec3(brightness);

	color -= vec3(0.5);
	color *= vec3(hdr);
	color += vec3(0.5);

	color = mix(vec3(dot(color, vec3(0.299, 0.587, 0.114))), color, saturation);

	OUTPUT_FRAGMENT_SINGLE_COLOR(vec4(color, 1.0));
}
