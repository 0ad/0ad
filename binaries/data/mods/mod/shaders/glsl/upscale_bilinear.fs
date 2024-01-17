#version 120

#include "common/fragment.h"
#include "common/stage.h"

BEGIN_DRAW_TEXTURES
	TEXTURE_2D(0, inTex)
END_DRAW_TEXTURES

BEGIN_DRAW_UNIFORMS
	UNIFORM(vec4, screenSize)
END_DRAW_UNIFORMS

VERTEX_OUTPUT(0, vec2, v_tex);

void main()
{
	OUTPUT_FRAGMENT_SINGLE_COLOR(SAMPLE_2D(GET_DRAW_TEXTURE_2D(inTex), v_tex));
}
