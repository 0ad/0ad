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

#ifndef NVTT_TEXIMAGE_H
#define NVTT_TEXIMAGE_H

#include "nvtt.h"

#include "nvcore/RefCounted.h"
#include "nvcore/Ptr.h"

#include "nvimage/Image.h"
#include "nvimage/FloatImage.h"

namespace nvtt
{

    struct Surface::Private : public nv::RefCounted
    {
        void operator=(const Private &);
    public:
        Private()
        {
            nvDebugCheck( refCount() == 0 );

            type = TextureType_2D;
            wrapMode = WrapMode_Mirror;
            alphaMode = AlphaMode_None;
            isNormalMap = false;
            
            image = NULL;
        }
        Private(const Private & p) : RefCounted() // Copy ctor. inits refcount to 0.
        {
            nvDebugCheck( refCount() == 0 );

            type = p.type;
            wrapMode = p.wrapMode;
            alphaMode = p.alphaMode;
            isNormalMap = p.isNormalMap;

            image = p.image->clone();
        }
        ~Private()
        {
            delete image;
        }

        TextureType type;
        WrapMode wrapMode;
        AlphaMode alphaMode;
        bool isNormalMap;

        nv::FloatImage * image;
    };

} // nvtt namespace

namespace nv {
    bool canMakeNextMipmap(uint w, uint h, uint d, uint min_size);
    uint countMipmaps(uint w);
    uint countMipmaps(uint w, uint h, uint d);
    uint countMipmapsWithMinSize(uint w, uint h, uint d, uint min_size);
    uint computeImageSize(uint w, uint h, uint d, uint bitCount, uint alignmentInBytes, nvtt::Format format);
    void getTargetExtent(int * w, int * h, int * d, int maxExtent, nvtt::RoundMode roundMode, nvtt::TextureType textureType);
}


#endif // NVTT_TEXIMAGE_H
