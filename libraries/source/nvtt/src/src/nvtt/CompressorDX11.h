// Copyright (c) 2009-2011 Ignacio Castano <castano@gmail.com>
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

#ifndef NVTT_COMPRESSORDX11_H
#define NVTT_COMPRESSORDX11_H

#include "BlockCompressor.h"

namespace nv
{
    struct CompressorBC6 : public FloatColorCompressor
    {
        virtual void compressBlock(const Vector4 colors[16], const float weights[16], const nvtt::CompressionOptions::Private & compressionOptions, void * output);
        virtual uint blockSize() const { return 16; }
    };

    struct CompressorBC7 : public FloatColorCompressor
    {
        virtual void compressBlock(const Vector4 colors[16], const float weights[16], const nvtt::CompressionOptions::Private & compressionOptions, void * output);
        virtual uint blockSize() const { return 16; }
    };
	
} // nv namespace


#endif // NVTT_COMPRESSORDX11_H
