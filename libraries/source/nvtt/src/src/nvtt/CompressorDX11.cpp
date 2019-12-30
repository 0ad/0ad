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

#include "CompressorDX11.h"

#include "nvtt.h"
#include "CompressionOptions.h"
#include "nvimage/ColorBlock.h"
#include "nvmath/Half.h"
#include "nvmath/Vector.inl"

#include "bc6h/zoh.h"
#include "bc7/avpcl.h"

#include <string.h> // memset

using namespace nv;
using namespace nvtt;


void CompressorBC6::compressBlock(const Vector4 colors[16], const float weights[16], const CompressionOptions::Private & compressionOptions, void * output)
{
    // !!!UNDONE: support channel weights
    // !!!UNDONE: set flags once, not per block (this is especially sketchy since block compression is multithreaded...)

    if (compressionOptions.pixelType == PixelType_UnsignedFloat ||
        compressionOptions.pixelType == PixelType_UnsignedNorm ||
        compressionOptions.pixelType == PixelType_UnsignedInt)
    {
        ZOH::Utils::FORMAT = ZOH::UNSIGNED_F16;
    }
    else
    {
        ZOH::Utils::FORMAT = ZOH::SIGNED_F16;
    }

    // Convert NVTT's tile struct to ZOH's, and convert float to half.
    ZOH::Tile zohTile(4, 4);
    memset(zohTile.data, 0, sizeof(zohTile.data));
    memset(zohTile.importance_map, 0, sizeof(zohTile.importance_map));
    for (uint y = 0; y < 4; ++y)
    {
        for (uint x = 0; x < 4; ++x)
        {
            Vector4 color = colors[4*y+x];
            uint16 rHalf = to_half(color.x);
            uint16 gHalf = to_half(color.y);
            uint16 bHalf = to_half(color.z);
            zohTile.data[y][x].x = ZOH::Tile::half2float(rHalf);
            zohTile.data[y][x].y = ZOH::Tile::half2float(gHalf);
            zohTile.data[y][x].z = ZOH::Tile::half2float(bHalf);
            zohTile.importance_map[y][x] = weights[4*y+x];
        }
    }

    ZOH::compress(zohTile, (char *)output);
}

void CompressorBC7::compressBlock(const Vector4 colors[16], const float weights[16], const CompressionOptions::Private & compressionOptions, void * output)
{
    // !!!UNDONE: support channel weights
    // !!!UNDONE: set flags once, not per block (this is especially sketchy since block compression is multithreaded...)

    AVPCL::mode_rgb = false;
    AVPCL::flag_premult = false; //(alphaMode == AlphaMode_Premultiplied);
    AVPCL::flag_nonuniform = false;
    AVPCL::flag_nonuniform_ati = false;
    
    // Convert NVTT's tile struct to AVPCL's.
    AVPCL::Tile avpclTile(4, 4);
    memset(avpclTile.data, 0, sizeof(avpclTile.data));
    for (uint y = 0; y < 4; ++y) {
        for (uint x = 0; x < 4; ++x) {
            Vector4 color = colors[4*y+x];
            avpclTile.data[y][x] = color * 255.0f;
            avpclTile.importance_map[y][x] = 1.0f; //weights[4*y+x];
        }
    }

    AVPCL::compress(avpclTile, (char *)output);
}
