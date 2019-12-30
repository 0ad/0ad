// This code is in the public domain -- castanyo@yahoo.es

#include "ColorBlock.h"
#include "Image.h"
#include "FloatImage.h"

#include "nvmath/Box.h"
#include "nvmath/Vector.inl"
#include "nvmath/ftoi.h"

#include "nvcore/Utils.h" // swap

#include <string.h> // memcpy

using namespace nv;

namespace {

    // Get approximate luminance.
    inline static uint colorLuminance(Color32 c)
    {
        return c.r + c.g + c.b;
    }

    // Get the euclidean distance between the given colors.
    inline static uint colorDistance(Color32 c0, Color32 c1)
    {
        return (c0.r - c1.r) * (c0.r - c1.r) + (c0.g - c1.g) * (c0.g - c1.g) + (c0.b - c1.b) * (c0.b - c1.b);
    }

} // namespace`


/// Default constructor.
ColorBlock::ColorBlock()
{
}

/// Init the color block from an array of colors.
ColorBlock::ColorBlock(const uint * linearImage)
{
    for(uint i = 0; i < 16; i++) {
        color(i) = Color32(linearImage[i]);
    }
}

/// Init the color block with the contents of the given block.
ColorBlock::ColorBlock(const ColorBlock & block)
{
    for(uint i = 0; i < 16; i++) {
        color(i) = block.color(i);
    }
}


/// Initialize this color block.
ColorBlock::ColorBlock(const Image * img, uint x, uint y)
{
    init(img, x, y);
}

void ColorBlock::init(const Image * img, uint x, uint y)
{
    init(img->width(), img->height(), (const uint *)img->pixels(), x, y);
}

void ColorBlock::init(uint w, uint h, const uint * data, uint x, uint y)
{
    nvDebugCheck(data != NULL);

    const uint bw = min(w - x, 4U);
    const uint bh = min(h - y, 4U);
    nvDebugCheck(bw != 0 && bh != 0);

    // Blocks that are smaller than 4x4 are handled by repeating the pixels.
    // @@ Thats only correct when block size is 1, 2 or 4, but not with 3. :(
    // @@ Ideally we should zero the weights of the pixels out of range.

    for (uint i = 0; i < 4; i++)
    {
        const int by = i % bh;

        for (uint e = 0; e < 4; e++)
        {
            const int bx = e % bw;
            const uint idx = (y + by) * w + x + bx;

            color(e, i).u = data[idx];
        }
    }
}

void ColorBlock::init(uint w, uint h, const float * data, uint x, uint y)
{
    nvDebugCheck(data != NULL);

    const uint bw = min(w - x, 4U);
    const uint bh = min(h - y, 4U);
    nvDebugCheck(bw != 0 && bh != 0);

    // Blocks that are smaller than 4x4 are handled by repeating the pixels.
    // @@ Thats only correct when block size is 1, 2 or 4, but not with 3. :(
    // @@ Ideally we should zero the weights of the pixels out of range.

    uint srcPlane = w * h;

    for (uint i = 0; i < 4; i++)
    {
        const uint by = i % bh;

        for (uint e = 0; e < 4; e++)
        {
            const uint bx = e % bw;
            const uint idx = ((y + by) * w + x + bx);

            Color32 & c = color(e, i);
            c.r = uint8(255 * clamp(data[idx + 0 * srcPlane], 0.0f, 1.0f)); // @@ Is this the right way to quantize floats to bytes?
            c.g = uint8(255 * clamp(data[idx + 1 * srcPlane], 0.0f, 1.0f));
            c.b = uint8(255 * clamp(data[idx + 2 * srcPlane], 0.0f, 1.0f));
            c.a = uint8(255 * clamp(data[idx + 3 * srcPlane], 0.0f, 1.0f));
        }
    }
}

static inline uint8 component(Color32 c, uint i)
{
    if (i == 0) return c.r;
    if (i == 1) return c.g;
    if (i == 2) return c.b;
    if (i == 3) return c.a;
    if (i == 4) return 0xFF;
    return 0;
}

void ColorBlock::swizzle(uint x, uint y, uint z, uint w)
{
    for (int i = 0; i < 16; i++)
    {
        Color32 c = m_color[i];
        m_color[i].r = component(c, x);
        m_color[i].g = component(c, y);
        m_color[i].b = component(c, z);
        m_color[i].a = component(c, w);
    }
}


/// Returns true if the block has a single color.
bool ColorBlock::isSingleColor(Color32 mask/*= Color32(0xFF, 0xFF, 0xFF, 0x00)*/) const
{
    uint u = m_color[0].u & mask.u;

    for (int i = 1; i < 16; i++)
    {
        if (u != (m_color[i].u & mask.u))
        {
            return false;
        }
    }

    return true;
}

/*
/// Returns true if the block has a single color, ignoring transparent pixels.
bool ColorBlock::isSingleColorNoAlpha() const
{
    Color32 c;
    int i;
    for(i = 0; i < 16; i++)
    {
        if (m_color[i].a != 0) c = m_color[i];
    }

    Color32 mask(0xFF, 0xFF, 0xFF, 0x00);
    uint u = c.u & mask.u;

    for(; i < 16; i++)
    {
        if (u != (m_color[i].u & mask.u))
        {
            return false;
        }
    }

    return true;
}
*/

/// Count number of unique colors in this color block.
/*uint ColorBlock::countUniqueColors() const
{
    uint count = 0;

    // @@ This does not have to be o(n^2)
    for(int i = 0; i < 16; i++)
    {
        bool unique = true;
        for(int j = 0; j < i; j++) {
            if( m_color[i] != m_color[j] ) {
                unique = false;
            }
        }

        if( unique ) {
            count++;
        }
    }

    return count;
}*/

/*/// Get average color of the block.
Color32 ColorBlock::averageColor() const
{
    uint r, g, b, a;
    r = g = b = a = 0;

    for(uint i = 0; i < 16; i++) {
        r += m_color[i].r;
        g += m_color[i].g;
        b += m_color[i].b;
        a += m_color[i].a;
    }

    return Color32(uint8(r / 16), uint8(g / 16), uint8(b / 16), uint8(a / 16));
}*/

/// Return true if the block is not fully opaque.
bool ColorBlock::hasAlpha() const
{
    for (uint i = 0; i < 16; i++)
    {
        if (m_color[i].a != 255) return true;
    }
    return false;
}

#if 0

/// Get diameter color range.
void ColorBlock::diameterRange(Color32 * start, Color32 * end) const
{
    nvDebugCheck(start != NULL);
    nvDebugCheck(end != NULL);

    Color32 c0, c1;
    uint best_dist = 0;

    for(int i = 0; i < 16; i++) {
        for (int j = i+1; j < 16; j++) {
            uint dist = colorDistance(m_color[i], m_color[j]);
            if( dist > best_dist ) {
                best_dist = dist;
                c0 = m_color[i];
                c1 = m_color[j];
            }
        }
    }

    *start = c0;
    *end = c1;
}

/// Get luminance color range.
void ColorBlock::luminanceRange(Color32 * start, Color32 * end) const
{
    nvDebugCheck(start != NULL);
    nvDebugCheck(end != NULL);

    Color32 minColor, maxColor;
    uint minLuminance, maxLuminance;

    maxLuminance = minLuminance = colorLuminance(m_color[0]);

    for(uint i = 1; i < 16; i++)
    {
        uint luminance = colorLuminance(m_color[i]);

        if (luminance > maxLuminance) {
            maxLuminance = luminance;
            maxColor = m_color[i];
        }
        else if (luminance < minLuminance) {
            minLuminance = luminance;
            minColor = m_color[i];
        }
    }

    *start = minColor;
    *end = maxColor;
}

/// Get color range based on the bounding box. 
void ColorBlock::boundsRange(Color32 * start, Color32 * end) const
{
    nvDebugCheck(start != NULL);
    nvDebugCheck(end != NULL);

    Color32 minColor(255, 255, 255);
    Color32 maxColor(0, 0, 0);

    for(uint i = 0; i < 16; i++)
    {
        if (m_color[i].r < minColor.r) { minColor.r = m_color[i].r; }
        if (m_color[i].g < minColor.g) { minColor.g = m_color[i].g; }
        if (m_color[i].b < minColor.b) { minColor.b = m_color[i].b; }
        if (m_color[i].r > maxColor.r) { maxColor.r = m_color[i].r; }
        if (m_color[i].g > maxColor.g) { maxColor.g = m_color[i].g; }
        if (m_color[i].b > maxColor.b) { maxColor.b = m_color[i].b; }
    }

    // Offset range by 1/16 of the extents
    Color32 inset;
    inset.r = (maxColor.r - minColor.r) >> 4;
    inset.g = (maxColor.g - minColor.g) >> 4;
    inset.b = (maxColor.b - minColor.b) >> 4;

    minColor.r = (minColor.r + inset.r <= 255) ? minColor.r + inset.r : 255;
    minColor.g = (minColor.g + inset.g <= 255) ? minColor.g + inset.g : 255;
    minColor.b = (minColor.b + inset.b <= 255) ? minColor.b + inset.b : 255;

    maxColor.r = (maxColor.r >= inset.r) ? maxColor.r - inset.r : 0;
    maxColor.g = (maxColor.g >= inset.g) ? maxColor.g - inset.g : 0;
    maxColor.b = (maxColor.b >= inset.b) ? maxColor.b - inset.b : 0;

    *start = minColor;
    *end = maxColor;
}

/// Get color range based on the bounding box. 
void ColorBlock::boundsRangeAlpha(Color32 * start, Color32 * end) const
{
    nvDebugCheck(start != NULL);
    nvDebugCheck(end != NULL);

    Color32 minColor(255, 255, 255, 255);
    Color32 maxColor(0, 0, 0, 0);

    for(uint i = 0; i < 16; i++)
    {
        if (m_color[i].r < minColor.r) { minColor.r = m_color[i].r; }
        if (m_color[i].g < minColor.g) { minColor.g = m_color[i].g; }
        if (m_color[i].b < minColor.b) { minColor.b = m_color[i].b; }
        if (m_color[i].a < minColor.a) { minColor.a = m_color[i].a; }
        if (m_color[i].r > maxColor.r) { maxColor.r = m_color[i].r; }
        if (m_color[i].g > maxColor.g) { maxColor.g = m_color[i].g; }
        if (m_color[i].b > maxColor.b) { maxColor.b = m_color[i].b; }
        if (m_color[i].a > maxColor.a) { maxColor.a = m_color[i].a; }
    }

    // Offset range by 1/16 of the extents
    Color32 inset;
    inset.r = (maxColor.r - minColor.r) >> 4;
    inset.g = (maxColor.g - minColor.g) >> 4;
    inset.b = (maxColor.b - minColor.b) >> 4;
    inset.a = (maxColor.a - minColor.a) >> 4;

    minColor.r = (minColor.r + inset.r <= 255) ? minColor.r + inset.r : 255;
    minColor.g = (minColor.g + inset.g <= 255) ? minColor.g + inset.g : 255;
    minColor.b = (minColor.b + inset.b <= 255) ? minColor.b + inset.b : 255;
    minColor.a = (minColor.a + inset.a <= 255) ? minColor.a + inset.a : 255;

    maxColor.r = (maxColor.r >= inset.r) ? maxColor.r - inset.r : 0;
    maxColor.g = (maxColor.g >= inset.g) ? maxColor.g - inset.g : 0;
    maxColor.b = (maxColor.b >= inset.b) ? maxColor.b - inset.b : 0;
    maxColor.a = (maxColor.a >= inset.a) ? maxColor.a - inset.a : 0;

    *start = minColor;
    *end = maxColor;
}
#endif

/*/// Sort colors by abosolute value in their 16 bit representation.
void ColorBlock::sortColorsByAbsoluteValue()
{
    // Dummy selection sort.
    for( uint a = 0; a < 16; a++ ) {
        uint max = a;
        Color16 cmax(m_color[a]);

        for( uint b = a+1; b < 16; b++ ) {
            Color16 cb(m_color[b]);

            if( cb.u > cmax.u ) {
                max = b;
                cmax = cb;
            }
        }
        swap( m_color[a], m_color[max] );
    }
}*/


/*/// Find extreme colors in the given axis.
void ColorBlock::computeRange(Vector3::Arg axis, Color32 * start, Color32 * end) const
{
    nvDebugCheck(start != NULL);
    nvDebugCheck(end != NULL);

    int mini, maxi;
    mini = maxi = 0;

    float min, max;	
    min = max = dot(Vector3(m_color[0].r, m_color[0].g, m_color[0].b), axis);

    for(uint i = 1; i < 16; i++)
    {
        const Vector3 vec(m_color[i].r, m_color[i].g, m_color[i].b);

        float val = dot(vec, axis);
        if( val < min ) {
            mini = i;
            min = val;
        }
        else if( val > max ) {
            maxi = i;
            max = val;
        }
    }

    *start = m_color[mini];
    *end = m_color[maxi];
}*/


/*/// Sort colors in the given axis.
void ColorBlock::sortColors(const Vector3 & axis)
{
    float luma_array[16];

    for(uint i = 0; i < 16; i++) {
        const Vector3 vec(m_color[i].r, m_color[i].g, m_color[i].b);
        luma_array[i] = dot(vec, axis);
    }

    // Dummy selection sort.
    for( uint a = 0; a < 16; a++ ) {
        uint min = a;
        for( uint b = a+1; b < 16; b++ ) {
            if( luma_array[b] < luma_array[min] ) {
                min = b;
            }
        }
        swap( luma_array[a], luma_array[min] );
        swap( m_color[a], m_color[min] );
    }
}*/


/*/// Get the volume of the color block.
float ColorBlock::volume() const
{
    Box bounds;
    bounds.clearBounds();

    for(int i = 0; i < 16; i++) {
        const Vector3 point(m_color[i].r, m_color[i].g, m_color[i].b);
        bounds.addPointToBounds(point);
    }

    return bounds.volume();
}*/

#if 0
void ColorSet::allocate(uint w, uint h)
{
    nvDebugCheck(w <= 4 && h <= 4);

    this->colorCount = w * h;
    this->indexCount = 16;
    this->w = 4;
    this->h = 4;

    //colors = new Vector4[colorCount];
    //weights = new float[colorCount];
    //indices = new int[indexCount];
}

// Allocate 4x4 block and fill with 
void ColorSet::setColors(const float * data, uint img_w, uint img_h, uint img_x, uint img_y)
{
    nvDebugCheck(img_x < img_w && img_y < img_h);

    const uint block_w = min(4U, img_w - img_x);
    const uint block_h = min(4U, img_h - img_y);
    nvDebugCheck(block_w != 0 && block_h != 0);

    allocate(block_w, block_h);

    const float * r = data + img_w * img_h * 0;
    const float * g = data + img_w * img_h * 1;
    const float * b = data + img_w * img_h * 2;
    const float * a = data + img_w * img_h * 3;

    // Set colors.
    for (uint y = 0, i = 0; y < block_h; y++)
    {
        for (uint x = 0; x < block_w; x++, i++)
        {
            uint idx = x + img_x + (y + img_y) * img_w;
            colors[i].x = r[idx];
            colors[i].y = g[idx];
            colors[i].z = b[idx];
            colors[i].w = a[idx];
        }
    }

    // Set default indices.
    for (uint y = 0, i = 0; y < 4; y++)
    {
        for (uint x = 0; x < 4; x++)
        {
            if (x < block_w && y < block_h) {
                indices[y*4+x] = i++;
            }
            else {
                indices[y*4+x] = -1;
            }
        }
    }
}

void ColorSet::setColors(const Vector3 colors[16], const float weights[16])
{

}

void ColorSet::setColors(const Vector4 colors[16], const float weights[16])
{

}



void ColorSet::setAlphaWeights()
{
    for (uint i = 0; i < colorCount; i++)
    {
        //weights[i] = max(colors[i].w, 0.001f); // Avoid division by zero.
        weights[i] = max(colors[i].w, 0.0f);
    }
}

void ColorSet::setUniformWeights()
{
    for (uint i = 0; i < colorCount; i++)
    {
        weights[i] = 1.0f;
    }
}


// @@ Handle complex blocks (not 4x4).
void ColorSet::createMinimalSet(bool ignoreTransparent)
{
    nvDebugCheck(indexCount == 16);
    nvDebugCheck(colorCount <= 16);

    Vector4 C[16];
    float W[16];
    memcpy(C, colors, sizeof(Vector4)*colorCount);
    memcpy(W, weights, sizeof(float)*colorCount);

    uint n = 0;
    for (uint i = 0; i < indexCount; i++)
    {
        if (indices[i] < 0) {
            continue;
        }

        Vector4 ci = C[indices[i]];
        float wi = W[indices[i]];

        if (ignoreTransparent && wi == 0) {
            indices[i] = -1;
            continue;
        }

        // Find matching color.
        uint j;
        for (j = 0; j < n; j++) {
            bool colorMatch = equal(colors[j].x, ci.x) && equal(colors[j].y, ci.y) && equal(colors[j].z, ci.z);
            //bool alphaMatch = equal(colors[j].w, ci.w);

            if (colorMatch) {
                weights[j] += wi;
                indices[i] = j;
                break;
            }
        }

        // No match found. Add new color.
        if (j == n) {
            colors[n] = ci;
            weights[n] = wi;
            indices[i] = n;
            n++;
        }
    }
    //nvDebugCheck(n != 0); // Fully transparent blocks are OK.

    for (uint i = n; i < colorCount; i++) {
        colors[i] = Vector4(0);
        weights[i] = 0;
    }

    colorCount = n;

    // Avoid empty blocks.
    if (colorCount == 0) {
        colorCount = 1;
        indices[0] = 0;
        //colors[0] = Vector4(0);
        weights[0] = 1;
    }
}


// Fill blocks that are smaller than (4,4) by wrapping indices.
void ColorSet::wrapIndices()
{
    for (uint y = h; y < 4; y++)
    {
        uint base = (y % h) * w;
        for (uint x = w; x < 4; x++)
        {
            indices[y*4+3] = indices[base + (x % w)];
        }
    }
}

bool ColorSet::isSingleColor(bool ignoreAlpha) const
{
    Vector4 v = colors[0];
    if (ignoreAlpha) v.w = 1.0f;

    for (uint i = 1; i < colorCount; i++)
    {
        Vector4 c = colors[i];
        if (ignoreAlpha) c.w = 1.0f;

        if (v != c) {
            return false;
        }
    }

    return true;
}


// 0=r, 1=g, 2=b, 3=a, 4=0xFF, 5=0
static inline float component(Vector4::Arg c, uint i)
{
    if (i == 0) return c.x;
    if (i == 1) return c.y;
    if (i == 2) return c.z;
    if (i == 3) return c.w;
    if (i == 4) return 0xFF;
    return 0;
}

void ColorSet::swizzle(uint x, uint y, uint z, uint w)
{
    for (uint i = 0; i < colorCount; i++)
    {
        Vector4 c = colors[i];
        colors[i].x = component(c, x);
        colors[i].y = component(c, y);
        colors[i].z = component(c, z);
        colors[i].w = component(c, w);
    }
}

bool ColorSet::hasAlpha() const
{
    for (uint i = 0; i < colorCount; i++)
    {
        if (colors[i].w != 0.0f) return true;
    }
    return false;
}
#endif // 0


void AlphaBlock4x4::init(uint8 a)
{
    for (int i = 0; i < 16; i++) {
        alpha[i] = a;
        weights[i] = 1.0f;
    }
}

void AlphaBlock4x4::init(const ColorBlock & src, uint channel)
{
    nvCheck(channel >= 0 && channel < 4);

    // Colors are in BGRA format.
    if (channel == 0) channel = 2;
    else if (channel == 2) channel = 0;

    for (int i = 0; i < 16; i++) {
        alpha[i] = src.color(i).component[channel];
        weights[i] = 1.0f;
    }
}




/*void AlphaBlock4x4::init(const ColorSet & src, uint channel)
{
    nvCheck(channel >= 0 && channel < 4);

    for (int i = 0; i < 16; i++) {
        float f = src.color(i).component[channel];
        alpha[i] = unitFloatToFixed8(f);
        weights[i] = 1.0f;
    }
}

void AlphaBlock4x4::initMaxRGB(const ColorSet & src, float threshold)
{
    for (int i = 0; i < 16; i++) {
        float x = src.color(i).x;
        float y = src.color(i).y;
        float z = src.color(i).z;
        alpha[i] = unitFloatToFixed8(max(max(x, y), max(z, threshold)));
        weights[i] = 1.0f;
    }
}*/

/*void AlphaBlock4x4::initWeights(const ColorSet & src)
{
    for (int i = 0; i < 16; i++) {
        weights[i] = src.weight(i);
    }
}*/

