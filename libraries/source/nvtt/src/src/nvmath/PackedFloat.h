// This code is in the public domain -- castano@gmail.com

#pragma once
#ifndef NV_MATH_PACKEDFLOAT_H
#define NV_MATH_PACKEDFLOAT_H

#include "nvmath.h"
#include "Vector.h"

namespace nv
{

    union FloatRGB9E5 {
        uint32 v;
        struct {
        #if NV_BIG_ENDIAN
            uint32 e : 5;
            uint32 zm : 9;
            uint32 ym : 9;
            uint32 xm : 9;
        #else
            uint32 xm : 9;
            uint32 ym : 9;
            uint32 zm : 9;
            uint32 e : 5;
        #endif
        };
    };

    union FloatR11G11B10 {
        uint32 v;
        struct {
        #if NV_BIG_ENDIAN
            uint32 ze : 5;
            uint32 zm : 5;
            uint32 ye : 5;
            uint32 ym : 6;
            uint32 xe : 5;
            uint32 xm : 6;
        #else
            uint32 xm : 6;
            uint32 xe : 5;
            uint32 ym : 6;
            uint32 ye : 5;
            uint32 zm : 5;
            uint32 ze : 5;
        #endif
        };
    };

    union FloatRGBE8 {
        uint32 v;
        struct {
        #if NV_LITTLE_ENDIAN
            uint8 r, g, b, e;
        #else
            uint8 e: 8;
            uint8 b: 8;
            uint8 g: 8;
            uint8 r: 8;
        #endif
        };
    };

    NVMATH_API Vector3 rgb9e5_to_vector3(FloatRGB9E5 v);
    NVMATH_API FloatRGB9E5 vector3_to_rgb9e5(const Vector3 & v);

    NVMATH_API float float11_to_float32(uint v);
    NVMATH_API float float10_to_float32(uint v);

    NVMATH_API Vector3 r11g11b10_to_vector3(FloatR11G11B10 v);
    NVMATH_API FloatR11G11B10 vector3_to_r11g11b10(const Vector3 & v);

    NVMATH_API Vector3 rgbe8_to_vector3(FloatRGBE8 v);
    NVMATH_API FloatRGBE8 vector3_to_rgbe8(const Vector3 & v);

} // nv

#endif // NV_MATH_PACKEDFLOAT_H
