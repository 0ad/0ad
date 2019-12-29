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

#ifndef NV_TT_COMPRESSIONOPTIONS_H
#define NV_TT_COMPRESSIONOPTIONS_H

#include "nvtt.h"
#include "nvmath/Vector.h"
#include "nvcore/StrLib.h"

namespace nvtt
{

    struct CompressionOptions::Private
    {
        Format format;

        Quality quality;

        nv::Vector4 colorWeight;

        // Pixel format description.
        uint bitcount;
        uint rmask;
        uint gmask;
        uint bmask;
        uint amask;
        uint8 rsize;
        uint8 gsize;
        uint8 bsize;
        uint8 asize;

        PixelType pixelType;
        uint pitchAlignment;

        nv::String externalCompressor;

        // Quantization.
        bool enableColorDithering;
        bool enableAlphaDithering;
        bool binaryAlpha;
        int alphaThreshold;			// reference value used for binary alpha quantization.

        Decoder decoder;

        uint getBitCount() const
        {
            if (format == Format_RGBA) {
                if (bitcount != 0) return bitcount;
                else return rsize + gsize + bsize + asize;
            }
            return 0;
        }
    };

} // nvtt namespace


#endif // NV_TT_COMPRESSIONOPTIONS_H
