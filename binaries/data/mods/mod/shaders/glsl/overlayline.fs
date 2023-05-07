#version 120

#include "overlayline.h"

#include "common/debug_fragment.h"
#include "common/fragment.h"
#include "common/los_fragment.h"

void main()
{
#if USE_OBJECTCOLOR
    vec3 color = objectColor.rgb;
    float alpha = objectColor.a;
#else
    vec3 color = v_color.rgb;
    float alpha = v_color.a;
#endif

    vec4 base = SAMPLE_2D(GET_DRAW_TEXTURE_2D(baseTex), v_tex);
    vec4 mask = SAMPLE_2D(GET_DRAW_TEXTURE_2D(maskTex), v_tex);
    color = mix(base.rgb, color, mask.r);

#if !IGNORE_LOS
    color *= getLOS(GET_DRAW_TEXTURE_2D(losTex), v_los);
#endif

    OUTPUT_FRAGMENT_SINGLE_COLOR(vec4(applyDebugColor(color, 1.0, alpha * base.a, 0.0), alpha * base.a));
}
