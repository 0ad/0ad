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

#ifndef NVTT_INPUTOPTIONS_H
#define NVTT_INPUTOPTIONS_H

#include "nvtt.h"

#include "nvmath/Vector.h"


namespace nvtt
{

    struct InputOptions::Private
    {
        Private() : images(NULL) {}

        WrapMode wrapMode;
        TextureType textureType;
        InputFormat inputFormat;
        AlphaMode alphaMode;

        uint width;
        uint height;
        uint depth;
        uint faceCount;
        uint mipmapCount;
        uint imageCount;

        void ** images;

        // Gamma conversion.
        float inputGamma;
        float outputGamma;

        // Mipmap generation options.
        bool generateMipmaps;
        int maxLevel;
        MipmapFilter mipmapFilter;

        // Kaiser filter parameters.
        float kaiserWidth;
        float kaiserAlpha;
        float kaiserStretch;

        // Normal map options.
        bool isNormalMap;
        bool normalizeMipmaps;
        bool convertToNormalMap;
        nv::Vector4 heightFactors;
        nv::Vector4 bumpFrequencyScale;

        // Adjust extents.
        uint maxExtent;
        RoundMode roundMode;
    };

} // nvtt namespace

#endif // NVTT_INPUTOPTIONS_H
