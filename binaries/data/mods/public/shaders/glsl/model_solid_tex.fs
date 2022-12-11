#version 110

#include "model_common.h"

#include "common/fragment.h"

void main()
{
  vec4 tex = SAMPLE_2D(GET_DRAW_TEXTURE_2D(baseTex), v_tex);

  #ifdef REQUIRE_ALPHA_GEQUAL
    if (tex.a < REQUIRE_ALPHA_GEQUAL)
      discard;
  #endif

  OUTPUT_FRAGMENT_SINGLE_COLOR(tex);
}
