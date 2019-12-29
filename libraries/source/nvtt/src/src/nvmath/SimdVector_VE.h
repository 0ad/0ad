/* -----------------------------------------------------------------------------

	Copyright (c) 2006 Simon Brown                          si@sjbrown.co.uk
	Copyright (c) 2016 Raptor Engineering, LLC

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
   
#ifndef NV_SIMD_VECTOR_VE_H
#define NV_SIMD_VECTOR_VE_H

#ifndef __APPLE_ALTIVEC__
#include <altivec.h>
#undef bool
#endif

namespace nv {

    class SimdVector
    {
	public:
        vector float vec;

        typedef SimdVector Arg;

        SimdVector() {}
        explicit SimdVector(float v) : vec(vec_splats(v)) {}	
        explicit SimdVector(vector float v) : vec(v) {}
        SimdVector(const SimdVector & arg) : vec(arg.vec) {}

        SimdVector& operator=(const SimdVector & arg)
        {
            vec = arg.vec;
            return *this;
        }

        SimdVector(const float * v)
        {
            union { vector float v; float c[4]; } u;
            u.c[0] = v[0];
            u.c[1] = v[1];
            u.c[2] = v[2];
            u.c[3] = v[3];
            vec = u.v;
        }

        SimdVector(float x, float y, float z, float w)
        {
            union { vector float v; float c[4]; } u;
            u.c[0] = x;
            u.c[1] = y;
            u.c[2] = z;
            u.c[3] = w;
            vec = u.v;
        }

        float toFloat() const
        {
            union { vector float v; float c[4]; } u;
            u.v = vec;
            return u.c[0];
        }

        Vector3 toVector3() const
        {
            union { vector float v; float c[4]; } u;
            u.v = vec;
            return Vector3( u.c[0], u.c[1], u.c[2] );
        }

        Vector4 toVector4() const
        {
            union { vector float v; float c[4]; } u;
            u.v = vec;
            return Vector4( u.c[0], u.c[1], u.c[2], u.c[3] );
        }

        SimdVector splatX() const { return SimdVector( vec_splat( vec, 0 ) ); }
        SimdVector splatY() const { return SimdVector( vec_splat( vec, 1 ) ); }
        SimdVector splatZ() const { return SimdVector( vec_splat( vec, 2 ) ); }
        SimdVector splatW() const { return SimdVector( vec_splat( vec, 3 ) ); }

        SimdVector& operator+=( Arg v )
        {
            vec = vec_add( vec, v.vec );
            return *this;
        }

        SimdVector& operator-=( Arg v )
        {
            vec = vec_sub( vec, v.vec );
            return *this;
        }

        SimdVector& operator*=( Arg v )
        {
            vec = vec_madd( vec, v.vec, vec_splats( -0.0f ) );
            return *this;
        }
    };

    inline SimdVector operator+( SimdVector::Arg left, SimdVector::Arg right  )
    {
        return SimdVector( vec_add( left.vec, right.vec ) );
    }

    inline SimdVector operator-( SimdVector::Arg left, SimdVector::Arg right  )
    {
        return SimdVector( vec_sub( left.vec, right.vec ) );
    }

    inline SimdVector operator*( SimdVector::Arg left, SimdVector::Arg right  )
    {
        return SimdVector( vec_madd( left.vec, right.vec, vec_splats( -0.0f ) ) );
    }

    // Returns a*b + c
    inline SimdVector multiplyAdd( SimdVector::Arg a, SimdVector::Arg b, SimdVector::Arg c )
    {
        return SimdVector( vec_madd( a.vec, b.vec, c.vec ) );
    }

    // Returns -( a*b - c )
    inline SimdVector negativeMultiplySubtract( SimdVector::Arg a, SimdVector::Arg b, SimdVector::Arg c )
    {
        return SimdVector( vec_nmsub( a.vec, b.vec, c.vec ) );
    }

    inline SimdVector reciprocal( SimdVector::Arg v )
    {
        // get the reciprocal estimate
        vector float estimate = vec_re( v.vec );

        // one round of Newton-Rhaphson refinement
        vector float diff = vec_nmsub( estimate, v.vec, vec_splats( 1.0f ) );
        return SimdVector( vec_madd( diff, estimate, estimate ) );
    }

    inline SimdVector min( SimdVector::Arg left, SimdVector::Arg right )
    {
        return SimdVector( vec_min( left.vec, right.vec ) );
    }

    inline SimdVector max( SimdVector::Arg left, SimdVector::Arg right )
    {
        return SimdVector( vec_max( left.vec, right.vec ) );
    }

    inline SimdVector truncate( SimdVector::Arg v )
    {
        return SimdVector( vec_trunc( v.vec ) );
    }

    inline SimdVector compareEqual( SimdVector::Arg left, SimdVector::Arg right )
    {
        return SimdVector( ( vector float )vec_cmpeq( left.vec, right.vec ) );
    }

    inline SimdVector select( SimdVector::Arg off, SimdVector::Arg on, SimdVector::Arg bits )
    {
        return SimdVector( vec_sel( off.vec, on.vec, ( vector unsigned int )bits.vec ) );
    }

    inline bool compareAnyLessThan( SimdVector::Arg left, SimdVector::Arg right ) 
    {
        return vec_any_lt( left.vec, right.vec ) != 0;
    }

} // namespace nv

#endif // NV_SIMD_VECTOR_VE_H
