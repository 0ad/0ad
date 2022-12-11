#version 110

#include "sky.h"

#include "common/fragment.h"

void main()
{
    vec4 tex = SAMPLE_CUBE(GET_DRAW_TEXTURE_CUBE(baseTex), v_tex);

    float m = (1.0 - v_tex.y) - 0.75;
    m *= 4.0;

    OUTPUT_FRAGMENT_SINGLE_COLOR((v_tex.y > 0.0) ? (tex * m) : tex);
}
