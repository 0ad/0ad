#version 120

#include "common/debug_fragment.h"
#include "common/los_fragment.h"

uniform sampler2D baseTex;
uniform sampler2D maskTex;

#if USE_OBJECTCOLOR
uniform vec4 objectColor;
#else
varying vec4 v_color;
#endif

varying vec2 v_tex;

void main()
{
#if USE_OBJECTCOLOR
    vec3 color = objectColor.rgb;
    float alpha = objectColor.a;
#else
    vec3 color = v_color.rgb;
    float alpha = v_color.a;
#endif

    vec4 base = texture2D(baseTex, v_tex);
    vec4 mask = texture2D(maskTex, v_tex);
    color = mix(base.rgb, color, mask.r);

    color *= getLOS();

    gl_FragColor = vec4(applyDebugColor(color, 1.0, alpha * base.a, 0.0), alpha * base.a);
}
