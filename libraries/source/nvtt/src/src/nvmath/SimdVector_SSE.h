/* -----------------------------------------------------------------------------

	Copyright (c) 2006 Simon Brown                          si@sjbrown.co.uk

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files (the 
	"Software"), to	deal in the Software without restriction, including
	without limitation the rights to use, copy, modify, merge, publish,
	distribute, sublicense, and/or sell copies of the Software, and to 
	permit persons to whom the Software is furnished to do so, subject to 
	the following conditions:

	The above copyright notice and this permission notice shall be included
	in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
	CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	
   -------------------------------------------------------------------------- */
   
#ifndef NV_SIMD_VECTOR_SSE_H
#define NV_SIMD_VECTOR_SSE_H

#include "nvcore/Memory.h"

#include <xmmintrin.h>
#if (NV_USE_SSE > 1)
#include <emmintrin.h>
#endif

// See this for ideas:
// http://molecularmusings.wordpress.com/2011/10/18/simdifying-multi-platform-math/


namespace nv {

#define NV_SIMD_NATIVE NV_FORCEINLINE
#define NV_SIMD_INLINE inline

    class SimdVector
    {
    public:
        __m128 vec;

        typedef SimdVector const& Arg;

        NV_SIMD_NATIVE SimdVector() {}
        
        NV_SIMD_NATIVE explicit SimdVector(__m128 v) : vec(v) {}
        
        NV_SIMD_NATIVE explicit SimdVector(float f) {
            vec = _mm_set1_ps(f);
        }

        NV_SIMD_NATIVE explicit SimdVector(const float * v)
        {
            vec = _mm_load_ps( v );
        }

        NV_SIMD_NATIVE SimdVector(float x, float y, float z, float w)
        {
            vec = _mm_setr_ps( x, y, z, w );
        }

        NV_SIMD_NATIVE SimdVector(const SimdVector & arg) : vec(arg.vec) {}

        NV_SIMD_NATIVE SimdVector & operator=(const SimdVector & arg)
        {
            vec = arg.vec;
            return *this;
        }

        NV_SIMD_INLINE float toFloat() const 
        {
            NV_ALIGN_16 float f;
            _mm_store_ss(&f, vec);
            return f;
        }

        NV_SIMD_INLINE Vector3 toVector3() const
        {
            NV_ALIGN_16 float c[4];
            _mm_store_ps( c, vec );
            return Vector3( c[0], c[1], c[2] );
        }

        NV_SIMD_INLINE Vector4 toVector4() const
        {
            NV_ALIGN_16 float c[4];
            _mm_store_ps( c, vec );
            return Vector4( c[0], c[1], c[2], c[3] );
        }

#define SSE_SPLAT( a ) ((a) | ((a) << 2) | ((a) << 4) | ((a) << 6))
        NV_SIMD_NATIVE SimdVector splatX() const { return SimdVector( _mm_shuffle_ps( vec, vec, SSE_SPLAT( 0 ) ) ); }
        NV_SIMD_NATIVE SimdVector splatY() const { return SimdVector( _mm_shuffle_ps( vec, vec, SSE_SPLAT( 1 ) ) ); }
        NV_SIMD_NATIVE SimdVector splatZ() const { return SimdVector( _mm_shuffle_ps( vec, vec, SSE_SPLAT( 2 ) ) ); }
        NV_SIMD_NATIVE SimdVector splatW() const { return SimdVector( _mm_shuffle_ps( vec, vec, SSE_SPLAT( 3 ) ) ); }
#undef SSE_SPLAT

        NV_SIMD_NATIVE SimdVector& operator+=( Arg v )
        {
            vec = _mm_add_ps( vec, v.vec );
            return *this;
        }

        NV_SIMD_NATIVE SimdVector& operator-=( Arg v )
        {
            vec = _mm_sub_ps( vec, v.vec );
            return *this;
        }

        NV_SIMD_NATIVE SimdVector& operator*=( Arg v )
        {
            vec = _mm_mul_ps( vec, v.vec );
            return *this;
        }
    };


    NV_SIMD_NATIVE SimdVector operator+( SimdVector::Arg left, SimdVector::Arg right  )
    {
        return SimdVector( _mm_add_ps( left.vec, right.vec ) );
    }

    NV_SIMD_NATIVE SimdVector operator-( SimdVector::Arg left, SimdVector::Arg right  )
    {
        return SimdVector( _mm_sub_ps( left.vec, right.vec ) );
    }

    NV_SIMD_NATIVE SimdVector operator*( SimdVector::Arg left, SimdVector::Arg right  )
    {
        return SimdVector( _mm_mul_ps( left.vec, right.vec ) );
    }

    // Returns a*b + c
    NV_SIMD_INLINE SimdVector multiplyAdd( SimdVector::Arg a, SimdVector::Arg b, SimdVector::Arg c )
    {
        return SimdVector( _mm_add_ps( _mm_mul_ps( a.vec, b.vec ), c.vec ) );
    }

    // Returns -( a*b - c )
    NV_SIMD_INLINE SimdVector negativeMultiplySubtract( SimdVector::Arg a, SimdVector::Arg b, SimdVector::Arg c )
    {
        return SimdVector( _mm_sub_ps( c.vec, _mm_mul_ps( a.vec, b.vec ) ) );
    }

    NV_SIMD_INLINE SimdVector reciprocal( SimdVector::Arg v )
    {
        // get the reciprocal estimate
        __m128 estimate = _mm_rcp_ps( v.vec );

        // one round of Newton-Rhaphson refinement
        __m128 diff = _mm_sub_ps( _mm_set1_ps( 1.0f ), _mm_mul_ps( estimate, v.vec ) );
        return SimdVector( _mm_add_ps( _mm_mul_ps( diff, estimate ), estimate ) );
    }

    NV_SIMD_NATIVE SimdVector min( SimdVector::Arg left, SimdVector::Arg right )
    {
        return SimdVector( _mm_min_ps( left.vec, right.vec ) );
    }

    NV_SIMD_NATIVE SimdVector max( SimdVector::Arg left, SimdVector::Arg right )
    {
        return SimdVector( _mm_max_ps( left.vec, right.vec ) );
    }

    NV_SIMD_INLINE SimdVector truncate( SimdVector::Arg v )
    {
#if (NV_USE_SSE == 1)
        // convert to ints
        __m128 input = v.vec;
        __m64 lo = _mm_cvttps_pi32( input );
        __m64 hi = _mm_cvttps_pi32( _mm_movehl_ps( input, input ) );

        // convert to floats
        __m128 part = _mm_movelh_ps( input, _mm_cvtpi32_ps( input, hi ) );
        __m128 truncated = _mm_cvtpi32_ps( part, lo );

        // clear out the MMX multimedia state to allow FP calls later
        _mm_empty(); 
        return SimdVector( truncated );
#else
        // use SSE2 instructions
        return SimdVector( _mm_cvtepi32_ps( _mm_cvttps_epi32( v.vec ) ) );
#endif
    }

    NV_SIMD_NATIVE SimdVector compareEqual( SimdVector::Arg left, SimdVector::Arg right )
    {
        return SimdVector( _mm_cmpeq_ps( left.vec, right.vec ) );
    }

    NV_SIMD_INLINE SimdVector select( SimdVector::Arg off, SimdVector::Arg on, SimdVector::Arg bits )
    {
        __m128 a = _mm_andnot_ps( bits.vec, off.vec );
        __m128 b = _mm_and_ps( bits.vec, on.vec );

        return SimdVector( _mm_or_ps( a, b ) );
    }

    NV_SIMD_INLINE bool compareAnyLessThan( SimdVector::Arg left, SimdVector::Arg right ) 
    {
        __m128 bits = _mm_cmplt_ps( left.vec, right.vec );
        int value = _mm_movemask_ps( bits );
        return value != 0;
    }

} // namespace nv

#endif // NV_SIMD_VECTOR_SSE_H
