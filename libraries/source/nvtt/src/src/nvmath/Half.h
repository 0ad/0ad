#pragma once
#ifndef NV_MATH_HALF_H
#define NV_MATH_HALF_H

#include "nvmath.h"

namespace nv {

    NVMATH_API uint32 half_to_float( uint16 h );
    NVMATH_API uint16 half_from_float( uint32 f );

    // vin,vout must be 16 byte aligned. count must be a multiple of 8.
    // implement a non-SSE version if we need it. For now, this naming makes it clear this is only available when SSE2 is
    void half_to_float_array_SSE2(const uint16 * vin, float * vout, int count);

    NVMATH_API void half_init_tables();
    NVMATH_API uint32 fast_half_to_float(uint16 h);

    inline uint16 to_half(float c) {
        union { float f; uint32 u; } f;
        f.f = c;
        return nv::half_from_float( f.u );
    }

    inline float to_float(uint16 c) {
        union { float f; uint32 u; } f;
        f.u = nv::fast_half_to_float( c );
        return f.f;
    }


    union Half {
        uint16 raw;
        struct {
        #if NV_BIG_ENDIAN
            uint negative:1;
            uint biasedexponent:5;
            uint mantissa:10;
        #else
            uint mantissa:10;
            uint biasedexponent:5;
            uint negative:1;
        #endif
        } field;
    };


    inline float TestHalfPrecisionAwayFromZero(float input)
    {
        Half h;
        h.raw = to_half(input);
        h.raw += 1;

        float f = to_float(h.raw);
        
        // Subtract the initial value to find our precision
        float delta = f - input;

        return delta;
    }
     
    inline float TestHalfPrecisionTowardsZero(float input)
    {
        Half h;
        h.raw = to_half(input);
        h.raw -= 1;

        float f = to_float(h.raw);

        // Subtract the initial value to find our precision
        float delta = f - input;

        return -delta;
    }

} // nv namespace

#endif // NV_MATH_HALF_H
