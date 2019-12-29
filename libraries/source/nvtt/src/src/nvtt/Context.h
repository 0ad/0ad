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

#ifndef NV_TT_CONTEXT_H
#define NV_TT_CONTEXT_H

#include "nvcore/Ptr.h"

#include "nvtt/Compressor.h"
#include "nvtt/cuda/CudaCompressorDXT.h"
#include "nvtt.h"
#include "TaskDispatcher.h"

namespace nv
{
    class Image;
}

namespace nvtt
{
    struct Mipmap;

    struct Compressor::Private
    {
        Private() {}

        bool compress(const InputOptions::Private & inputOptions, const CompressionOptions::Private & compressionOptions, const OutputOptions::Private & outputOptions) const;
        bool compress(const Surface & tex, int face, int mipmap, const CompressionOptions::Private & compressionOptions, const OutputOptions::Private & outputOptions) const;
        bool compress(AlphaMode alphaMode, int w, int h, int d, int face, int mipmap, const float * data, const CompressionOptions::Private & compressionOptions, const OutputOptions::Private & outputOptions) const;

        void quantize(Surface & tex, const CompressionOptions::Private & compressionOptions) const;

        bool outputHeader(nvtt::TextureType textureType, int w, int h, int d, int faceCount, int mipmapCount, bool isNormalMap, const CompressionOptions::Private & compressionOptions, const OutputOptions::Private & outputOptions) const;

        nv::CompressorInterface * chooseCpuCompressor(const CompressionOptions::Private & compressionOptions) const;
        nv::CompressorInterface * chooseGpuCompressor(const CompressionOptions::Private & compressionOptions) const;


        bool cudaSupported;
        bool cudaEnabled;

        nv::AutoPtr<nv::CudaContext> cuda;

        TaskDispatcher * dispatcher;
        //SequentialTaskDispatcher defaultDispatcher;
        ConcurrentTaskDispatcher defaultDispatcher;
    };

} // nvtt namespace


#endif // NV_TT_CONTEXT_H
