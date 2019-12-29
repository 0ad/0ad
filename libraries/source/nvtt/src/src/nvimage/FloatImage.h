// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_IMAGE_FLOATIMAGE_H
#define NV_IMAGE_FLOATIMAGE_H

#include "nvimage.h"

#include "nvmath/nvmath.h" // lerp

#include "nvcore/Debug.h"
#include "nvcore/Utils.h" // clamp

#include <stdlib.h> // abs


namespace nv
{
    class Vector4;
    class Matrix;
    class Image;
    class Filter;
    class Kernel1;
    class Kernel2;
    class PolyphaseKernel;

    /// Multicomponent floating point image class.
    class FloatImage
    {
    public:

        enum WrapMode {
            WrapMode_Clamp,
            WrapMode_Repeat,
            WrapMode_Mirror
        };

        NVIMAGE_API FloatImage();
        NVIMAGE_API FloatImage(const Image * img);
        NVIMAGE_API virtual ~FloatImage();

        /** @name Conversion. */
        //@{
        NVIMAGE_API void initFrom(const Image * img);
        NVIMAGE_API Image * createImage(uint base_component = 0, uint num = 4) const;
        NVIMAGE_API Image * createImageGammaCorrect(float gamma = 2.2f) const;
        //@}

        /** @name Allocation. */
        //@{
        NVIMAGE_API void allocate(uint c, uint w, uint h, uint d = 1);
        NVIMAGE_API void free(); // Does not clear members.
        NVIMAGE_API void resizeChannelCount(uint c);
        //@}

        /** @name Manipulation. */
        //@{
        NVIMAGE_API void clear(float f = 0.0f);
        NVIMAGE_API void clear(uint component, float f = 0.0f);
        NVIMAGE_API void copyChannel(uint src, uint dst);

        NVIMAGE_API void normalize(uint base_component);

        NVIMAGE_API void packNormals(uint base_component);
        NVIMAGE_API void expandNormals(uint base_component);
        NVIMAGE_API void scaleBias(uint base_component, uint num, float scale, float add);

        NVIMAGE_API void clamp(uint base_component, uint num, float low, float high);

        NVIMAGE_API void toLinear(uint base_component, uint num, float gamma = 2.2f);
        NVIMAGE_API void toGamma(uint base_component, uint num, float gamma = 2.2f);
        NVIMAGE_API void exponentiate(uint base_component, uint num, float power);

        NVIMAGE_API void transform(uint base_component, const Matrix & m, const Vector4 & offset);
        NVIMAGE_API void swizzle(uint base_component, uint r, uint g, uint b, uint a);

        NVIMAGE_API FloatImage * fastDownSample() const;
        NVIMAGE_API FloatImage * downSample(const Filter & filter, WrapMode wm) const;
        NVIMAGE_API FloatImage * downSample(const Filter & filter, WrapMode wm, uint alpha) const;
        NVIMAGE_API FloatImage * resize(const Filter & filter, uint w, uint h, WrapMode wm) const;
        NVIMAGE_API FloatImage * resize(const Filter & filter, uint w, uint h, uint d, WrapMode wm) const;
        NVIMAGE_API FloatImage * resize(const Filter & filter, uint w, uint h, WrapMode wm, uint alpha) const;
        NVIMAGE_API FloatImage * resize(const Filter & filter, uint w, uint h, uint d, WrapMode wm, uint alpha) const;

        NVIMAGE_API void convolve(const Kernel2 & k, uint c, WrapMode wm);

        //NVIMAGE_API FloatImage * downSample(const Kernel1 & filter, WrapMode wm) const;
        //NVIMAGE_API FloatImage * downSample(const Kernel1 & filter, uint w, uint h, WrapMode wm) const;
        //@}

        NVIMAGE_API float applyKernelXY(const Kernel2 * k, int x, int y, int z, uint c, WrapMode wm) const;
        NVIMAGE_API float applyKernelX(const Kernel1 * k, int x, int y, int z, uint c, WrapMode wm) const;
        NVIMAGE_API float applyKernelY(const Kernel1 * k, int x, int y, int z, uint c, WrapMode wm) const;
        NVIMAGE_API float applyKernelZ(const Kernel1 * k, int x, int y, int z, uint c, WrapMode wm) const;
        NVIMAGE_API void applyKernelX(const PolyphaseKernel & k, int y, int z, uint c, WrapMode wm, float * output) const;
        NVIMAGE_API void applyKernelY(const PolyphaseKernel & k, int x, int z, uint c, WrapMode wm, float * output) const;
        NVIMAGE_API void applyKernelZ(const PolyphaseKernel & k, int x, int y, uint c, WrapMode wm, float * output) const;
        NVIMAGE_API void applyKernelX(const PolyphaseKernel & k, int y, int z, uint c, uint a, WrapMode wm, float * output) const;
        NVIMAGE_API void applyKernelY(const PolyphaseKernel & k, int x, int z, uint c, uint a, WrapMode wm, float * output) const;
        NVIMAGE_API void applyKernelZ(const PolyphaseKernel & k, int x, int y, uint c, uint a, WrapMode wm, float * output) const;


        NVIMAGE_API void flipX();
        NVIMAGE_API void flipY();
        NVIMAGE_API void flipZ();

        NVIMAGE_API float alphaTestCoverage(float alphaRef, int alphaChannel, float alphaScale = 1.0f) const;
        NVIMAGE_API void scaleAlphaToCoverage(float coverage, float alphaRef, int alphaChannel);


        uint width() const { return m_width; }
        uint height() const { return m_height; }
        uint depth() const { return m_depth; }
        uint componentCount() const { return m_componentCount; }
        uint floatCount() const { return m_floatCount; }
        uint pixelCount() const { return m_pixelCount; }


        /** @name Pixel access. */
        //@{
        const float * channel(uint c) const;
        float * channel(uint c);

        const float * plane(uint c, uint z) const;
        float * plane(uint c, uint z);

        const float * scanline(uint c, uint y, uint z) const;
        float * scanline(uint c, uint y, uint z);

        //float pixel(uint c, uint x, uint y) const;
        //float & pixel(uint c, uint x, uint y);

        float pixel(uint c, uint x, uint y, uint z) const;
        float & pixel(uint c, uint x, uint y, uint z);

        float pixel(uint c, uint idx) const;
        float & pixel(uint c, uint idx);

        float pixel(uint idx) const;
        float & pixel(uint idx);

        float sampleNearest(uint c, float x, float y, WrapMode wm) const;
        float sampleLinear(uint c, float x, float y, WrapMode wm) const;

        float sampleNearest(uint c, float x, float y, float z, WrapMode wm) const;
        float sampleLinear(uint c, float x, float y, float z, WrapMode wm) const;

        float sampleNearestClamp(uint c, float x, float y) const;
        float sampleNearestRepeat(uint c, float x, float y) const;
        float sampleNearestMirror(uint c, float x, float y) const;

        float sampleNearestClamp(uint c, float x, float y, float z) const;
        float sampleNearestRepeat(uint c, float x, float y, float z) const;
        float sampleNearestMirror(uint c, float x, float y, float z) const;

        NVIMAGE_API float sampleLinearClamp(uint c, float x, float y) const;
        float sampleLinearRepeat(uint c, float x, float y) const;
        float sampleLinearMirror(uint c, float x, float y) const;

        float sampleLinearClamp(uint c, float x, float y, float z) const;
        float sampleLinearRepeat(uint c, float x, float y, float z) const;
        float sampleLinearMirror(uint c, float x, float y, float z) const;
        //@}


        NVIMAGE_API FloatImage* clone() const;

    public:

        uint index(uint x, uint y, uint z) const;
        uint indexClamp(int x, int y, int z) const;
        uint indexRepeat(int x, int y, int z) const;
        uint indexMirror(int x, int y, int z) const;
        uint index(int x, int y, int z, WrapMode wm) const;

        float bilerp(uint c, int ix0, int iy0, int ix1, int iy1, float fx, float fy) const;
        float trilerp(uint c, int ix0, int iy0, int iz0, int ix1, int iy1, int iz1, float fx, float fy, float fz) const;

    public:

        uint16 m_componentCount;
        uint16 m_width;
        uint16 m_height;
        uint16 m_depth;
        uint32 m_pixelCount;
        uint32 m_floatCount;
        float * m_mem;

    };


    /// Get const channel pointer.
    inline const float * FloatImage::channel(uint c) const
    {
        nvDebugCheck(m_mem != NULL);
        nvDebugCheck(c < m_componentCount);
        return m_mem + c * m_pixelCount;
    }

    /// Get channel pointer.
    inline float * FloatImage::channel(uint c) {
        nvDebugCheck(m_mem != NULL);
        nvDebugCheck(c < m_componentCount);
        return m_mem + c * m_pixelCount;
    }

    inline const float * FloatImage::plane(uint c, uint z) const {
        nvDebugCheck(z < m_depth);
        return channel(c) + z * m_width * m_height;        
    }

    inline float * FloatImage::plane(uint c, uint z) {
        nvDebugCheck(z < m_depth);
        return channel(c) + z * m_width * m_height;        
    }

    /// Get const scanline pointer.
    inline const float * FloatImage::scanline(uint c, uint y, uint z) const
    {
        nvDebugCheck(y < m_height);
        return plane(c, z) + y * m_width;
    }

    /// Get scanline pointer.
    inline float * FloatImage::scanline(uint c, uint y, uint z)
    {
        nvDebugCheck(y < m_height);
        return plane(c, z) + y * m_width;
    }

    /// Get pixel component.
    inline float FloatImage::pixel(uint c, uint x, uint y, uint z) const
    {
        nvDebugCheck(m_mem != NULL);
        nvDebugCheck(c < m_componentCount);
        nvDebugCheck(x < m_width);
        nvDebugCheck(y < m_height);
        nvDebugCheck(z < m_depth);
        return m_mem[c * m_pixelCount + index(x, y, z)];
    }

    /// Get pixel component.
    inline float & FloatImage::pixel(uint c, uint x, uint y, uint z)
    {
        nvDebugCheck(m_mem != NULL);
        nvDebugCheck(c < m_componentCount);
        nvDebugCheck(x < m_width);
        nvDebugCheck(y < m_height);
        nvDebugCheck(z < m_depth);
        return m_mem[c * m_pixelCount + index(x, y, z)];
    }

    /// Get pixel component.
    inline float FloatImage::pixel(uint c, uint idx) const
    {
        nvDebugCheck(m_mem != NULL);
        nvDebugCheck(c < m_componentCount);
        nvDebugCheck(idx < m_pixelCount);
        return m_mem[c * m_pixelCount + idx];
    }

    /// Get pixel component.
    inline float & FloatImage::pixel(uint c, uint idx)
    {
        nvDebugCheck(m_mem != NULL);
        nvDebugCheck(c < m_componentCount);
        nvDebugCheck(idx < m_pixelCount);
        return m_mem[c * m_pixelCount + idx];
    }

    /// Get pixel component.
    inline float FloatImage::pixel(uint idx) const
    {
        nvDebugCheck(m_mem != NULL);
        nvDebugCheck(idx < m_floatCount);
        return m_mem[idx];
    }

    /// Get pixel component.
    inline float & FloatImage::pixel(uint idx)
    {
        nvDebugCheck(m_mem != NULL);
        nvDebugCheck(idx < m_floatCount);
        return m_mem[idx];
    }

    inline uint FloatImage::index(uint x, uint y, uint z) const
    {
        nvDebugCheck(x < m_width);
        nvDebugCheck(y < m_height);
        nvDebugCheck(z < m_depth);
        uint idx = (z * m_height + y) * m_width + x;
        nvDebugCheck(idx < m_pixelCount);
        return idx;
    }


    inline int wrapClamp(int x, int w)
    {
        return nv::clamp(x, 0, w - 1);
    }
    inline int wrapRepeat(int x, int w)
    {
        if (x >= 0) return x % w;
        else return (x + 1) % w + w - 1;
    }
    inline int wrapMirror(int x, int w)
    {
        if (w == 1) x = 0;

        x = abs(x);
        while (x >= w) {
            x = abs(w + w - x - 2);
        }

        return x;
    }



    inline uint FloatImage::indexClamp(int x, int y, int z) const
    {
        x = wrapClamp(x, m_width);
        y = wrapClamp(y, m_height);
        z = wrapClamp(z, m_depth);
        return index(x, y, z);
    }


    inline uint FloatImage::indexRepeat(int x, int y, int z) const
    {
        x = wrapRepeat(x, m_width);
        y = wrapRepeat(y, m_height);
        z = wrapRepeat(z, m_depth);
        return index(x, y, z);
   }

    inline uint FloatImage::indexMirror(int x, int y, int z) const
    {
        x = wrapMirror(x, m_width);
        y = wrapMirror(y, m_height);
        z = wrapMirror(z, m_depth);
        return index(x, y, z);
    }

    inline uint FloatImage::index(int x, int y, int z, WrapMode wm) const
    {
        if (wm == WrapMode_Clamp) return indexClamp(x, y, z);
        if (wm == WrapMode_Repeat) return indexRepeat(x, y, z);
        /*if (wm == WrapMode_Mirror)*/ return indexMirror(x, y, z);
    }

    inline float FloatImage::bilerp(uint c, int ix0, int iy0, int ix1, int iy1, float fx, float fy) const {
        int iz = 0;
        float f1 = pixel(c, ix0, iy0, iz);
        float f2 = pixel(c, ix1, iy0, iz);
        float f3 = pixel(c, ix0, iy1, iz);
        float f4 = pixel(c, ix1, iy1, iz);

        float i1 = lerp(f1, f2, fx);
        float i2 = lerp(f3, f4, fx);

        return lerp(i1, i2, fy);
    }

    inline float FloatImage::trilerp(uint c, int ix0, int iy0, int iz0, int ix1, int iy1, int iz1, float fx, float fy, float fz) const {
        float f000 = pixel(c, ix0, iy0, iz0);
        float f100 = pixel(c, ix1, iy0, iz0);
        float f010 = pixel(c, ix0, iy1, iz0);
        float f110 = pixel(c, ix1, iy1, iz0);
        float f001 = pixel(c, ix0, iy0, iz1);
        float f101 = pixel(c, ix1, iy0, iz1);
        float f011 = pixel(c, ix0, iy1, iz1);
        float f111 = pixel(c, ix1, iy1, iz1);

        float i1 = lerp(f000, f001, fz);
        float i2 = lerp(f010, f011, fz);
        float j1 = lerp(f100, f101, fz);
        float j2 = lerp(f110, f111, fz);

        float w1 = lerp(i1, i2, fy);
        float w2 = lerp(j1, j2, fy);

        return lerp(w1, w2, fx);
    }

    // Does not compare channel count.
    inline bool sameLayout(const FloatImage * img0, const FloatImage * img1) {
        if (img0 == NULL || img1 == NULL) return false;
        return img0->width() == img1->width() && img0->height() == img1->height() && img0->depth() == img1->depth();
    }


} // nv namespace



#endif // NV_IMAGE_FLOATIMAGE_H
