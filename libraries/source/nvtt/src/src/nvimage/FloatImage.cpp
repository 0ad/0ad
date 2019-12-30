// This code is in the public domain -- castanyo@yahoo.es

#include "FloatImage.h"
#include "Filter.h"
#include "Image.h"

#include "nvmath/Color.h"
#include "nvmath/Vector.inl"
#include "nvmath/Matrix.inl"
#include "nvmath/ftoi.h"
#include "nvmath/Gamma.h"

#include "nvcore/Utils.h" // max
#include "nvcore/Ptr.h"
#include "nvcore/Memory.h"
#include "nvcore/Array.inl"

#include <math.h>
#include <string.h> // memset, memcpy


using namespace nv;


/// Ctor.
FloatImage::FloatImage() : m_componentCount(0), m_width(0), m_height(0), m_depth(0),
  m_pixelCount(0), m_floatCount(0), m_mem(NULL)
{
}

/// Ctor. Init from image.
FloatImage::FloatImage(const Image * img) : m_componentCount(0), m_width(0), m_height(0), m_depth(0),
    m_pixelCount(0), m_floatCount(0), m_mem(NULL)
{
    initFrom(img);
}

/// Dtor.
FloatImage::~FloatImage()
{
    free();
}


/// Init the floating point image from a regular image.
void FloatImage::initFrom(const Image * img)
{
    nvCheck(img != NULL);

    allocate(4, img->width(), img->height(), img->depth());

    float * red_channel = channel(0);
    float * green_channel = channel(1);
    float * blue_channel = channel(2);
    float * alpha_channel = channel(3);

    const uint count = m_pixelCount;
    for (uint i = 0; i < count; i++) {
        Color32 pixel = img->pixel(i);
        red_channel[i] = float(pixel.r) / 255.0f;
        green_channel[i] = float(pixel.g) / 255.0f;
        blue_channel[i] = float(pixel.b) / 255.0f;
        alpha_channel[i] = float(pixel.a) / 255.0f;
    }
}

/// Convert the floating point image to a regular image.
Image * FloatImage::createImage(uint baseComponent/*= 0*/, uint num/*= 4*/) const
{
    nvCheck(num <= 4);
    nvCheck(baseComponent + num <= m_componentCount);

    AutoPtr<Image> img(new Image());
    img->allocate(m_width, m_height, m_depth);

    for (uint i = 0; i < m_pixelCount; i++) {

        uint c;
        uint8 rgba[4]= {0, 0, 0, 0xff};

        for (c = 0; c < num; c++) {
            float f = pixel(baseComponent + c, i);
            rgba[c] = nv::clamp(int(255.0f * f), 0, 255);
        }

        img->pixel(i) = Color32(rgba[0], rgba[1], rgba[2], rgba[3]);
    }

    return img.release();
}


/// Convert the floating point image to a regular image. Correct gamma of rgb, but not alpha.
Image * FloatImage::createImageGammaCorrect(float gamma/*= 2.2f*/) const
{
    nvCheck(m_componentCount == 4);

    AutoPtr<Image> img(new Image());
    img->allocate(m_width, m_height, m_depth);

    const float * rChannel = this->channel(0);
    const float * gChannel = this->channel(1);
    const float * bChannel = this->channel(2);
    const float * aChannel = this->channel(3);

    const uint count = m_pixelCount;
    for (uint i = 0; i < count; i++)
    {
        const uint8 r = nv::clamp(int(255.0f * pow(rChannel[i], 1.0f/gamma)), 0, 255);
        const uint8 g = nv::clamp(int(255.0f * pow(gChannel[i], 1.0f/gamma)), 0, 255);
        const uint8 b = nv::clamp(int(255.0f * pow(bChannel[i], 1.0f/gamma)), 0, 255);
        const uint8 a = nv::clamp(int(255.0f * aChannel[i]), 0, 255);

        img->pixel(i) = Color32(r, g, b, a);
    }

    return img.release();
}

/// Allocate a 2D float image of the given format and the given extents.
void FloatImage::allocate(uint c, uint w, uint h, uint d)
{
    if (m_componentCount != c || m_width != w || m_height != h || m_depth != d)
    {
        free();

        m_width = w;
        m_height = h;
        m_depth = d;
        m_componentCount = c;
        m_pixelCount = w * h * d;
        m_floatCount = m_pixelCount * c;
        m_mem = malloc<float>(m_floatCount);
    }
}

/// Free the image, but don't clear the members.
void FloatImage::free()
{
    ::free(m_mem);
    m_mem = NULL;
}

void FloatImage::resizeChannelCount(uint c)
{
    if (m_componentCount != c) {
        uint count = m_pixelCount * c;
        m_mem = realloc<float>(m_mem, count);

        if (c > m_componentCount) {
            memset(m_mem + m_floatCount, 0, (count - m_floatCount) * sizeof(float));
        }

        m_componentCount = c;
        m_floatCount = count;
    }
}

void FloatImage::clear(float f/*=0.0f*/)
{
    for (uint i = 0; i < m_floatCount; i++) {
        m_mem[i] = f;
    }
}

void FloatImage::clear(uint c, float f/*= 0.0f*/)
{
    float * channel = this->channel(c);

    const uint count = m_pixelCount;
    for (uint i = 0; i < count; i++) {
        channel[i] = f;
    }
}

void FloatImage::copyChannel(uint src, uint dst)
{
    nvCheck(src < m_componentCount);
    nvCheck(dst < m_componentCount);

    const float * srcChannel = this->channel(src);
    float * dstChannel = this->channel(dst);

    memcpy(dstChannel, srcChannel, sizeof(float)*m_pixelCount);
}

void FloatImage::normalize(uint baseComponent)
{
    nvCheck(baseComponent + 3 <= m_componentCount);

    float * xChannel = this->channel(baseComponent + 0);
    float * yChannel = this->channel(baseComponent + 1);
    float * zChannel = this->channel(baseComponent + 2);

    const uint count = m_pixelCount;
    for (uint i = 0; i < count; i++) {

        Vector3 normal(xChannel[i], yChannel[i], zChannel[i]);
        normal = normalizeSafe(normal, Vector3(0), 0.0f);

        xChannel[i] = normal.x;
        yChannel[i] = normal.y;
        zChannel[i] = normal.z;
    }
}

void FloatImage::packNormals(uint baseComponent)
{
    scaleBias(baseComponent, 3, 0.5f, 0.5f);
}

void FloatImage::expandNormals(uint baseComponent)
{
    scaleBias(baseComponent, 3, 2, -1.0);
}

void FloatImage::scaleBias(uint baseComponent, uint num, float scale, float bias)
{
    const uint size = m_pixelCount;

    for (uint c = 0; c < num; c++) {
        float * ptr = this->channel(baseComponent + c);

        for (uint i = 0; i < size; i++) {
            ptr[i] = scale * ptr[i] + bias;
        }
    }
}

/// Clamp the elements of the image.
void FloatImage::clamp(uint baseComponent, uint num, float low, float high)
{
    const uint size = m_pixelCount;

    for (uint c = 0; c < num; c++) {
        float * ptr = this->channel(baseComponent + c);

        for (uint i = 0; i < size; i++) {
            ptr[i] = nv::clamp(ptr[i], low, high);
        }
    }
}

/// From gamma to linear space.
void FloatImage::toLinear(uint baseComponent, uint num, float gamma /*= 2.2f*/)
{
    if (gamma == 2.2f) {
        for (uint c = 0; c < num; c++) {
            float * ptr = this->channel(baseComponent + c);

            powf_11_5(ptr, ptr, m_pixelCount);
        }
    } else {
        exponentiate(baseComponent, num, gamma);
    }
}

/// From linear to gamma space.
void FloatImage::toGamma(uint baseComponent, uint num, float gamma /*= 2.2f*/)
{
    if (gamma == 2.2f) {
        for (uint c = 0; c < num; c++) {
            float * ptr = this->channel(baseComponent + c);

            powf_5_11(ptr, ptr, m_pixelCount);
        }
    } else {
        exponentiate(baseComponent, num, 1.0f/gamma);
    }
}

/// Exponentiate the elements of the image.
void FloatImage::exponentiate(uint baseComponent, uint num, float power)
{
    const uint size = m_pixelCount;

    for(uint c = 0; c < num; c++) {
        float * ptr = this->channel(baseComponent + c);

        for(uint i = 0; i < size; i++) {
            ptr[i] = powf(max(0.0f, ptr[i]), power);
        }
    }
}

/// Apply linear transform.
void FloatImage::transform(uint baseComponent, const Matrix & m, Vector4::Arg offset)
{
    nvCheck(baseComponent + 4 <= m_componentCount);

    float * r = this->channel(baseComponent + 0);
    float * g = this->channel(baseComponent + 1);
    float * b = this->channel(baseComponent + 2);
    float * a = this->channel(baseComponent + 3);

    const uint size = m_pixelCount;
    for (uint i = 0; i < size; i++)
    {
        Vector4 color = nv::transform(m, Vector4(*r, *g, *b, *a)) + offset;

        *r++ = color.x;
        *g++ = color.y;
        *b++ = color.z;
        *a++ = color.w;
    }
}

void FloatImage::swizzle(uint baseComponent, uint r, uint g, uint b, uint a)
{
    nvCheck(baseComponent + 4 <= m_componentCount);
    nvCheck(r < 7 && g < 7 && b < 7 && a < 7);

    float consts[] = { 1.0f, 0.0f, -1.0f };
    float * c[7];
    c[0] = this->channel(baseComponent + 0);
    c[1] = this->channel(baseComponent + 1);
    c[2] = this->channel(baseComponent + 2);
    c[3] = this->channel(baseComponent + 3);
    c[4] = consts;
    c[5] = consts + 1;
    c[6] = consts + 2;

    const uint size = m_pixelCount;
    for (uint i = 0; i < size; i++)
    {
        float tmp[4] = { *c[r], *c[g], *c[b], *c[a] };

        *c[0]++ = tmp[0];
        *c[1]++ = tmp[1];
        *c[2]++ = tmp[2];
        *c[3]++ = tmp[3];
    }
}

float FloatImage::sampleNearest(uint c, float x, float y, const WrapMode wm) const
{
    if( wm == WrapMode_Clamp ) return sampleNearestClamp(c, x, y);
    else if( wm == WrapMode_Repeat ) return sampleNearestRepeat(c, x, y);
    else /*if( wm == WrapMode_Mirror )*/ return sampleNearestMirror(c, x, y);
}

float FloatImage::sampleLinear(uint c, float x, float y, WrapMode wm) const
{
    if( wm == WrapMode_Clamp ) return sampleLinearClamp(c, x, y);
    else if( wm == WrapMode_Repeat ) return sampleLinearRepeat(c, x, y);
    else /*if( wm == WrapMode_Mirror )*/ return sampleLinearMirror(c, x, y);
}

float FloatImage::sampleNearest(uint c, float x, float y, float z, WrapMode wm) const
{
    if( wm == WrapMode_Clamp ) return sampleNearestClamp(c, x, y, z);
    else if( wm == WrapMode_Repeat ) return sampleNearestRepeat(c, x, y, z);
    else /*if( wm == WrapMode_Mirror )*/ return sampleNearestMirror(c, x, y, z);
}

float FloatImage::sampleLinear(uint c, float x, float y, float z, WrapMode wm) const
{
    if( wm == WrapMode_Clamp ) return sampleLinearClamp(c, x, y, z);
    else if( wm == WrapMode_Repeat ) return sampleLinearRepeat(c, x, y, z);
    else /*if( wm == WrapMode_Mirror )*/ return sampleLinearMirror(c, x, y, z);
}

float FloatImage::sampleNearestClamp(uint c, float x, float y) const
{
    int ix = wrapClamp(iround(x * m_width), m_width);
    int iy = wrapClamp(iround(y * m_height), m_height);
    return pixel(c, ix, iy, 0);
}

float FloatImage::sampleNearestRepeat(uint c, float x, float y) const
{
    int ix = wrapRepeat(iround(x * m_width), m_width);
    int iy = wrapRepeat(iround(y * m_height), m_height);
    return pixel(c, ix, iy, 0);
}

float FloatImage::sampleNearestMirror(uint c, float x, float y) const
{
    int ix = wrapMirror(iround(x * m_width), m_width);
    int iy = wrapMirror(iround(y * m_height), m_height);
    return pixel(c, ix, iy, 0);
}

float FloatImage::sampleNearestClamp(uint c, float x, float y, float z) const
{
    int ix = wrapClamp(iround(x * m_width), m_width);
    int iy = wrapClamp(iround(y * m_height), m_height);
    int iz = wrapClamp(iround(z * m_depth), m_depth);
    return pixel(c, ix, iy, iz);
}

float FloatImage::sampleNearestRepeat(uint c, float x, float y, float z) const
{
    int ix = wrapRepeat(iround(x * m_width), m_width);
    int iy = wrapRepeat(iround(y * m_height), m_height);
    int iz = wrapRepeat(iround(z * m_depth), m_depth);
    return pixel(c, ix, iy, iz);
}

float FloatImage::sampleNearestMirror(uint c, float x, float y, float z) const
{
    int ix = wrapMirror(iround(x * m_width), m_width);
    int iy = wrapMirror(iround(y * m_height), m_height);
    int iz = wrapMirror(iround(z * m_depth), m_depth);
    return pixel(c, ix, iy, iz);
}


float FloatImage::sampleLinearClamp(uint c, float x, float y) const
{
    const int w = m_width;
    const int h = m_height;

    x *= w;
    y *= h;

    const float fracX = frac(x);
    const float fracY = frac(y);

    const int ix0 = ::clamp(ifloor(x), 0, w-1);
    const int iy0 = ::clamp(ifloor(y), 0, h-1);
    const int ix1 = ::clamp(ifloor(x)+1, 0, w-1);
    const int iy1 = ::clamp(ifloor(y)+1, 0, h-1);

    return bilerp(c, ix0, iy0, ix1, iy1, fracX, fracY);
}

float FloatImage::sampleLinearRepeat(uint c, float x, float y) const
{
    const int w = m_width;
    const int h = m_height;

    const float fracX = frac(x * w);
    const float fracY = frac(y * h);

    // @@ Using floor in some places, but round in others?
    int ix0 = ifloor(frac(x) * w);
    int iy0 = ifloor(frac(y) * h);
    int ix1 = ifloor(frac(x + 1.0f/w) * w);
    int iy1 = ifloor(frac(y + 1.0f/h) * h);

    return bilerp(c, ix0, iy0, ix1, iy1, fracX, fracY);
}

float FloatImage::sampleLinearMirror(uint c, float x, float y) const
{
    const int w = m_width;
    const int h = m_height;

    x *= w;
    y *= h;

    const float fracX = frac(x);
    const float fracY = frac(y);

    int ix0 = wrapMirror(iround(x), w);
    int iy0 = wrapMirror(iround(y), h);
    int ix1 = wrapMirror(iround(x) + 1, w);
    int iy1 = wrapMirror(iround(y) + 1, h);

    return bilerp(c, ix0, iy0, ix1, iy1, fracX, fracY);
}

float FloatImage::sampleLinearClamp(uint c, float x, float y, float z) const
{
    const int w = m_width;
    const int h = m_height;
    const int d = m_depth;

    x *= w;
    y *= h;
    z *= d;

    const float fracX = frac(x);
    const float fracY = frac(y);
    const float fracZ = frac(z);

    // @@ Using floor in some places, but round in others?
    const int ix0 = ::clamp(ifloor(x), 0, w-1);
    const int iy0 = ::clamp(ifloor(y), 0, h-1);
    const int iz0 = ::clamp(ifloor(z), 0, h-1);
    const int ix1 = ::clamp(ifloor(x)+1, 0, w-1);
    const int iy1 = ::clamp(ifloor(y)+1, 0, h-1);
    const int iz1 = ::clamp(ifloor(z)+1, 0, h-1);

    return trilerp(c, ix0, iy0, iz0, ix1, iy1, iz1, fracX, fracY, fracZ);
}

float FloatImage::sampleLinearRepeat(uint c, float x, float y, float z) const
{
    const int w = m_width;
    const int h = m_height;
    const int d = m_depth;

    const float fracX = frac(x * w);
    const float fracY = frac(y * h);
    const float fracZ = frac(z * d);

    int ix0 = ifloor(frac(x) * w);
    int iy0 = ifloor(frac(y) * h);
    int iz0 = ifloor(frac(z) * d);
    int ix1 = ifloor(frac(x + 1.0f/w) * w);
    int iy1 = ifloor(frac(y + 1.0f/h) * h);
    int iz1 = ifloor(frac(z + 1.0f/d) * d);

    return trilerp(c, ix0, iy0, iz0, ix1, iy1, iz1, fracX, fracY, fracZ);
}

float FloatImage::sampleLinearMirror(uint c, float x, float y, float z) const
{
    const int w = m_width;
    const int h = m_height;
    const int d = m_depth;

    x *= w;
    y *= h;
    z *= d;

    int ix0 = wrapMirror(iround(x), w);
    int iy0 = wrapMirror(iround(y), h);
    int iz0 = wrapMirror(iround(z), d);
    int ix1 = wrapMirror(iround(x) + 1, w);
    int iy1 = wrapMirror(iround(y) + 1, h);
    int iz1 = wrapMirror(iround(z) + 1, d);

    const float fracX = frac(x);
    const float fracY = frac(y);
    const float fracZ = frac(z);

    return trilerp(c, ix0, iy0, iz0, ix1, iy1, iz1, fracX, fracY, fracZ);
}


/// Fast downsampling using box filter. 
///
/// The extents of the image are divided by two and rounded down.
///
/// When the size of the image is odd, this uses a polyphase box filter as explained in:
/// http://developer.nvidia.com/object/np2_mipmapping.html
///
FloatImage * FloatImage::fastDownSample() const
{
    nvDebugCheck(m_depth == 1);
    nvDebugCheck(m_width != 1 || m_height != 1);

    AutoPtr<FloatImage> dst_image( new FloatImage() );

    const uint w = max(1, m_width / 2);
    const uint h = max(1, m_height / 2);
    dst_image->allocate(m_componentCount, w, h);

    // 1D box filter.
    if (m_width == 1 || m_height == 1)
    {
        const uint n = w * h;

        if ((m_width * m_height) & 1)
        {
            const float scale = 1.0f / (2 * n + 1);

            for(uint c = 0; c < m_componentCount; c++)
            {
                const float * src = this->channel(c);
                float * dst = dst_image->channel(c);

                for(uint x = 0; x < n; x++)
                {
                    const float w0 = float(n - x);
                    const float w1 = float(n - 0);
                    const float w2 = float(1 + x);

                    *dst++ = scale * (w0 * src[0] + w1 * src[1] + w2 * src[2]);
                    src += 2;
                }
            }
        }
        else
        {
            for(uint c = 0; c < m_componentCount; c++)
            {
                const float * src = this->channel(c);
                float * dst = dst_image->channel(c);

                for(uint x = 0; x < n; x++)
                {
                    *dst = 0.5f * (src[0] + src[1]);
                    dst++;
                    src += 2;
                }
            }
        }
    }

    // Regular box filter.
    else if ((m_width & 1) == 0 && (m_height & 1) == 0)
    {
        for(uint c = 0; c < m_componentCount; c++)
        {
            const float * src = this->channel(c);
            float * dst = dst_image->channel(c);

            for(uint y = 0; y < h; y++)
            {
                for(uint x = 0; x < w; x++)
                {
                    *dst = 0.25f * (src[0] + src[1] + src[m_width] + src[m_width + 1]);
                    dst++;
                    src += 2;
                }

                src += m_width;
            }
        }
    }

    // Polyphase filters.
    else if (m_width & 1 && m_height & 1)
    {
        nvDebugCheck(m_width == 2 * w + 1);
        nvDebugCheck(m_height == 2 * h + 1);

        const float scale = 1.0f / (m_width * m_height);

        for(uint c = 0; c < m_componentCount; c++)
        {
            const float * src = this->channel(c);
            float * dst = dst_image->channel(c);

            for(uint y = 0; y < h; y++)
            {
                const float v0 = float(h - y);
                const float v1 = float(h - 0);
                const float v2 = float(1 + y);

                for (uint x = 0; x < w; x++)
                {
                    const float w0 = float(w - x);
                    const float w1 = float(w - 0);
                    const float w2 = float(1 + x);

                    float f = 0.0f;
                    f += v0 * (w0 * src[0 * m_width + 2 * x] + w1 * src[0 * m_width + 2 * x + 1] + w2 * src[0 * m_width + 2 * x + 2]);
                    f += v1 * (w0 * src[1 * m_width + 2 * x] + w1 * src[1 * m_width + 2 * x + 1] + w2 * src[1 * m_width + 2 * x + 2]);
                    f += v2 * (w0 * src[2 * m_width + 2 * x] + w1 * src[2 * m_width + 2 * x + 1] + w2 * src[2 * m_width + 2 * x + 2]);

                    *dst = f * scale;
                    dst++;
                }

                src += 2 * m_width;
            }
        }
    }
    else if (m_width & 1)
    {
        nvDebugCheck(m_width == 2 * w + 1);
        const float scale = 1.0f / (2 * m_width);

        for(uint c = 0; c < m_componentCount; c++)
        {
            const float * src = this->channel(c);
            float * dst = dst_image->channel(c);

            for(uint y = 0; y < h; y++)
            {
                for (uint x = 0; x < w; x++)
                {
                    const float w0 = float(w - x);
                    const float w1 = float(w - 0);
                    const float w2 = float(1 + x);

                    float f = 0.0f;
                    f += w0 * (src[2 * x + 0] + src[m_width + 2 * x + 0]);
                    f += w1 * (src[2 * x + 1] + src[m_width + 2 * x + 1]);
                    f += w2 * (src[2 * x + 2] + src[m_width + 2 * x + 2]);

                    *dst = f * scale;
                    dst++;
                }

                src += 2 * m_width;
            }
        }
    }
    else if (m_height & 1)
    {
        nvDebugCheck(m_height == 2 * h + 1);

        const float scale = 1.0f / (2 * m_height);

        for(uint c = 0; c < m_componentCount; c++)
        {
            const float * src = this->channel(c);
            float * dst = dst_image->channel(c);

            for(uint y = 0; y < h; y++)
            {
                const float v0 = float(h - y);
                const float v1 = float(h - 0);
                const float v2 = float(1 + y);

                for (uint x = 0; x < w; x++)
                {
                    float f = 0.0f;
                    f += v0 * (src[0 * m_width + 2 * x] + src[0 * m_width + 2 * x + 1]);
                    f += v1 * (src[1 * m_width + 2 * x] + src[1 * m_width + 2 * x + 1]);
                    f += v2 * (src[2 * m_width + 2 * x] + src[2 * m_width + 2 * x + 1]);

                    *dst = f * scale;
                    dst++;
                }

                src += 2 * m_width;
            }
        }
    }

    return dst_image.release();
}

/// Downsample applying a 1D kernel separately in each dimension.
FloatImage * FloatImage::downSample(const Filter & filter, WrapMode wm) const
{
    const uint w = max(1, m_width / 2);
    const uint h = max(1, m_height / 2);
    const uint d = max(1, m_depth / 2);

    return resize(filter, w, h, d, wm);
}

/// Downsample applying a 1D kernel separately in each dimension.
FloatImage * FloatImage::downSample(const Filter & filter, WrapMode wm, uint alpha) const
{
    const uint w = max(1, m_width / 2);
    const uint h = max(1, m_height / 2);
    const uint d = max(1, m_depth / 2);

    return resize(filter, w, h, d, wm, alpha);
}


/// Downsample applying a 1D kernel separately in each dimension.
FloatImage * FloatImage::resize(const Filter & filter, uint w, uint h, WrapMode wm) const
{
    // @@ Use monophase filters when frac(m_width / w) == 0

    AutoPtr<FloatImage> tmp_image( new FloatImage() );
    AutoPtr<FloatImage> dst_image( new FloatImage() );	

    PolyphaseKernel xkernel(filter, m_width, w, 32);
    PolyphaseKernel ykernel(filter, m_height, h, 32);

    // @@ Select fastest filtering order:
    //if (w * m_height <= h * m_width)
    {
        tmp_image->allocate(m_componentCount, w, m_height);
        dst_image->allocate(m_componentCount, w, h);

        // @@ We could avoid this allocation, write directly to dst_plane.
        Array<float> tmp_column(h);
        tmp_column.resize(h);

        for (uint c = 0; c < m_componentCount; c++)
        {
            for (uint z = 0; z < m_depth; z++)
            {
                float * tmp_plane = tmp_image->plane(c, z);

                for (uint y = 0; y < m_height; y++) {
                    this->applyKernelX(xkernel, y, z, c, wm, tmp_plane + y * w);
                }

                float * dst_plane = dst_image->plane(c, z);

                for (uint x = 0; x < w; x++) {
                    tmp_image->applyKernelY(ykernel, x, z, c, wm, tmp_column.buffer());

                    // @@ We could avoid this copy, write directly to dst_plane.
                    for (uint y = 0; y < h; y++) {
                        dst_plane[y * w + x] = tmp_column[y];
                    }
                }
            }
        }
    }

    return dst_image.release();
}

/// Downsample applying a 1D kernel separately in each dimension. (for 3d textures)
FloatImage * FloatImage::resize(const Filter & filter, uint w, uint h, uint d, WrapMode wm) const
{
    // @@ Use monophase filters when frac(m_width / w) == 0

    // Use the existing 2d version if we are not resizing in the Z axis:
    if (m_depth == d) {
        return resize(filter, w, h, wm);
    }

    AutoPtr<FloatImage> tmp_image( new FloatImage() );
    AutoPtr<FloatImage> tmp_image2( new FloatImage() );
    AutoPtr<FloatImage> dst_image( new FloatImage() );

    PolyphaseKernel xkernel(filter, m_width, w, 32);
    PolyphaseKernel ykernel(filter, m_height, h, 32);
    PolyphaseKernel zkernel(filter, m_depth, d, 32);

    tmp_image->allocate(m_componentCount, w, m_height, m_depth);
    tmp_image2->allocate(m_componentCount, w, m_height, d);
    dst_image->allocate(m_componentCount, w, h, d);

    Array<float> tmp_column(h);
    tmp_column.resize(h);

    for (uint c = 0; c < m_componentCount; c++)
    {
        float * tmp_channel = tmp_image->channel(c);

        // split width in half
        for (uint z = 0; z < m_depth; z++ ) {
            for (uint y = 0; y < m_height; y++) {
                this->applyKernelX(xkernel, y, z, c, wm, tmp_channel + z * m_height * w + y * w);
            }
        }

        // split depth in half
        float * tmp2_channel = tmp_image2->channel(c);
        for (uint y = 0; y < m_height; y++) {
            for (uint x = 0; x < w; x++) {
                tmp_image->applyKernelZ(zkernel, x, y, c, wm, tmp_column.buffer() );

                for (uint z = 0; z < d; z++) {
                    tmp2_channel[z * m_height * w + y * w + x] = tmp_column[z];
                }
            }
        }

        // split height in half
        float * dst_channel = dst_image->channel(c);

        for (uint z = 0; z < d; z++ ) {
            for (uint x = 0; x < w; x++) {
                tmp_image2->applyKernelY(ykernel, x, z, c, wm, tmp_column.buffer());

                for (uint y = 0; y < h; y++) {
                    dst_channel[z * h * w + y * w + x] = tmp_column[y];
                }
            }
        }
    }

    return dst_image.release();
}


/// Downsample applying a 1D kernel separately in each dimension.
FloatImage * FloatImage::resize(const Filter & filter, uint w, uint h, WrapMode wm, uint alpha) const
{
    nvCheck(alpha < m_componentCount);

    AutoPtr<FloatImage> tmp_image( new FloatImage() );
    AutoPtr<FloatImage> dst_image( new FloatImage() );	

    PolyphaseKernel xkernel(filter, m_width, w, 32);
    PolyphaseKernel ykernel(filter, m_height, h, 32);

    {
        tmp_image->allocate(m_componentCount, w, m_height);
        dst_image->allocate(m_componentCount, w, h);

        Array<float> tmp_column(h);
        tmp_column.resize(h);

        for (uint i = 0; i < m_componentCount; i++)
        {
            // Process alpha channel first.
            uint c;
            if (i == 0) c = alpha;
            else if (i > alpha) c = i;
            else c = i - 1;

            for (uint z = 0; z < m_depth; z++)
            {
                float * tmp_plane = tmp_image->plane(c, z);

                for (uint y = 0; y < m_height; y++) {
                    this->applyKernelX(xkernel, y, z, c, wm, tmp_plane + y * w);
                }

                float * dst_plane = dst_image->plane(c, z);

                for (uint x = 0; x < w; x++) {
                    tmp_image->applyKernelY(ykernel, x, z, c, wm, tmp_column.buffer());

                    // @@ Avoid this copy, write directly to dst_plane.
                    for (uint y = 0; y < h; y++) {
                        dst_plane[y * w + x] = tmp_column[y];
                    }
                }
            }
        }
    }

    return dst_image.release();
}


/// Downsample applying a 1D kernel separately in each dimension. (for 3d textures)
FloatImage * FloatImage::resize(const Filter & filter, uint w, uint h, uint d, WrapMode wm, uint alpha) const
{
    nvCheck(alpha < m_componentCount);

    // use the existing 2d version if we are a 2d image:
    if (m_depth == d) {
        return resize( filter, w, h, wm, alpha );
    }

    AutoPtr<FloatImage> tmp_image( new FloatImage() );
    AutoPtr<FloatImage> tmp_image2( new FloatImage() );
    AutoPtr<FloatImage> dst_image( new FloatImage() );

    PolyphaseKernel xkernel(filter, m_width, w, 32);
    PolyphaseKernel ykernel(filter, m_height, h, 32);
    PolyphaseKernel zkernel(filter, m_depth, d, 32);

    tmp_image->allocate(m_componentCount, w, m_height, m_depth);
    tmp_image2->allocate(m_componentCount, w, m_height, d);
    dst_image->allocate(m_componentCount, w, h, d);

    Array<float> tmp_column(h);
    tmp_column.resize(h);

    for (uint i = 0; i < m_componentCount; i++)
    {
        // Process alpha channel first.
        uint c;
        if (i == 0) c = alpha;
        else if (i > alpha) c = i;
        else c = i - 1;

        float * tmp_channel = tmp_image->channel(c);

        for (uint z = 0; z < m_depth; z++ ) {
            for (uint y = 0; y < m_height; y++) {
                this->applyKernelX(xkernel, y, z, c, wm, tmp_channel + z * m_height * w + y * w);
            }
        }

        float * tmp2_channel = tmp_image2->channel(c);
        for (uint y = 0; y < m_height; y++) {
            for (uint x = 0; x < w; x++) {
                tmp_image->applyKernelZ(zkernel, x, y, c, wm, tmp_column.buffer() );

                for (uint z = 0; z < d; z++) {
                    tmp2_channel[z * m_height * w + y * w + x] = tmp_column[z];
                }
            }
        }

        float * dst_channel = dst_image->channel(c);

        for (uint z = 0; z < d; z++ ) {
            for (uint x = 0; x < w; x++) {
                tmp_image2->applyKernelY(ykernel, x, z, c, wm, tmp_column.buffer());

                for (uint y = 0; y < h; y++) {
                    dst_channel[z * h * w + y * w + x] = tmp_column[y];
                }
            }
        }
    }

    return dst_image.release();
}


void FloatImage::convolve(const Kernel2 & k, uint c, WrapMode wm)
{
    AutoPtr<FloatImage> tmpImage(clone());

    uint w = m_width;
    uint h = m_height;
    uint d = m_depth;

    for (uint z = 0; z < d; z++)
    {
        for (uint y = 0; y < h; y++)
        {
            for (uint x = 0; x < w; x++)
            {
                pixel(c, x, y, 0) = tmpImage->applyKernelXY(&k, x, y, z, c, wm);
            }
        }
    }
}


/// Apply 2D kernel at the given coordinates and return result.
float FloatImage::applyKernelXY(const Kernel2 * k, int x, int y, int z, uint c, WrapMode wm) const
{
    nvDebugCheck(k != NULL);

    const uint kernelWindow = k->windowSize();
    const int kernelOffset = int(kernelWindow / 2);

    const float * channel = this->plane(c, z);

    float sum = 0.0f;
    for (uint i = 0; i < kernelWindow; i++)
    {
        int src_y = int(y + i) - kernelOffset;

        for (uint e = 0; e < kernelWindow; e++)
        {
            int src_x = int(x + e) - kernelOffset;

            int idx = this->index(src_x, src_y, z, wm);

            sum += k->valueAt(e, i) * channel[idx];
        }
    }

    return sum;
}


/// Apply 1D horizontal kernel at the given coordinates and return result.
float FloatImage::applyKernelX(const Kernel1 * k, int x, int y, int z, uint c, WrapMode wm) const
{
    nvDebugCheck(k != NULL);

    const uint kernelWindow = k->windowSize();
    const int kernelOffset = int(kernelWindow / 2);

    const float * channel = this->channel(c);

    float sum = 0.0f;
    for (uint i = 0; i < kernelWindow; i++)
    {
        const int src_x = int(x + i) - kernelOffset;
        const int idx = this->index(src_x, y, z, wm);

        sum += k->valueAt(i) * channel[idx];
    }

    return sum;
}

/// Apply 1D vertical kernel at the given coordinates and return result.
float FloatImage::applyKernelY(const Kernel1 * k, int x, int y, int z, uint c, WrapMode wm) const
{
    nvDebugCheck(k != NULL);

    const uint kernelWindow = k->windowSize();
    const int kernelOffset = int(kernelWindow / 2);

    const float * channel = this->channel(c);

    float sum = 0.0f;
    for (uint i = 0; i < kernelWindow; i++)
    {
        const int src_y = int(y + i) - kernelOffset;
        const int idx = this->index(x, src_y, z, wm);

        sum += k->valueAt(i) * channel[idx];
    }

    return sum;
}

/// Apply 1D kernel in the z direction at the given coordinates and return result.
float FloatImage::applyKernelZ(const Kernel1 * k, int x, int y, int z, uint c, WrapMode wm) const
{
    nvDebugCheck(k != NULL);

    const uint kernelWindow = k->windowSize();
    const int kernelOffset = int(kernelWindow / 2);

    const float * channel = this->channel(c);

    float sum = 0.0f;
    for (uint i = 0; i < kernelWindow; i++)
    {
        const int src_z = int(z + i) - kernelOffset;
        const int idx = this->index(x, y, src_z, wm);

        sum += k->valueAt(i) * channel[idx];
    }

    return sum;
}


/// Apply 1D horizontal kernel at the given coordinates and return result.
void FloatImage::applyKernelX(const PolyphaseKernel & k, int y, int z, uint c, WrapMode wm, float * __restrict output) const
{
    const uint length = k.length();
    const float scale = float(length) / float(m_width);
    const float iscale = 1.0f / scale;

    const float width = k.width();
    const int windowSize = k.windowSize();

    const float * channel = this->channel(c);

    for (uint i = 0; i < length; i++)
    {
        const float center = (0.5f + i) * iscale;

        const int left = (int)floorf(center - width);
        const int right = (int)ceilf(center + width);
        nvDebugCheck(right - left <= windowSize);

        float sum = 0;
        for (int j = 0; j < windowSize; ++j)
        {
            const int idx = this->index(left + j, y, z, wm);

            sum += k.valueAt(i, j) * channel[idx];
        }

        output[i] = sum;
    }
}

/// Apply 1D vertical kernel at the given coordinates and return result.
void FloatImage::applyKernelY(const PolyphaseKernel & k, int x, int z, uint c, WrapMode wm, float * __restrict output) const
{
    const uint length = k.length();
    const float scale = float(length) / float(m_height);
    const float iscale = 1.0f / scale;

    const float width = k.width();
    const int windowSize = k.windowSize();

    const float * channel = this->channel(c);

    for (uint i = 0; i < length; i++)
    {
        const float center = (0.5f + i) * iscale;

        const int left = (int)floorf(center - width);
        const int right = (int)ceilf(center + width);
        nvCheck(right - left <= windowSize);

        float sum = 0;
        for (int j = 0; j < windowSize; ++j)
        {
            const int idx = this->index(x, j+left, z, wm);

            sum += k.valueAt(i, j) * channel[idx];
        }

        output[i] = sum;
    }
}

/// Apply 1D kernel in the Z direction at the given coordinates and return result.
void FloatImage::applyKernelZ(const PolyphaseKernel & k, int x, int y, uint c, WrapMode wm, float * __restrict output) const
{
    const uint length = k.length();
    const float scale = float(length) / float(m_height);
    const float iscale = 1.0f / scale;

    const float width = k.width();
    const int windowSize = k.windowSize();

    const float * channel = this->channel(c);

    for (uint i = 0; i < length; i++)
    {
        const float center = (0.5f + i) * iscale;

        const int left = (int)floorf(center - width);
        const int right = (int)ceilf(center + width);
        nvCheck(right - left <= windowSize);

        float sum = 0;
        for (int j = 0; j < windowSize; ++j)
        {
            const int idx = this->index(x, y, j+left, wm);

            sum += k.valueAt(i, j) * channel[idx];
        }

        output[i] = sum;
    }
}


/// Apply 1D horizontal kernel at the given coordinates and return result.
void FloatImage::applyKernelX(const PolyphaseKernel & k, int y, int z, uint c, uint a, WrapMode wm, float * __restrict output) const
{
    const uint length = k.length();
    const float scale = float(length) / float(m_width);
    const float iscale = 1.0f / scale;

    const float width = k.width();
    const int windowSize = k.windowSize();

    const float * channel = this->channel(c);
    const float * alpha = this->channel(a);

    for (uint i = 0; i < length; i++)
    {
        const float center = (0.5f + i) * iscale;

        const int left = (int)floorf(center - width);
        const int right = (int)ceilf(center + width);
        nvDebugCheck(right - left <= windowSize);

        float norm = 0.0f;
        float sum = 0;
        for (int j = 0; j < windowSize; ++j)
        {
            const int idx = this->index(left + j, y, z, wm);

            float w = k.valueAt(i, j) * (alpha[idx] + (1.0f / 256.0f));
            norm += w;
            sum += w * channel[idx];
        }

        output[i] = sum / norm;
    }
}

/// Apply 1D vertical kernel at the given coordinates and return result.
void FloatImage::applyKernelY(const PolyphaseKernel & k, int x, int z, uint c, uint a, WrapMode wm, float * __restrict output) const
{
    const uint length = k.length();
    const float scale = float(length) / float(m_height);
    const float iscale = 1.0f / scale;

    const float width = k.width();
    const int windowSize = k.windowSize();

    const float * channel = this->channel(c);
    const float * alpha = this->channel(a);

    for (uint i = 0; i < length; i++)
    {
        const float center = (0.5f + i) * iscale;

        const int left = (int)floorf(center - width);
        const int right = (int)ceilf(center + width);
        nvCheck(right - left <= windowSize);

        float norm = 0;
        float sum = 0;
        for (int j = 0; j < windowSize; ++j)
        {
            const int idx = this->index(x, j+left, z, wm);

            float w = k.valueAt(i, j) * (alpha[idx] + (1.0f / 256.0f));
            norm += w;
            sum += w * channel[idx];
        }

        output[i] = sum / norm;
    }
}

/// Apply 1D horizontal kernel at the given coordinates and return result.
void FloatImage::applyKernelZ(const PolyphaseKernel & k, int x, int y, uint c, uint a, WrapMode wm, float * __restrict output) const
{
    const uint length = k.length();
    const float scale = float(length) / float(m_width);
    const float iscale = 1.0f / scale;

    const float width = k.width();
    const int windowSize = k.windowSize();

    const float * channel = this->channel(c);
    const float * alpha = this->channel(a);

    for (uint i = 0; i < length; i++)
    {
        const float center = (0.5f + i) * iscale;

        const int left = (int)floorf(center - width);
        const int right = (int)ceilf(center + width);
        nvDebugCheck(right - left <= windowSize);

        float norm = 0.0f;
        float sum = 0;
        for (int j = 0; j < windowSize; ++j)
        {
            const int idx = this->index(x, y, left + j, wm);

            float w = k.valueAt(i, j) * (alpha[idx] + (1.0f / 256.0f));
            norm += w;
            sum += w * channel[idx];
        }

        output[i] = sum / norm;
    }
}


void FloatImage::flipX()
{
    const uint w = m_width;
    const uint h = m_height;
    const uint d = m_depth;
    const uint w2 = w / 2;

    for (uint c = 0; c < m_componentCount; c++) {
        for (uint z = 0; z < d; z++) {
            for (uint y = 0; y < h; y++) {
                float * line = scanline(c, y, z);
                for (uint x = 0; x < w2; x++) {
                    swap(line[x], line[w - 1 - x]);
                }
            }
        }
    }
}

void FloatImage::flipY()
{
    const uint w = m_width;
    const uint h = m_height;
    const uint d = m_depth;
    const uint h2 = h / 2;

    for (uint c = 0; c < m_componentCount; c++) {
        for (uint z = 0; z < d; z++) {
            for (uint y = 0; y < h2; y++) {
                float * src = scanline(c, y, z);
                float * dst = scanline(c, h - 1 - y, z);
                for (uint x = 0; x < w; x++) {
                    swap(src[x], dst[x]);
                }
            }
        }
    }
}

void FloatImage::flipZ()
{
    const uint w = m_width;
    const uint h = m_height;
    const uint d = m_depth;
    const uint d2 = d / 2;

    for (uint c = 0; c < m_componentCount; c++) {
        for (uint z = 0; z < d2; z++) {
            float * src = plane(c, z);
            float * dst = plane(c, d - 1 - z);
            for (uint i = 0; i < w*h; i++) {
                swap(src[i], dst[i]);
            }
        }
    }
}



float FloatImage::alphaTestCoverage(float alphaRef, int alphaChannel, float alphaScale/*=1*/) const
{
    const uint w = m_width;
    const uint h = m_height;

    float coverage = 0.0f;

#if 0
    const float * alpha = channel(alphaChannel);

    const uint count = m_pixelCount;
    for (uint i = 0; i < count; i++) {
        if (alpha[i] > alphaRef) coverage += 1.0f; // @@ gt or lt?
    }
    
    return coverage / float(w * h);
#else
    const uint n = 8;

    // If we want subsampling:
    for (uint y = 0; y < h-1; y++) {
        for (uint x = 0; x < w-1; x++) {

            float alpha00 = nv::saturate(pixel(alphaChannel, x+0, y+0, 0) * alphaScale);
            float alpha10 = nv::saturate(pixel(alphaChannel, x+1, y+0, 0) * alphaScale);
            float alpha01 = nv::saturate(pixel(alphaChannel, x+0, y+1, 0) * alphaScale);
            float alpha11 = nv::saturate(pixel(alphaChannel, x+1, y+1, 0) * alphaScale);

            for (float fy = 0.5f/n; fy < 1.0f; fy++) {
                for (float fx = 0.5f/n; fx < 1.0f; fx++) {
                    float alpha = alpha00 * (1 - fx) * (1 - fy) + alpha10 * fx * (1 - fy) + alpha01 * (1 - fx) * fy + alpha11 * fx * fy;
                    if (alpha > alphaRef) coverage += 1.0f;
                }
            }
        }
    }

    return coverage / float(w * h * n * n);
#endif
}

void FloatImage::scaleAlphaToCoverage(float desiredCoverage, float alphaRef, int alphaChannel)
{
#if 0
    float minAlphaRef = 0.0f;
    float maxAlphaRef = 1.0f;
    float midAlphaRef = 0.5f;

    // Determine desired scale using a binary search. Hardcoded to 8 steps max.
    for (int i = 0; i < 10; i++) {
        float currentCoverage = alphaTestCoverage(midAlphaRef, alphaChannel);

        if (currentCoverage > desiredCoverage) {
            minAlphaRef = midAlphaRef;
        }
        else if (currentCoverage < desiredCoverage) {
            maxAlphaRef = midAlphaRef;
        }
        else {
            break;
        }

        midAlphaRef = (minAlphaRef + maxAlphaRef) * 0.5f;
    }

    float alphaScale = alphaRef / midAlphaRef;

    // Scale alpha channel.
    scaleBias(alphaChannel, 1, alphaScale, 0.0f);
    clamp(alphaChannel, 1, 0.0f, 1.0f); 
#else
    float minAlphaScale = 0.0f;
    float maxAlphaScale = 4.0f;
    float alphaScale = 1.0f;

    // Determine desired scale using a binary search. Hardcoded to 8 steps max.
    for (int i = 0; i < 10; i++) {
        float currentCoverage = alphaTestCoverage(alphaRef, alphaChannel, alphaScale);

        if (currentCoverage < desiredCoverage) {
            minAlphaScale = alphaScale;
        }
        else if (currentCoverage > desiredCoverage) {
            maxAlphaScale = alphaScale;
        }
        else {
            break;
        }

        alphaScale = (minAlphaScale + maxAlphaScale) * 0.5f;
    }

    // Scale alpha channel.
    scaleBias(alphaChannel, 1, alphaScale, 0.0f);
    clamp(alphaChannel, 1, 0.0f, 1.0f); 
#endif
#if _DEBUG
    alphaTestCoverage(alphaRef, alphaChannel);
#endif
}

FloatImage* FloatImage::clone() const
{
    FloatImage* copy = new FloatImage();

    copy->allocate(m_componentCount, m_width, m_height, m_depth);
    memcpy(copy->m_mem, m_mem, m_floatCount * sizeof(float));

    return copy;
}

