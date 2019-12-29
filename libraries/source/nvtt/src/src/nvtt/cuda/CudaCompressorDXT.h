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

#ifndef NV_TT_CUDACOMPRESSORDXT_H
#define NV_TT_CUDACOMPRESSORDXT_H

#include "nvtt/nvtt.h"
#include "nvtt/Compressor.h" // CompressorInterface

struct cudaArray;

namespace nv
{
    class CudaContext
    {
    public:
        CudaContext();
        ~CudaContext();

        bool isValid() const;

    public:
        // Device pointers.
        uint * bitmapTable;
        uint * bitmapTableCTX;
        uint * data;
        uint * result;
    };

#if defined HAVE_CUDA

    struct CudaCompressor : public CompressorInterface
    {
        CudaCompressor(CudaContext & ctx);

        virtual void compress(nvtt::AlphaMode alphaMode, uint w, uint h, uint d, const float * data, nvtt::TaskDispatcher * dispatcher, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions);

        virtual void setup(cudaArray * image, const nvtt::CompressionOptions::Private & compressionOptions) = 0;
        virtual void compressBlocks(uint first, uint count, uint w, uint h, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output) = 0;
        virtual uint blockSize() const = 0;

    protected:
        CudaContext & m_ctx;
    };

    struct CudaCompressorDXT1 : public CudaCompressor
    {
        CudaCompressorDXT1(CudaContext & ctx) : CudaCompressor(ctx) {}

        virtual void setup(cudaArray * image, const nvtt::CompressionOptions::Private & compressionOptions);
        virtual void compressBlocks(uint first, uint count, uint w, uint h, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output);
        virtual uint blockSize() const { return 8; };
    };

    /*struct CudaCompressorDXT1n : public CudaCompressor
    {
        virtual void setup(const CompressionOptions::Private & compressionOptions);
        virtual void compressBlocks(uint blockCount, const void * input, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output) = 0;
        virtual uint blockSize() const { return 8; };
    };*/

    struct CudaCompressorDXT3 : public CudaCompressor
    {
        CudaCompressorDXT3(CudaContext & ctx) : CudaCompressor(ctx) {}

        virtual void setup(cudaArray * image, const nvtt::CompressionOptions::Private & compressionOptions);
        virtual void compressBlocks(uint first, uint count, uint w, uint h, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output);
        virtual uint blockSize() const { return 16; };
    };

    struct CudaCompressorDXT5 : public CudaCompressor
    {
        CudaCompressorDXT5(CudaContext & ctx) : CudaCompressor(ctx) {}

        virtual void setup(cudaArray * image, const nvtt::CompressionOptions::Private & compressionOptions);
        virtual void compressBlocks(uint first, uint count, uint w, uint h, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output);
        virtual uint blockSize() const { return 16; };
    };

    /*struct CudaCompressorCXT1 : public CudaCompressor
    {
        virtual void setup(const CompressionOptions::Private & compressionOptions);
        virtual void compressBlocks(uint blockCount, const void * input, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output) = 0;
        virtual uint blockSize() const { return 8; };
    };*/

#endif // defined HAVE_CUDA

} // nv namespace


#endif // NV_TT_CUDAUTILS_H
