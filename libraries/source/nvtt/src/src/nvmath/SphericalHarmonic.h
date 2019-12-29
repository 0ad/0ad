// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_MATH_SPHERICALHARMONIC_H
#define NV_MATH_SPHERICALHARMONIC_H

#include "nvmath.h"

#include <string.h> // memcpy


namespace nv
{
    class Vector3;
    class Matrix;

    NVMATH_API float legendrePolynomial( int l, int m, float x ) NV_CONST;
    NVMATH_API float shBasis( int l, int m, float theta, float phi ) NV_CONST;
    NVMATH_API float shBasis( int l, int m, const Vector3 & v ) NV_CONST;
    NVMATH_API float hshBasis( int l, int m, float theta, float phi ) NV_CONST;
    NVMATH_API float hshBasis( int l, int m, const Vector3 & v ) NV_CONST;

    class Sh;
    float dot(const Sh & a, const Sh & b) NV_CONST;


    /// Spherical harmonic class.
    class Sh
    {
        friend class Sh2;
        friend class ShMatrix;
    public:

        /// Construct a spherical harmonic of the given order.
        Sh(int o) : m_order(o)
        {
            m_elemArray = new float[basisNum()];
        }

        /// Copy constructor.
        Sh(const Sh & sh) : m_order(sh.order())
        {
            m_elemArray = new float[basisNum()];
            memcpy(m_elemArray, sh.m_elemArray, sizeof(float) * basisNum());
        }

        /// Destructor.
        ~Sh()
        {
            delete [] m_elemArray;
            m_elemArray = NULL;
        }

        /// Get number of bands.
        static int bandNum(int m_order) {
            return m_order + 1;
        }

        /// Get number of sh basis.
        static int basisNum(int m_order) {
            return (m_order + 1) * (m_order + 1);
        }

        /// Get the index for the given coefficients.
        static int index( int l, int m ) {
            return l * l + l + m;
        }

        /// Get sh order.
        int order() const
        {
            return m_order;
        }

        /// Get sh order.
        int bandNum() const
        {
            return bandNum(m_order);
        }

        /// Get sh order.
        int basisNum() const
        {
            return basisNum(m_order);
        }

        /// Get sh coefficient indexed by l,m.
        float elem( int l, int m ) const
        {
            return m_elemArray[index(l, m)];
        }

        /// Get sh coefficient indexed by l,m.
        float & elem( int l, int m )
        {
            return m_elemArray[index(l, m)];
        }


        /// Get sh coefficient indexed by i.
        float elemAt( int i ) const {
            return m_elemArray[i];
        }

        /// Get sh coefficient indexed by i.
        float & elemAt( int i )
        {
            return m_elemArray[i];
        }


        /// Reset the sh coefficients.
        void reset()
        {
            for( int i = 0; i < basisNum(); i++ ) {
                m_elemArray[i] = 0.0f;
            }
        }

        /// Copy spherical harmonic.
        void operator= ( const Sh & sh )
        {
            nvDebugCheck(order() <= sh.order());

            for(int i = 0; i < basisNum(); i++) {
                m_elemArray[i] = sh.m_elemArray[i];
            }
        }

        /// Add spherical harmonics.
        void operator+= ( const Sh & sh )
        {
            nvDebugCheck(order() == sh.order());

            for(int i = 0; i < basisNum(); i++) {
                m_elemArray[i] += sh.m_elemArray[i];
            }
        }

        /// Substract spherical harmonics.
        void operator-= ( const Sh & sh )
        {
            nvDebugCheck(order() == sh.order());

            for(int i = 0; i < basisNum(); i++) {
                m_elemArray[i] -= sh.m_elemArray[i];
            }
        }

        // Not exactly convolution, nor product.
        void operator*= ( const Sh & sh )
        {
            nvDebugCheck(order() == sh.order());

            for(int i = 0; i < basisNum(); i++) {
                m_elemArray[i] *= sh.m_elemArray[i];
            }
        }

        /// Scale spherical harmonics.
        void operator*= ( float f )
        {
            for(int i = 0; i < basisNum(); i++) {
                m_elemArray[i] *= f;
            }
        }

        /// Add scaled spherical harmonics.
        void addScaled( const Sh & sh, float f )
        {
            nvDebugCheck(order() == sh.order());

            for(int i = 0; i < basisNum(); i++) {
                m_elemArray[i] += sh.m_elemArray[i] * f;
            }
        }


        /*/// Add a weighted sample to the sh coefficients.
        void AddSample( const Vec3 & dir, const Color3f & color, float w=1.0f ) {
        for(int l = 0; l <= order; l++) {
        for(int m = -l; m <= l; m++) {
        Color3f & elem = GetElem(l, m);
        elem.Mad( elem, color, w * shBasis(l, m, dir) );
        }
        }
        }*/

        /// Evaluate 
        void eval(const Vector3 & dir)
        {
            for(int l = 0; l <= m_order; l++) {
                for(int m = -l; m <= l; m++) {
                    elem(l, m) = shBasis(l, m, dir);
                }
            }
        }


        /// Evaluate the spherical harmonic function.
        float sample(const Vector3 & dir) const
        {
            Sh sh(order());
            sh.eval(dir);

            return dot(sh, *this);
        }


    protected:

        const int m_order;
        float * m_elemArray;

    };


    /// Compute dot product of the spherical harmonics.
    inline float dot(const Sh & a, const Sh & b)
    {
        nvDebugCheck(a.order() == b.order());

        float sum = 0;
        for( int i = 0; i < Sh::basisNum(a.order()); i++ ) {
            sum += a.elemAt(i) * b.elemAt(i);
        }

        return sum;
    }


    /// Second order spherical harmonic.
    class Sh2 : public Sh
    {
    public:

        /// Constructor.
        Sh2() : Sh(2) {}

        /// Copy constructor.
        Sh2(const Sh2 & sh) : Sh(sh) {}

        /// Spherical harmonic resulting from projecting the clamped cosine transfer function to the SH basis.
        void cosineTransfer()
        {
            const float c1 = 0.282095f;	// K(0, 0)
            const float c2 = 0.488603f; // K(1, 0)
            const float c3 = 1.092548f; // sqrt(15.0f / PI) / 2.0f = K(2, -2)
            const float c4 = 0.315392f; // sqrt(5.0f / PI) / 4.0f) = K(2, 0)
            const float c5 = 0.546274f; // sqrt(15.0f / PI) / 4.0f) = K(2, 2)

            const float normalization = PI * 16.0f / 17.0f;

            const float const1 = c1 * normalization * 1.0f;
            const float const2 = c2 * normalization * (2.0f / 3.0f);
            const float const3 = c3 * normalization * (1.0f / 4.0f);
            const float const4 = c4 * normalization * (1.0f / 4.0f);
            const float const5 = c5 * normalization * (1.0f / 4.0f);

            m_elemArray[0] = const1;

            m_elemArray[1] = -const2;
            m_elemArray[2] = const2;
            m_elemArray[3] = -const2;

            m_elemArray[4] = const3;
            m_elemArray[5] = -const3;
            m_elemArray[6] = const4;
            m_elemArray[7] = -const3;
            m_elemArray[8] = const5;
        }
    };



    /// Spherical harmonic matrix.
    class ShMatrix
    {
    public:

        /// Create an identity matrix of the given order.
        ShMatrix(int o = 2) : m_order(o), m_identity(true)
        {
            nvCheck(m_order > 0);
            m_e = new float[size()];
            m_band = new float *[bandNum()];		
            setupBands();
        }

        /// Destroy and free matrix elements.
        ~ShMatrix()
        {
            delete m_e;
            delete m_band;
        }

        /// Set identity matrix.
        void setIdentity()
        {
            m_identity = true;
        }

        /// Return true if this is an identity matrix, false in other case.
        bool isIdentity() const {
            return m_identity;
        }

        /// Get number of bands of this matrix.
        int bandNum() const
        {
            return m_order+1;
        }

        /// Get total number of elements in the matrix.
        int size() const
        {
            int size = 0;
            for (int i = 0; i < bandNum(); i++) {
                size += square(i * 2 + 1);
            }
            return size;
        }

        /// Get element at the given raw index.
        float element(int idx) const
        {
            return m_e[idx];
        }

        /// Get element at the given with the given indices.
        float & element(int b, int x, int y)
        {
            nvDebugCheck(b >= 0);
            nvDebugCheck(b < bandNum());
            return m_band[b][(b + y) * (b * 2 + 1) + (b + x)];
        }

        /// Get element at the given with the given indices.
        float element(int b, int x, int y) const
        {
            nvDebugCheck(b >= 0);
            nvDebugCheck(b < bandNum());
            return m_band[b][(b + y) * (b * 2 + 1) + (b + x)];
        }

        /// Copy matrix.
        void copy(const ShMatrix & m)
        {
            nvDebugCheck(m_order == m.m_order);
            memcpy(m_e, m.m_e, size() * sizeof(float));
        }

        /// Rotate the given coefficients.
        /*void transform( const Sh & restrict source,  Sh * restrict dest ) const {
            nvCheck( &source != dest );	// Make sure there's no aliasing.
            nvCheck( dest->m_order <= m_order );
            nvCheck( m_order <= source.m_order );

            if (m_identity) {
                *dest = source;
                return;
            }

            // Loop through each band.
            for (int l = 0; l <= dest->m_order; l++) {

                for (int mo = -l; mo <= l; mo++) {

                    Color3f rgb = Color3f::Black;

                    for( int mi = -l; mi <= l; mi++ ) {
                        rgb.Mad( rgb, source.elem(l, mi), elem(l, mo, mi) );
                    }

                    dest->elem(l, mo) = rgb;
                }
            }
        }*/


        NVMATH_API void multiply( const ShMatrix &A, const ShMatrix &B );
        NVMATH_API void rotation( const Matrix & m );
        NVMATH_API void rotation( int axis, float angles );
        NVMATH_API void print();


    private:

        // @@ These could be static indices precomputed only once.
        /// Setup the band pointers.
        void setupBands()
        {
            int size = 0;
            for( int i = 0; i < bandNum(); i++ ) {
                m_band[i] = &m_e[size];
                size += square(i * 2 + 1);
            }
        }


    private:

        // Matrix order.
        const int m_order;

        // Identity flag for quick transform.
        bool m_identity;

        // Array of elements.
        float * m_e;

        // Band pointers.
        float ** m_band;

    };


} // nv namespace

#endif // NV_MATH_SPHERICALHARMONIC_H
