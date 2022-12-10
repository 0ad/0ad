#version 110

// LICENSE
// =======
// Copyright (c) 2020 Advanced Micro Devices, Inc. All rights reserved.
// -------
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
// -------
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
// Software.
// -------
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE

// GLSL port of the CasFilter() (no scaling). https://github.com/GPUOpen-Effects/FidelityFX-CAS/blob/master/ffx-cas/ffx_cas.h

#include "common/fragment.h"
#include "common/stage.h"

BEGIN_DRAW_TEXTURES
	TEXTURE_2D(0, renderedTex)
END_DRAW_TEXTURES

BEGIN_DRAW_UNIFORMS
	UNIFORM(float, width)
	UNIFORM(float, height)
	UNIFORM(float, sharpness)
END_DRAW_UNIFORMS

VERTEX_OUTPUT(0, vec2, v_tex);

float saturate(float inputFloat)
{
    return clamp(inputFloat, 0.0, 1.0);
}

vec3 saturate(vec3 inputFloat)
{
    return vec3(saturate(inputFloat.x), saturate(inputFloat.y), saturate(inputFloat.z));
}

vec3 sharpen(sampler2D tex)
{
    vec2 invSSize = vec2(1.0 / width, 1.0 / height);

    // No scaling algorithm uses a minimal 3x3 neighborhood around the pixel 'e',
    //  a b c
    //  d(e)f
    //  g h i
    vec3 a = SAMPLE_2D(tex, v_tex + vec2(-1.0, -1.0) * invSSize).rgb;
    vec3 b = SAMPLE_2D(tex, v_tex + vec2(0.0, -1.0) * invSSize).rgb;
    vec3 c = SAMPLE_2D(tex, v_tex + vec2(1.0, -1.0) * invSSize).rgb;
    vec3 d = SAMPLE_2D(tex, v_tex + vec2(-1.0, 0.0) * invSSize).rgb;
    vec3 e = SAMPLE_2D(tex, v_tex + vec2(0.0, 0.0) * invSSize).rgb;
    vec3 f = SAMPLE_2D(tex, v_tex + vec2(1.0, 0.0) * invSSize).rgb;
    vec3 g = SAMPLE_2D(tex, v_tex + vec2(-1.0, 1.0) * invSSize).rgb;
    vec3 h = SAMPLE_2D(tex, v_tex + vec2(0.0, 1.0) * invSSize).rgb;
    vec3 i = SAMPLE_2D(tex, v_tex + vec2(1.0, 1.0) * invSSize).rgb;

    // Soft min and max.
    //  a b c             b
    //  d e f * 0.5  +  d e f * 0.5
    //  g h i             h
    // These are 2.0x bigger (factored out the extra multiply).
    vec3 mnRGB = min(min(min(d, e), min(f, b)), h);
    vec3 mnRGB2 = min(mnRGB, min(min(a, c), min(g, i)));
    mnRGB += mnRGB2;

    vec3 mxRGB = max(max(max(d, e), max(f, b)), h);
    vec3 mxRGB2 = max(mxRGB, max(max(a, c), max(g, i)));
    mxRGB += mxRGB2;

    // Smooth minimum distance to signal limit divided by smooth max.
    vec3 rcpMRGB = 1.0 / (mxRGB);
    vec3 ampRGB = saturate(min(mnRGB, 2.0 - mxRGB) * rcpMRGB);    

    // Shaping amount of sharpening.
    ampRGB = sqrt(ampRGB);

    float peak = -1.0 / (8.0 - 3.0 * sharpness);
    vec3 wRGB = ampRGB * peak;

    vec3 rcpWeightRGB = 1.0 / (1.0 + 4.0 * wRGB);

    //                          0 w 0
    //  Filter shape:           w 1 w
    //                          0 w 0  
    vec3 outColor = saturate(((b + d + f + h) * wRGB + e) * rcpWeightRGB);

    return outColor;
}

void main()
{
    OUTPUT_FRAGMENT_SINGLE_COLOR(vec4(sharpen(GET_DRAW_TEXTURE_2D(renderedTex)), 1.0));
}
