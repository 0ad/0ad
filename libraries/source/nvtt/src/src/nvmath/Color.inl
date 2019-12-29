// This code is in the public domain -- castanyo@yahoo.es

#pragma once
#ifndef NV_MATH_COLOR_INL
#define NV_MATH_COLOR_INL

#include "Color.h"
#include "Vector.inl"
#include "ftoi.h"


namespace nv
{
    // for Color16 & Color16_4444 bitfields
    NV_FORCEINLINE uint32 U32round(float f) { return uint32(floorf(f + 0.5f)); }
    NV_FORCEINLINE uint16 U16round(float f) { return uint16(floorf(f + 0.5f)); }
    NV_FORCEINLINE uint16 toU4_in_U16(int x) { nvDebugCheck(x >= 0 && x <= 15u); return (uint16)x; }
    NV_FORCEINLINE uint16 toU5_in_U16(int x) { nvDebugCheck(x >= 0 && x <= 31u); return (uint16)x; }
    NV_FORCEINLINE uint16 toU6_in_U16(int x) { nvDebugCheck(x >= 0 && x <= 63u); return (uint16)x; }

    // Clamp color components.
    inline Vector3 colorClamp(Vector3::Arg c)
    {
        return Vector3(saturate(c.x), saturate(c.y), saturate(c.z));
    }

    // Clamp without allowing the hue to change.
    inline Vector3 colorNormalize(Vector3::Arg c)
    {
        float scale = 1.0f;
        if (c.x > scale) scale = c.x;
        if (c.y > scale) scale = c.y;
        if (c.z > scale) scale = c.z;
        return c / scale;
    }

    // Convert Color16 from float components
    inline Color16 toColor16(float r, float g, float b)
    {
        Color16 color; // 5,6,5
        color.r = toU5_in_U16(nv::U16round(saturate(r) * 31u));
        color.g = toU6_in_U16(nv::U16round(saturate(g) * 63u));
        color.b = toU5_in_U16(nv::U16round(saturate(b) * 31u));
        return color;
    }

    // Convert Color32 to Color16.
    inline Color16 toColor16(Color32 c)
    {
        Color16 color;
        //         rrrrrggggggbbbbb
        // rrrrr000gggggg00bbbbb000
        // color.u = (c.u >> 3) & 0x1F;
        // color.u |= (c.u >> 5) & 0x7E0;
        // color.u |= (c.u >> 8) & 0xF800;

        color.r = c.r >> 3;
        color.g = c.g >> 2;
        color.b = c.b >> 3;
        return color; 
    }

    // Convert Color32 to Color16_4444.
    inline Color16_4444 toColor16_4444(Color32 c)
    {
        Color16_4444 color;
        color.a = c.a >> 4;
        color.r = c.r >> 4;
        color.g = c.g >> 4;
        color.b = c.b >> 4;
        return color; 
    }

    // Convert float[4] to Color16_4444.
    inline Color16_4444 toColor16_4444(float r, float g, float b, float a)
    {
        Color16_4444 color;
        color.a = toU4_in_U16(nv::U16round(saturate(a) * 15u));
        color.r = toU4_in_U16(nv::U16round(saturate(r) * 15u));
        color.g = toU4_in_U16(nv::U16round(saturate(g) * 15u));
        color.b = toU4_in_U16(nv::U16round(saturate(b) * 15u));
        return color;
    }

    // Convert float[4] to Color16_4444.
    inline Color16_4444 toColor16_4444_from_argb(float * fc)
    {
        Color16_4444 color;
        color.a = toU4_in_U16(nv::U16round(saturate(fc[0]) * 15u));
        color.r = toU4_in_U16(nv::U16round(saturate(fc[1]) * 15u));
        color.g = toU4_in_U16(nv::U16round(saturate(fc[2]) * 15u));
        color.b = toU4_in_U16(nv::U16round(saturate(fc[3]) * 15u));
        return color;
    }

    // Convert float[4] to Color16_4444.
    inline Color16_4444 toColor16_4444_from_bgra(float * fc)
    {
        Color16_4444 color;
        color.b = toU4_in_U16(nv::U16round(saturate(fc[0]) * 15u));
        color.g = toU4_in_U16(nv::U16round(saturate(fc[1]) * 15u));
        color.r = toU4_in_U16(nv::U16round(saturate(fc[2]) * 15u));
        color.a = toU4_in_U16(nv::U16round(saturate(fc[3]) * 15u));
        return color;
    }

    // Promote 16 bit color to 32 bit using regular bit expansion.
    inline Color32 toColor32(Color16 c)
    {
        Color32 color;
        // c.u = ((col0.u << 3) & 0xf8) | ((col0.u << 5) & 0xfc00) | ((col0.u << 8) & 0xf80000);
        // c.u |= (c.u >> 5) & 0x070007;
        // c.u |= (c.u >> 6) & 0x000300;

        color.b = (c.b << 3) | (c.b >> 2);
        color.g = (c.g << 2) | (c.g >> 4);
        color.r = (c.r << 3) | (c.r >> 2);
        color.a = 0xFF;

        return color;
    }

    // @@ Quantize with exact endpoints or with uniform bins?
    inline Color32 toColor32(const Vector4 & v)
    {
        Color32 color;
        color.r = U8(ftoi_round(saturate(v.x) * 255));
        color.g = U8(ftoi_round(saturate(v.y) * 255));
        color.b = U8(ftoi_round(saturate(v.z) * 255));
        color.a = U8(ftoi_round(saturate(v.w) * 255));
        return color;
    }

    inline Color32 toColor32_from_bgra(const Vector4 & v)
    {
        Color32 color;
        color.b = U8(ftoi_round(saturate(v.x) * 255));
        color.g = U8(ftoi_round(saturate(v.y) * 255));
        color.r = U8(ftoi_round(saturate(v.z) * 255));
        color.a = U8(ftoi_round(saturate(v.w) * 255));
        return color;
    }

    inline Color32 toColor32_from_argb(const Vector4 & v)
    {
        Color32 color;
        color.a = U8(ftoi_round(saturate(v.x) * 255));
        color.r = U8(ftoi_round(saturate(v.y) * 255));
        color.g = U8(ftoi_round(saturate(v.z) * 255));
        color.b = U8(ftoi_round(saturate(v.w) * 255));
        return color;
    }

    inline Vector4 toVector4(Color32 c)
    {
        const float scale = 1.0f / 255.0f;
        return Vector4(c.r * scale, c.g * scale, c.b * scale, c.a * scale);
    }


    inline float perceptualColorDistance(Vector3::Arg c0, Vector3::Arg c1)
    {
        float rmean = (c0.x + c1.x) * 0.5f;
        float r = c1.x - c0.x;
        float g = c1.y - c0.y;
        float b = c1.z - c0.z;
        return sqrtf((2 + rmean)*r*r + 4*g*g + (3 - rmean)*b*b);
    }

    
    inline float hue(float r, float g, float b) {
        float h = atan2f(sqrtf(3.0f)*(g-b), 2*r-g-b) * (1.0f / (2 * PI)) + 0.5f;
        return h;
    }

    inline float toSrgb(float f) {
        if (nv::isNan(f))           f = 0.0f;
        else if (f <= 0.0f)         f = 0.0f;
        else if (f <= 0.0031308f)   f = 12.92f * f;
        else if (f <= 1.0f)         f = (powf(f, 0.41666f) * 1.055f) - 0.055f;
        else                        f = 1.0f;
        return f;
    }

    inline float fromSrgb(float f) {
        if (f < 0.0f)           f = 0.0f;
        else if (f < 0.04045f)  f = f / 12.92f;
        else if (f <= 1.0f)     f = powf((f + 0.055f) / 1.055f, 2.4f);
        else                    f = 1.0f;
        return f;
    }

    inline Vector3 toSrgb(const Vector3 & v) {
        return Vector3(toSrgb(v.x), toSrgb(v.y), toSrgb(v.z));
    }

    inline Vector3 fromSrgb(const Vector3 & v) {
        return Vector3(fromSrgb(v.x), fromSrgb(v.y), fromSrgb(v.z));
    }

} // nv namespace

#endif // NV_MATH_COLOR_INL
