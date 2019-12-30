// Copyright (c) 2009-2011 Ignacio Castano <castano@gmail.com>
// Copyright (c) 2007-2009 NVIDIA Corporation -- Ignacio Castano <icastano@nvidia.com>
// 
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#include "CompressorRGB.h"
#include "CompressionOptions.h"
#include "OutputOptions.h"

#include "nvimage/Image.h"
#include "nvimage/FloatImage.h"
#include "nvimage/PixelFormat.h"

#include "nvmath/Color.h"
#include "nvmath/Half.h"
#include "nvmath/ftoi.h"
#include "nvmath/Vector.inl"

#include "nvcore/Debug.h"

using namespace nv;
using namespace nvtt;

namespace 
{
    /* 11 and 10 bit floating point numbers according to the OpenGL packed float extension:
       http://www.opengl.org/registry/specs/EXT/packed_float.txt

       2.1.A  Unsigned 11-Bit Floating-Point Numbers

        An unsigned 11-bit floating-point number has no sign bit, a 5-bit
        exponent (E), and a 6-bit mantissa (M).  The value of an unsigned
        11-bit floating-point number (represented as an 11-bit unsigned
        integer N) is determined by the following:

            0.0,                      if E == 0 and M == 0,
            2^-14 * (M / 64),         if E == 0 and M != 0,
            2^(E-15) * (1 + M/64),    if 0 < E < 31,
            INF,                      if E == 31 and M == 0, or
            NaN,                      if E == 31 and M != 0,

        where

            E = floor(N / 64), and
            M = N mod 64.

        Implementations are also allowed to use any of the following
        alternative encodings:

            0.0,                      if E == 0 and M != 0
            2^(E-15) * (1 + M/64)     if E == 31 and M == 0
            2^(E-15) * (1 + M/64)     if E == 31 and M != 0

        When a floating-point value is converted to an unsigned 11-bit
        floating-point representation, finite values are rounded to the closet
        representable finite value.  While less accurate, implementations
        are allowed to always round in the direction of zero.  This means
        negative values are converted to zero.  Likewise, finite positive
        values greater than 65024 (the maximum finite representable unsigned
        11-bit floating-point value) are converted to 65024.  Additionally:
        negative infinity is converted to zero; positive infinity is converted
        to positive infinity; and both positive and negative NaN are converted
        to positive NaN.

        Any representable unsigned 11-bit floating-point value is legal
        as input to a GL command that accepts 11-bit floating-point data.
        The result of providing a value that is not a floating-point number
        (such as infinity or NaN) to such a command is unspecified, but must
        not lead to GL interruption or termination.  Providing a denormalized
        number or negative zero to GL must yield predictable results.

        2.1.B  Unsigned 10-Bit Floating-Point Numbers

        An unsigned 10-bit floating-point number has no sign bit, a 5-bit
        exponent (E), and a 5-bit mantissa (M).  The value of an unsigned
        10-bit floating-point number (represented as an 10-bit unsigned
        integer N) is determined by the following:

            0.0,                      if E == 0 and M == 0,
            2^-14 * (M / 32),         if E == 0 and M != 0,
            2^(E-15) * (1 + M/32),    if 0 < E < 31,
            INF,                      if E == 31 and M == 0, or
            NaN,                      if E == 31 and M != 0,

        where

            E = floor(N / 32), and
            M = N mod 32.

        When a floating-point value is converted to an unsigned 10-bit
        floating-point representation, finite values are rounded to the closet
        representable finite value.  While less accurate, implementations
        are allowed to always round in the direction of zero.  This means
        negative values are converted to zero.  Likewise, finite positive
        values greater than 64512 (the maximum finite representable unsigned
        10-bit floating-point value) are converted to 64512.  Additionally:
        negative infinity is converted to zero; positive infinity is converted
        to positive infinity; and both positive and negative NaN are converted
        to positive NaN.

        Any representable unsigned 10-bit floating-point value is legal
        as input to a GL command that accepts 10-bit floating-point data.
        The result of providing a value that is not a floating-point number
        (such as infinity or NaN) to such a command is unspecified, but must
        not lead to GL interruption or termination.  Providing a denormalized
        number or negative zero to GL must yield predictable results.
    */

    // @@ Is this correct? Not tested!
    // 6 bits of mantissa, 5 bits of exponent.
    static uint toFloat11(float f) {
        if (f < 0) f = 0;           // Flush to 0 or to epsilon?
        if (f > 65024) f = 65024;   // Flush to infinity or max?

        Float754 F;
        F.value = f;

        uint E = F.field.biasedexponent - 127 + 15;
        nvDebugCheck(E < 32);

        uint M = F.field.mantissa >> (23 - 6);

        return (E << 6) | M;
    }

    // @@ Is this correct? Not tested!
    // 5 bits of mantissa, 5 bits of exponent.
    static uint toFloat10(float f) {
        if (f < 0) f = 0;           // Flush to 0 or to epsilon?
        if (f > 64512) f = 64512;   // Flush to infinity or max?

        Float754 F;
        F.value = f;

        uint E = F.field.biasedexponent - 127 + 15;
        nvDebugCheck(E < 32);

        uint M = F.field.mantissa >> (23 - 5);

        return (E << 5) | M;
    }


    // IC: Inf/NaN and denormal handling based on DirectXMath.
    static float fromFloat11(uint u) {
        // 5 bit exponent
        // 6 bit mantissa
        
        uint E = (u >> 6) & 0x1F;
        uint M = u & 0x3F;

        Float754 F;
        F.field.negative = 0;

        if (E == 0x1f) { // INF or NAN.
            E = 0xFF;
        }
        else {
            if (E != 0) {
                F.field.biasedexponent = E + 127 - 15;
                F.field.mantissa = M << (23 - 6);
            }
            else if (M != 0) {
                E = 1;
                do {
                    E--;
                    M <<= 1;
                } while((M & 0x40) == 0);

                M &= 0x3F;
            }
        }

        F.field.biasedexponent = 0xFF;
        F.field.mantissa = M << (23 - 6);
        
		return F.value;
#if 0
        // X Channel (6-bit mantissa)
        Mantissa = pSource->xm;

        if ( pSource->xe == 0x1f ) // INF or NAN
        {
            Result[0] = 0x7f800000 | (pSource->xm << 17);
        }
        else
        {
            if ( pSource->xe != 0 ) // The value is normalized
            {
                Exponent = pSource->xe;
            }
            else if (Mantissa != 0) // The value is denormalized
            {
                // Normalize the value in the resulting float
                Exponent = 1;
        
                do
                {
                    Exponent--;
                    Mantissa <<= 1;
                } while ((Mantissa & 0x40) == 0);
        
                Mantissa &= 0x3F;
            }
            else // The value is zero
            {
                Exponent = (uint32_t)-112;
            }
    
            Result[0] = ((Exponent + 112) << 23) | (Mantissa << 17);
        }
#endif
    }

    // https://www.opengl.org/registry/specs/EXT/texture_shared_exponent.txt
    Float3SE toFloat3SE(float r, float g, float b)
    {
        const int N = 9;                    // Mantissa bits.
        const int E = 5;                    // Exponent bits.
        const int Emax = (1 << E) - 1;      // 31
        const int B = (1 << (E-1)) - 1;     // 15
        const float sharedexp_max = float((1 << N) - 1) / (1 << N) * (1 << (Emax-B));   // 65408

        // Clamp color components.
        r = max(0.0f, min(sharedexp_max, r));
        g = max(0.0f, min(sharedexp_max, g));
        b = max(0.0f, min(sharedexp_max, b));

        // Get max component.
        float max_c = max3(r, g, b);

        // Compute shared exponent.
        int exp_shared_p = max(-B-1, ftoi_floor(log2f(max_c))) + 1 + B;

        int max_s = ftoi_round(max_c / (1 << (exp_shared_p - B - N)));

        int exp_shared = exp_shared_p;
        if (max_s == (1 << N)) exp_shared++;

        Float3SE v;
        v.e = exp_shared;

        // Compute mantissas.
        v.xm = ftoi_round(r / (1 << (exp_shared - B - N)));
        v.ym = ftoi_round(g / (1 << (exp_shared - B - N)));
        v.zm = ftoi_round(b / (1 << (exp_shared - B - N)));

        return v;
    }

    Vector3 fromFloat3SE(Float3SE v) {
        Float754 f;
        f.raw = 0x33800000 + (v.e << 23);
        float scale = f.value;
        return scale * Vector3(float(v.xm), float(v.ym), float(v.zm));
    }

    // These are based on: http://www.graphics.cornell.edu/~bjw/rgbe/rgbe.c
    uint toRGBE(float r, float g, float b)
    {
        float v = max3(r, g, b);

        uint rgbe;

        if (v < 1e-32) {
            rgbe = 0;
        }
        else {
            int e;
            float scale = frexpf(v, &e) * 256.0f / v;
            //Float754 f;
            //f.value = v;
            //float scale = f.field.biasedexponent * 256.0f / v;
            //e = f.field.biasedexponent - 127

            rgbe |= U8(ftoi_round(r * scale)) << 0;
            rgbe |= U8(ftoi_round(g * scale)) << 8;
            rgbe |= U8(ftoi_round(b * scale)) << 16;
            rgbe |= U8(e + 128) << 24;
        }

        return rgbe;
    }

    Vector3 fromRGBE(uint rgbe) {
        uint r = (rgbe >> 0) & 0xFF;
        uint g = (rgbe >> 8) & 0xFF;
        uint b = (rgbe >> 16) & 0xFF;
        uint e = (rgbe >> 24);

        if (e != 0) {
            float scale = ldexpf(1.0f, e-(int)(128+8));             // +8 to divide by 256. @@ Shouldn't we divide by 255 instead?
            return scale * Vector3(float(r), float(g), float(b));
        }
        
        return Vector3(0);
    }


    struct BitStream
    {
        BitStream(uint8 * ptr) : ptr(ptr), buffer(0), bits(0) {
        }

        void putBits(uint p, int bitCount)
        {
            nvDebugCheck(bits < 8);
            nvDebugCheck(bitCount <= 32);

            uint64 buffer = (this->buffer << bitCount) | p;
            uint bits = this->bits + bitCount;

            while (bits >= 8)
            {
                *ptr++ = (buffer & 0xFF);
                
                buffer >>= 8;
                bits -= 8;
            }

            this->buffer = (uint8)buffer;
            this->bits = bits;
        }

        void putFloat(float f)
        {
            nvDebugCheck(bits == 0); // @@ Do not require alignment.
            *((float *)ptr) = f;
            ptr += 4;
        }

        void putHalf(float f)
        {
            nvDebugCheck(bits == 0); // @@ Do not require alignment.
            *((uint16 *)ptr) = to_half(f);
            ptr += 2;
        }

        void putFloat11(float f)
        {
            putBits(toFloat11(f), 11);
        }

        void putFloat10(float f)
        {
            putBits(toFloat10(f), 10);
        }

        void flush()
        {
            nvDebugCheck(bits < 8);
            if (bits) {
                *ptr++ = buffer;
                buffer = 0;
                bits = 0;
            }
        }

        void align(int alignment)
        {
            nvDebugCheck(alignment >= 1);
            flush();
            int remainder = (int)((uintptr_t)ptr % alignment);
            if (remainder != 0) {
                putBits(0, (alignment - remainder) * 8);
            }
        }

        uint8 * ptr;
        uint8 buffer;
        uint8 bits;
    };

} // namespace



void PixelFormatConverter::compress(nvtt::AlphaMode /*alphaMode*/, uint w, uint h, uint d, const float * data, nvtt::TaskDispatcher * dispatcher, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions)
{
    nvDebugCheck (compressionOptions.format == nvtt::Format_RGBA);

    uint bitCount;
    uint rmask, rshift, rsize;
    uint gmask, gshift, gsize;
    uint bmask, bshift, bsize;
    uint amask, ashift, asize;

    if (compressionOptions.pixelType == nvtt::PixelType_Float)
    {
        rsize = compressionOptions.rsize;
        gsize = compressionOptions.gsize;
        bsize = compressionOptions.bsize;
        asize = compressionOptions.asize;

        // Other float sizes are not supported and will be zero-padded.
        nvDebugCheck(rsize == 0 || rsize == 10 || rsize == 11 || rsize == 16 || rsize == 32);
        nvDebugCheck(gsize == 0 || gsize == 10 || gsize == 11 || gsize == 16 || gsize == 32);
        nvDebugCheck(bsize == 0 || bsize == 10 || bsize == 11 || bsize == 16 || bsize == 32);
        nvDebugCheck(asize == 0 || asize == 10 || asize == 11 || asize == 16 || asize == 32);

        bitCount = rsize + gsize + bsize + asize;
    }
    else
    {
        if (compressionOptions.bitcount != 0)
        {
            bitCount = compressionOptions.bitcount;
            nvCheck(bitCount <= 32);

            rmask = compressionOptions.rmask;
            gmask = compressionOptions.gmask;
            bmask = compressionOptions.bmask;
            amask = compressionOptions.amask;

            PixelFormat::maskShiftAndSize(rmask, &rshift, &rsize);
            PixelFormat::maskShiftAndSize(gmask, &gshift, &gsize);
            PixelFormat::maskShiftAndSize(bmask, &bshift, &bsize);
            PixelFormat::maskShiftAndSize(amask, &ashift, &asize);
        }
        else
        {
            rsize = compressionOptions.rsize;
            gsize = compressionOptions.gsize;
            bsize = compressionOptions.bsize;
            asize = compressionOptions.asize;

            bitCount = rsize + gsize + bsize + asize;
            nvCheck(bitCount <= 32);

            ashift = 0;
            bshift = ashift + asize;
            gshift = bshift + bsize;
            rshift = gshift + gsize;

            rmask = ((1 << rsize) - 1) << rshift;
            gmask = ((1 << gsize) - 1) << gshift;
            bmask = ((1 << bsize) - 1) << bshift;
            amask = ((1 << asize) - 1) << ashift;
        }
    }

    const uint pitch = computeBytePitch(w, bitCount, compressionOptions.pitchAlignment);
    const uint whd = w * h * d;

    // Allocate output scanline.
    uint8 * const dst = malloc<uint8>(pitch);

    for (uint z = 0; z < d; z++)
    {
        for (uint y = 0; y < h; y++)
        {
            const float * src = (const float *)data + (z * h + y) * w;

            BitStream stream(dst);

            for (uint x = 0; x < w; x++)
            {
                float r = src[x + 0 * whd];
                float g = src[x + 1 * whd];
                float b = src[x + 2 * whd];
                float a = src[x + 3 * whd];

                if (compressionOptions.pixelType == nvtt::PixelType_Float)
                {
                    if (rsize == 32) stream.putFloat(r);
                    else if (rsize == 16) stream.putHalf(r);
                    else if (rsize == 11) stream.putFloat11(r);
                    else if (rsize == 10) stream.putFloat10(r);
                    else stream.putBits(0, rsize);

                    if (gsize == 32) stream.putFloat(g);
                    else if (gsize == 16) stream.putHalf(g);
                    else if (gsize == 11) stream.putFloat11(g);
                    else if (gsize == 10) stream.putFloat10(g);
                    else stream.putBits(0, gsize);

                    if (bsize == 32) stream.putFloat(b);
                    else if (bsize == 16) stream.putHalf(b);
                    else if (bsize == 11) stream.putFloat11(b);
                    else if (bsize == 10) stream.putFloat10(b);
                    else stream.putBits(0, bsize);

                    if (asize == 32) stream.putFloat(a);
                    else if (asize == 16) stream.putHalf(a);
                    else if (asize == 11) stream.putFloat11(a);
                    else if (asize == 10) stream.putFloat10(a);
                    else stream.putBits(0, asize);
                }
                else if (compressionOptions.pixelType == nvtt::PixelType_SharedExp)
                {
                    if (rsize == 9 && gsize == 9 && bsize == 9 && asize == 5) {
                        Float3SE v = toFloat3SE(r, g, b);
                        stream.putBits(v.v, 32);
                    }
                    else if (rsize == 8 && gsize == 8 && bsize == 8 && asize == 8) {
                        // @@ 
                    }
                    else {
                        // @@ Not supported. Filling with zeros.
                        stream.putBits(0, bitCount);
                    }
                }
                else
                {
                    // We first convert to 16 bits, then to the target size. @@ If greater than 16 bits, this will truncate and bitexpand.
                    
                    // @@ Add support for nvtt::PixelType_SignedInt, nvtt::PixelType_SignedNorm, nvtt::PixelType_UnsignedInt

                    int ir, ig, ib, ia;
                    if (compressionOptions.pixelType == nvtt::PixelType_UnsignedNorm) {
                        ir = iround(clamp(r * 65535.0f, 0.0f, 65535.0f));
                        ig = iround(clamp(g * 65535.0f, 0.0f, 65535.0f));
                        ib = iround(clamp(b * 65535.0f, 0.0f, 65535.0f));
                        ia = iround(clamp(a * 65535.0f, 0.0f, 65535.0f));
                    }
                    else if (compressionOptions.pixelType == nvtt::PixelType_SignedNorm) {
                        // @@
                    }
                    else if (compressionOptions.pixelType == nvtt::PixelType_UnsignedInt) {
                        ir = iround(clamp(r, 0.0f, 65535.0f));
                        ig = iround(clamp(g, 0.0f, 65535.0f));
                        ib = iround(clamp(b, 0.0f, 65535.0f));
                        ia = iround(clamp(a, 0.0f, 65535.0f));
                    }
                    else if (compressionOptions.pixelType == nvtt::PixelType_SignedInt) {
                        // @@
                    }
                    
                    uint p = 0;
                    p |= PixelFormat::convert(ir, 16, rsize) << rshift;
                    p |= PixelFormat::convert(ig, 16, gsize) << gshift;
                    p |= PixelFormat::convert(ib, 16, bsize) << bshift;
                    p |= PixelFormat::convert(ia, 16, asize) << ashift;

                    stream.putBits(p, bitCount);
                }
            }

            // Zero padding.
            stream.align(compressionOptions.pitchAlignment);
            nvDebugCheck(stream.ptr == dst + pitch);

            // Scanlines are always byte-aligned.
            outputOptions.writeData(dst, pitch);
        }
    }

    free(dst);
}
