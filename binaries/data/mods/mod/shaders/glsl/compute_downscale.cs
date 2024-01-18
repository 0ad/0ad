#version 430

#include "common/compute.h"

BEGIN_DRAW_TEXTURES
	TEXTURE_2D(0, inTex)
END_DRAW_TEXTURES

BEGIN_DRAW_UNIFORMS
	UNIFORM(vec4, screenSize)
END_DRAW_UNIFORMS

STORAGE_2D(0, rgba8, outTex);

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
	ivec2 position = ivec2(gl_GlobalInvocationID.xy);
	if (any(greaterThanEqual(position, ivec2(screenSize.zw))))
		return;
	vec2 uv = (vec2(position) + vec2(0.5, 0.5)) / screenSize.zw;
	imageStore(outTex, position, texture(GET_DRAW_TEXTURE_2D(inTex), uv));
}
