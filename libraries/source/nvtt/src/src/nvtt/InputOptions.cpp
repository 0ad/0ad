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

#include "InputOptions.h"

#include "nvmath/Vector.inl"

#include "nvcore/Utils.h" // nextPowerOfTwo
#include "nvcore/Memory.h"

#include <string.h> // memcpy, memset



using namespace nv;
using namespace nvtt;

namespace
{

    static uint countMipmaps(int w, int h, int d)
    {
        uint mipmap = 0;

        while (w != 1 || h != 1 || d != 1) {
            w = max(1, w / 2);
            h = max(1, h / 2);
            d = max(1, d / 2);
            mipmap++;
        }

        return mipmap + 1;
    }

    // 1 -> 1, 2 -> 2, 3 -> 2, 4 -> 4, 5 -> 4, ...
    static uint previousPowerOfTwo(const uint v)
    {
        return nextPowerOfTwo(v + 1) / 2;
    }

    static uint nearestPowerOfTwo(const uint v)
    {
        const uint np2 = nextPowerOfTwo(v);
        const uint pp2 = previousPowerOfTwo(v);

        if (np2 - v <= v - pp2)
        {
            return np2;
        }
        else
        {
            return pp2;
        }
    }

} // namespace


/// Constructor.
InputOptions::InputOptions() : m(*new InputOptions::Private())
{ 
    reset();
}

// Delete images.
InputOptions::~InputOptions()
{
    resetTextureLayout();

    delete &m;
}


// Reset input options.
void InputOptions::reset()
{
    m.wrapMode = WrapMode_Mirror;
    m.textureType = TextureType_2D;
    m.inputFormat = InputFormat_BGRA_8UB;

    m.alphaMode = AlphaMode_None;

    m.inputGamma = 2.2f;
    m.outputGamma = 2.2f;

    m.generateMipmaps = true;
    m.maxLevel = -1;
    m.mipmapFilter = MipmapFilter_Box;

    m.kaiserWidth = 3;
    m.kaiserAlpha = 4.0f;
    m.kaiserStretch = 1.0f;

    m.isNormalMap = false;
    m.normalizeMipmaps = true;
    m.convertToNormalMap = false;
    m.heightFactors.set(0.0f, 0.0f, 0.0f, 1.0f);
    m.bumpFrequencyScale = Vector4(1.0f, 0.5f, 0.25f, 0.125f) / (1.0f + 0.5f + 0.25f + 0.125f);

    m.maxExtent = 0;
    m.roundMode = RoundMode_None;
}


// Setup the input image.
void InputOptions::setTextureLayout(TextureType type, int width, int height, int depth /*= 1*/, int arraySize /*= 1*/)
{
    // Validate arguments.
    nvCheck(width >= 0);
    nvCheck(height >= 0);
    nvCheck(depth >= 0);
    nvCheck(arraySize >= 0);

    // Correct arguments.
    if (width == 0) width = 1;
    if (height == 0) height = 1;
    if (depth == 0) depth = 1;
    if (arraySize == 0) arraySize = 1;

    // Delete previous images.
    resetTextureLayout();

    m.textureType = type;
    m.width = width;
    m.height = height;
    m.depth = depth;

    // Allocate images.
    if (type == TextureType_Cube) {
        nvCheck(arraySize == 1);
        m.faceCount = 6;
    }
    else if (type == TextureType_Array) {
        m.faceCount = arraySize;
    } else {
        nvCheck(arraySize == 1);
        m.faceCount = 1;
    }
    m.mipmapCount = countMipmaps(width, height, depth);
    m.imageCount = m.mipmapCount * m.faceCount;
    m.images = new void *[m.imageCount];

    memset(m.images, 0, sizeof(void *) * m.imageCount);
}


void InputOptions::resetTextureLayout()
{
    if (m.images != NULL)
    {
        // Delete images.
        for (uint i = 0; i < m.imageCount; i++) {
            free(m.images[i]);
        }

        // Delete image array.
        delete [] m.images;
        m.images = NULL;

        m.faceCount = 0;
        m.mipmapCount = 0;
        m.imageCount = 0;
    }
}


// Copies the data to our internal structures.
bool InputOptions::setMipmapData(const void * data, int width, int height, int depth /*= 1*/, int face /*= 0*/, int mipLevel /*= 0*/)
{
    if (uint(face) >= m.faceCount) {
        return false;
    }
    if (uint(mipLevel) >= m.mipmapCount) {
        return false;
    }

    const uint idx = mipLevel * m.faceCount + face;
    if (idx >= m.imageCount) {
        return false;
    }

    // Compute expected width, height and depth for this mipLevel. Return false if it doesn't match.
    int w = m.width;
    int h = m.height;
    int d = m.depth;
    for (int i = 0; i < mipLevel; i++) {
        w = max(1, w/2);
        h = max(1, h/2);
        d = max(1, d/2);
    }
    if (w != width || h != height || d != depth) {
        return false;
    }

    int imageSize = width * height * depth;
    if (m.inputFormat == InputFormat_BGRA_8UB)
    {
        imageSize *= 4 * sizeof(uint8);
    }
    else if (m.inputFormat == InputFormat_RGBA_16F)
    {
        imageSize *= 4 * sizeof(uint16);
    }
    else if (m.inputFormat == InputFormat_RGBA_32F)
    {
        imageSize *= 4 * sizeof(float);
    }
    else if (m.inputFormat == InputFormat_R_32F)
    {
        imageSize *= 1 * sizeof(float);
    }
    else
    {
        return false;
    }

    m.images[idx] = realloc(m.images[idx], imageSize);
    if (m.images[idx] == NULL) {
        // Out of memory.
        return false;
    }

    memcpy(m.images[idx], data, imageSize);

    return true;
}


/// Describe the format of the input.
void InputOptions::setFormat(InputFormat format)
{
    m.inputFormat = format;
}


/// Set the way the input alpha channel is interpreted.
void InputOptions::setAlphaMode(AlphaMode alphaMode)
{
    m.alphaMode = alphaMode;
}


/// Set gamma settings.
void InputOptions::setGamma(float inputGamma, float outputGamma)
{
    m.inputGamma = inputGamma;
    m.outputGamma = outputGamma;
}


/// Set texture wrappign mode.
void InputOptions::setWrapMode(WrapMode mode)
{
    m.wrapMode = mode;
}


/// Set mipmap filter.
void InputOptions::setMipmapFilter(MipmapFilter filter)
{
    m.mipmapFilter = filter;
}

/// Set mipmap generation.
void InputOptions::setMipmapGeneration(bool enabled, int maxLevel/*= -1*/)
{
    m.generateMipmaps = enabled;
    m.maxLevel = maxLevel;
}

/// Set Kaiser filter parameters.
void InputOptions::setKaiserParameters(float width, float alpha, float stretch)
{
    m.kaiserWidth = width;
    m.kaiserAlpha = alpha;
    m.kaiserStretch = stretch;
}

/// Indicate whether input is a normal map or not.
void InputOptions::setNormalMap(bool b)
{
    m.isNormalMap = b;
}

/// Enable normal map conversion.
void InputOptions::setConvertToNormalMap(bool convert)
{
    m.convertToNormalMap = convert;
}

/// Set height evaluation factors.
void InputOptions::setHeightEvaluation(float redScale, float greenScale, float blueScale, float alphaScale)
{
    // Do not normalize height factors.
//  float total = redScale + greenScale + blueScale + alphaScale;
    m.heightFactors = Vector4(redScale, greenScale, blueScale, alphaScale);
}

/// Set normal map conversion filter.
void InputOptions::setNormalFilter(float small, float medium, float big, float large)
{
    float total = small + medium + big + large;
    m.bumpFrequencyScale = Vector4(small, medium, big, large) / total;
}

/// Enable mipmap normalization.
void InputOptions::setNormalizeMipmaps(bool normalize)
{
    m.normalizeMipmaps = normalize;
}

void InputOptions::setMaxExtents(int e)
{
    nvDebugCheck(e > 0);
    m.maxExtent = e;
}

void InputOptions::setRoundMode(RoundMode mode)
{
    m.roundMode = mode;
}
