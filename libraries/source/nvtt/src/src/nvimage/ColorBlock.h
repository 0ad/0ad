// This code is in the public domain -- castanyo@yahoo.es

#pragma once
#ifndef NV_IMAGE_COLORBLOCK_H
#define NV_IMAGE_COLORBLOCK_H

#include "nvimage.h"

#include "nvmath/Color.h"
#include "nvmath/Vector.h"

namespace nv
{
    class Image;
    class FloatImage;


    /// Uncompressed 4x4 color block.
    struct NVIMAGE_CLASS ColorBlock
    {
        ColorBlock();
        ColorBlock(const uint * linearImage);
        ColorBlock(const ColorBlock & block);
        ColorBlock(const Image * img, uint x, uint y);

        void init(const Image * img, uint x, uint y);
        void init(uint w, uint h, const uint * data, uint x, uint y);
        void init(uint w, uint h, const float * data, uint x, uint y);

        void swizzle(uint x, uint y, uint z, uint w); // 0=r, 1=g, 2=b, 3=a, 4=0xFF, 5=0

        bool isSingleColor(Color32 mask = Color32(0xFF, 0xFF, 0xFF, 0x00)) const;
        bool hasAlpha() const;


        // Accessors
        const Color32 * colors() const;

        Color32 color(uint i) const;
        Color32 & color(uint i);

        Color32 color(uint x, uint y) const;
        Color32 & color(uint x, uint y);

    private:

        Color32 m_color[4*4];

    };


    /// Get pointer to block colors.
    inline const Color32 * ColorBlock::colors() const
    {
        return m_color;
    }

    /// Get block color.
    inline Color32 ColorBlock::color(uint i) const
    {
        nvDebugCheck(i < 16);
        return m_color[i];
    }

    /// Get block color.
    inline Color32 & ColorBlock::color(uint i)
    {
        nvDebugCheck(i < 16);
        return m_color[i];
    }

    /// Get block color.
    inline Color32 ColorBlock::color(uint x, uint y) const
    {
        nvDebugCheck(x < 4 && y < 4);
        return m_color[y * 4 + x];
    }

    /// Get block color.
    inline Color32 & ColorBlock::color(uint x, uint y)
    {
        nvDebugCheck(x < 4 && y < 4);
        return m_color[y * 4 + x];
    }

    /*
    struct ColorSet
    {
        ColorSet() : colorCount(0), indexCount(0), w(0), h(0) {}
        //~ColorSet() {}

        void allocate(uint w, uint h);

        void setColors(const float * data, uint img_w, uint img_h, uint img_x, uint img_y);
        void setColors(const Vector3 colors[16], const float weights[16]);
        void setColors(const Vector4 colors[16], const float weights[16]);

        void setAlphaWeights();
        void setUniformWeights();

        void createMinimalSet(bool ignoreTransparent);
        void wrapIndices();

        void swizzle(uint x, uint y, uint z, uint w); // 0=r, 1=g, 2=b, 3=a, 4=0xFF, 5=0

        bool isSingleColor(bool ignoreAlpha) const;
        bool hasAlpha() const;

        // These methods require indices to be set:
        Vector4 color(uint x, uint y) const { nvDebugCheck(x < w && y < h); return colors[indices[y * 4 + x]]; }
        Vector4 & color(uint x, uint y) { nvDebugCheck(x < w && y < h); return colors[indices[y * 4 + x]]; }

        Vector4 color(uint i) const { nvDebugCheck(i < indexCount); return colors[indices[i]]; }
        Vector4 & color(uint i) { nvDebugCheck(i < indexCount); return colors[indices[i]]; }

        float weight(uint i) const { nvDebugCheck(i < indexCount); return weights[indices[i]]; }

        bool isValidIndex(uint i) const { return i < indexCount && indices[i] >= 0; }

        uint colorCount;
        uint indexCount;    // Fixed to 16
        uint w, h;          // Fixed to 4x4

        // Allocate color set dynamically and add support for sets larger than 4x4.
        Vector4 colors[16];
        float weights[16];  // @@ Add mask to indicate what color components are weighted?
        int indices[16];
    };
    */


    /// Uncompressed 4x4 alpha block.
    struct NVIMAGE_CLASS AlphaBlock4x4
    {
        void init(uint8 value);
        void init(const ColorBlock & src, uint channel);
        //void init(const ColorSet & src, uint channel);

        //void initMaxRGB(const ColorSet & src, float threshold);
        //void initWeights(const ColorSet & src);

        uint8 alpha[4*4];
        float weights[16];
    };


    struct FloatAlphaBlock4x4
    {
        float alphas[4 * 4];
        float weights[4 * 4];
    };

    struct FloatColorBlock4x4
    {
        Vector4 colors[4 * 4];
        float weights[4 * 4];
    };



} // nv namespace

#endif // NV_IMAGE_COLORBLOCK_H
