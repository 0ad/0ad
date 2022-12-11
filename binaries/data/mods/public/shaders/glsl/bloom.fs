#version 110

#include "common/fragment.h"
#include "common/stage.h"

BEGIN_DRAW_TEXTURES
	TEXTURE_2D(0, renderedTex)
END_DRAW_TEXTURES

BEGIN_DRAW_UNIFORMS
	UNIFORM(vec2, texSize)
END_DRAW_UNIFORMS

VERTEX_OUTPUT(0, vec2, v_tex);

void main()
{
  #if BLOOM_NOP
    OUTPUT_FRAGMENT_SINGLE_COLOR(vec4(SAMPLE_2D(GET_DRAW_TEXTURE_2D(renderedTex), v_tex).rgb, 1.0));
  #endif

  #if BLOOM_PASS_H
    vec4 color = vec4(0.0);
    vec2 v_tex_offs = vec2(v_tex.x - 0.01, v_tex.y);
    
    for (int i = 0; i < 6; ++i)
    {
      color += SAMPLE_2D(GET_DRAW_TEXTURE_2D(renderedTex), v_tex_offs);
      v_tex_offs += vec2(0.004, 0.0);
    }
    
    OUTPUT_FRAGMENT_SINGLE_COLOR(vec4(color.rgb / 6.0, 1.0));
  #endif

  #if BLOOM_PASS_V
    vec4 color = vec4(0.0);
    vec2 v_tex_offs = vec2(v_tex.x, v_tex.y - 0.01);
    
    for (int i = 0; i < 6; ++i)
    {
      color += SAMPLE_2D(GET_DRAW_TEXTURE_2D(renderedTex), v_tex_offs);
      v_tex_offs += vec2(0.0, 0.004);
    }
    
    OUTPUT_FRAGMENT_SINGLE_COLOR(vec4(color.rgb / 6.0, 1.0));
  #endif
}
