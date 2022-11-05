#version 110

#include "common/fragment.h"

uniform samplerCube baseTex;
varying vec3 v_tex;

void main()
{
    vec4 tex = textureCube(baseTex, v_tex);

    float m = (1.0 - v_tex.y) - 0.75;
    m *= 4.0;

    OUTPUT_FRAGMENT_SINGLE_COLOR((v_tex.y > 0.0) ? (tex * m) : tex);
}
