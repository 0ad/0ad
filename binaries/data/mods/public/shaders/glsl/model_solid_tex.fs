#version 110

#include "common/fragment.h"

uniform sampler2D baseTex;

varying vec2 v_tex;

void main()
{
  vec4 tex = texture2D(baseTex, v_tex);

  #ifdef REQUIRE_ALPHA_GEQUAL
    if (tex.a < REQUIRE_ALPHA_GEQUAL)
      discard;
  #endif

  OUTPUT_FRAGMENT_SINGLE_COLOR(tex);
}
