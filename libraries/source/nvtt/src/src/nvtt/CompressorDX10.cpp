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

#include "CompressorDX10.h"
#include "QuickCompressDXT.h"
#include "OptimalCompressDXT.h"

#include "nvtt.h"

#include "nvimage/ColorBlock.h"
#include "nvimage/BlockDXT.h"

#include "nvmath/ftoi.h"

#include <new> // placement new

using namespace nv;
using namespace nvtt;


void FastCompressorBC4::compressBlock(ColorBlock & src, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
	BlockATI1 * block = new(output) BlockATI1;
	
    AlphaBlock4x4 tmp;
    tmp.init(src, 0);  // Copy red to alpha
	QuickCompress::compressDXT5A(tmp, &block->alpha);
}

void FastCompressorBC5::compressBlock(ColorBlock & src, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
	BlockATI2 * block = new(output) BlockATI2;
	
    AlphaBlock4x4 tmp;

    tmp.init(src, 0);  // Copy red to alpha
	QuickCompress::compressDXT5A(tmp, &block->x);
	
    tmp.init(src, 1);  // Copy green to alpha
	QuickCompress::compressDXT5A(tmp, &block->y);
}


void ProductionCompressorBC4::compressBlock(ColorBlock & src, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
	BlockATI1 * block = new(output) BlockATI1;

    AlphaBlock4x4 tmp;
    tmp.init(src, 0);  // Copy red to alpha
	OptimalCompress::compressDXT5A(tmp, &block->alpha);
}

void ProductionCompressorBC5::compressBlock(ColorBlock & src, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
	BlockATI2 * block = new(output) BlockATI2;

    AlphaBlock4x4 tmp;

    tmp.init(src, 0);  // Copy red to alpha
	OptimalCompress::compressDXT5A(tmp, &block->x);
	
    tmp.init(src, 1);  // Copy green to alpha
	OptimalCompress::compressDXT5A(tmp, &block->y);
}


#if 0
void ProductionCompressorBC5_Luma::compressBlock(ColorSet & set, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    BlockATI2 * block = new(output) BlockATI2;

    AlphaBlock4x4 tmp;
    tmp.init(set, /*channel=*/0);
    OptimalCompress::compressDXT5A(tmp, &block->x);

    // Decode block->x
    AlphaBlock4x4 decoded;
    block->x.decodeBlock(&decoded);

    const float R = 1.0f / 256.0f; // Maximum residual that we can represent. @@ Tweak this.

    // Compute residual block.
    for (int i = 0; i < 16; i++) {
        float in = set.color(i).x;                      // [0,1]
        float out = float(decoded.alpha[i]) / 255.0f;   // [0,1]

        float residual = (out - in);                    // [-1,1], but usually [-R,R]

        // Normalize residual to [-1,1] range.
        residual /= R;

        // Pack in [0,1] range.
        residual = residual * 0.5f + 0.5f;

        tmp.alpha[i] = nv::ftoi_round(nv::saturate(residual) * 255.0f);
    }

    OptimalCompress::compressDXT5A(tmp, &block->y);
   
}
#endif // 0