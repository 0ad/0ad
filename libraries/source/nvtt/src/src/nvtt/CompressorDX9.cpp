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

#include "CompressorDX9.h"
#include "QuickCompressDXT.h"
#include "OptimalCompressDXT.h"
#include "CompressionOptions.h"
#include "OutputOptions.h"
#include "ClusterFit.h"
#include "CompressorDXT1.h"
#include "CompressorDXT5_RGBM.h"

// squish
#include "squish/colourset.h"
#include "squish/weightedclusterfit.h"

#include "nvtt.h"

#include "nvimage/Image.h"
#include "nvimage/ColorBlock.h"
#include "nvimage/BlockDXT.h"

#include "nvmath/Vector.inl"
#include "nvmath/Color.inl"

#include "nvcore/Memory.h"

#include <new> // placement new

// s3_quant
#if defined(HAVE_S3QUANT)
#include "s3tc/s3_quant.h"
#endif

// ati tc
#if defined(HAVE_ATITC)
typedef int BOOL;
typedef _W64 unsigned long ULONG_PTR;
typedef ULONG_PTR DWORD_PTR;
#include "atitc/ATI_Compress.h"
#endif

// squish
#if defined(HAVE_SQUISH)
//#include "squish/squish.h"
#include "squish-1.10/squish.h"
#endif

// d3dx
#if defined(HAVE_D3DX)
#include <d3dx9.h>
#endif

// stb
#if defined(HAVE_STB)
#define STB_DEFINE
#include "stb/stb_dxt.h"
#endif

using namespace nv;
using namespace nvtt;


void FastCompressorDXT1::compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    BlockDXT1 * block = new(output) BlockDXT1;
    QuickCompress::compressDXT1(rgba, block);
}

void FastCompressorDXT1a::compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    BlockDXT1 * block = new(output) BlockDXT1;
    QuickCompress::compressDXT1a(rgba, block);
}

void FastCompressorDXT3::compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    BlockDXT3 * block = new(output) BlockDXT3;
    QuickCompress::compressDXT3(rgba, block);
}

void FastCompressorDXT5::compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    BlockDXT5 * block = new(output) BlockDXT5;
    QuickCompress::compressDXT5(rgba, block);
}

void FastCompressorDXT5n::compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    rgba.swizzle(4, 1, 5, 0); // 0xFF, G, 0, R

    BlockDXT5 * block = new(output) BlockDXT5;
    QuickCompress::compressDXT5(rgba, block);
}


#if 1

void CompressorDXT1::compressBlock(const Vector4 colors[16], const float weights[16], const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    compress_dxt1(colors, weights, compressionOptions.colorWeight.xyz(), /*three_color_mode*/true, (BlockDXT1 *)output);
}

#else
void CompressorDXT1::compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    nvsquish::WeightedClusterFit fit;
    fit.SetMetric(compressionOptions.colorWeight.x, compressionOptions.colorWeight.y, compressionOptions.colorWeight.z);

    if (rgba.isSingleColor())
    {
        BlockDXT1 * block = new(output) BlockDXT1;
        OptimalCompress::compressDXT1(rgba.color(0), block);
    }
    else
    {
        nvsquish::ColourSet colours((uint8 *)rgba.colors(), 0);
        fit.SetColourSet(&colours, nvsquish::kDxt1);
        fit.Compress(output);
    }
}
#endif

void CompressorDXT1a::compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    uint alphaMask = 0;
    for (uint i = 0; i < 16; i++)
    {
        if (rgba.color(i).a == 0) alphaMask |= (3 << (i * 2)); // Set two bits for each color.
    }

    const bool isSingleColor = rgba.isSingleColor();

    if (isSingleColor)
    {
        BlockDXT1 * block = new(output) BlockDXT1;
        OptimalCompress::compressDXT1a(rgba.color(0), alphaMask, block);
    }
    else
    {
        nvsquish::WeightedClusterFit fit;
        fit.SetMetric(compressionOptions.colorWeight.x, compressionOptions.colorWeight.y, compressionOptions.colorWeight.z);

        int flags = nvsquish::kDxt1;
        if (alphaMode == nvtt::AlphaMode_Transparency) flags |= nvsquish::kWeightColourByAlpha;

        nvsquish::ColourSet colours((uint8 *)rgba.colors(), flags);
        fit.SetColourSet(&colours, nvsquish::kDxt1);

        fit.Compress(output);
    }
}

void CompressorDXT1_Luma::compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    BlockDXT1 * block = new(output) BlockDXT1;
    OptimalCompress::compressDXT1_Luma(rgba, block);
}

void CompressorDXT3::compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    BlockDXT3 * block = new(output) BlockDXT3;

    // Compress explicit alpha.
    OptimalCompress::compressDXT3A(rgba, &block->alpha);

    // Compress color.
    if (rgba.isSingleColor())
    {
        OptimalCompress::compressDXT1(rgba.color(0), &block->color);
    }
    else
    {
        nvsquish::WeightedClusterFit fit;
        fit.SetMetric(compressionOptions.colorWeight.x, compressionOptions.colorWeight.y, compressionOptions.colorWeight.z);

        int flags = 0;
        if (alphaMode == nvtt::AlphaMode_Transparency) flags |= nvsquish::kWeightColourByAlpha;

        nvsquish::ColourSet colours((uint8 *)rgba.colors(), flags);
        fit.SetColourSet(&colours, 0);
        fit.Compress(&block->color);
    }
}

void CompressorDXT5::compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    BlockDXT5 * block = new(output) BlockDXT5;

    // Compress alpha.
    if (compressionOptions.quality == Quality_Highest)
    {
        OptimalCompress::compressDXT5A(rgba, &block->alpha);
    }
    else
    {
        QuickCompress::compressDXT5A(rgba, &block->alpha);
    }

    // Compress color.
    if (rgba.isSingleColor())
    {
        OptimalCompress::compressDXT1(rgba.color(0), &block->color);
    }
    else
    {
        nvsquish::WeightedClusterFit fit;
        fit.SetMetric(compressionOptions.colorWeight.x, compressionOptions.colorWeight.y, compressionOptions.colorWeight.z);

        int flags = 0;
        if (alphaMode == nvtt::AlphaMode_Transparency) flags |= nvsquish::kWeightColourByAlpha;

        nvsquish::ColourSet colours((uint8 *)rgba.colors(), flags);
        fit.SetColourSet(&colours, 0);
        fit.Compress(&block->color);
    }
}


void CompressorDXT5n::compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    BlockDXT5 * block = new(output) BlockDXT5;

    // Compress Y.
    if (compressionOptions.quality == Quality_Highest)
    {
        OptimalCompress::compressDXT1G(rgba, &block->color);
    }
    else
    {
        if (rgba.isSingleColor(Color32(0, 0xFF, 0, 0))) // Mask all but green channel.
        {
                OptimalCompress::compressDXT1G(rgba.color(0).g, &block->color);
        }
        else
        {
            ColorBlock tile = rgba;
            tile.swizzle(4, 1, 5, 3); // leave alpha in alpha channel.

            nvsquish::WeightedClusterFit fit;
            fit.SetMetric(0, 1, 0);

            int flags = 0;
            if (alphaMode == nvtt::AlphaMode_Transparency) flags |= nvsquish::kWeightColourByAlpha;

            nvsquish::ColourSet colours((uint8 *)tile.colors(), flags);
            fit.SetColourSet(&colours, 0);
            fit.Compress(&block->color);
        }
    }

    rgba.swizzle(4, 1, 5, 0); // 1, G, 0, R

    // Compress X.
    if (compressionOptions.quality == Quality_Highest)
    {
        OptimalCompress::compressDXT5A(rgba, &block->alpha);
    }
    else
    {
        QuickCompress::compressDXT5A(rgba, &block->alpha);
    }
}


void CompressorBC3_RGBM::compressBlock(const Vector4 colors[16], const float weights[16], const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    float min_m = 0.25f; // @@ Get from compression options.
    compress_dxt5_rgbm(colors, weights, min_m, (BlockDXT5 *)output);
}


#if defined(HAVE_ATITC)

void AtiCompressorDXT1::compress(nvtt::InputFormat inputFormat, nvtt::AlphaMode alphaMode, uint w, uint h, uint d, void * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions)
{
    nvDebugCheck(d == 1);

    // Init source texture
    ATI_TC_Texture srcTexture;
    srcTexture.dwSize = sizeof(srcTexture);
    srcTexture.dwWidth = w;
    srcTexture.dwHeight = h;
    if (inputFormat == nvtt::InputFormat_BGRA_8UB)
    {
        srcTexture.dwPitch = w * 4;
        srcTexture.format = ATI_TC_FORMAT_ARGB_8888;
    }
    else
    {
        // @@ Floating point input is not swizzled.
        srcTexture.dwPitch = w * 16;
        srcTexture.format = ATI_TC_FORMAT_ARGB_32F;
    }
    srcTexture.dwDataSize = ATI_TC_CalculateBufferSize(&srcTexture);
    srcTexture.pData = (ATI_TC_BYTE*) data;

    // Init dest texture
    ATI_TC_Texture destTexture;
    destTexture.dwSize = sizeof(destTexture);
    destTexture.dwWidth = w;
    destTexture.dwHeight = h;
    destTexture.dwPitch = 0;
    destTexture.format = ATI_TC_FORMAT_DXT1;
    destTexture.dwDataSize = ATI_TC_CalculateBufferSize(&destTexture);
    destTexture.pData = (ATI_TC_BYTE*) mem::malloc(destTexture.dwDataSize);

    ATI_TC_CompressOptions options;
    options.dwSize = sizeof(options);
    options.bUseChannelWeighting = false;
    options.bUseAdaptiveWeighting = false;
    options.bDXT1UseAlpha = false;
    options.nCompressionSpeed = ATI_TC_Speed_Normal;
    options.bDisableMultiThreading = false;
    //options.bDisableMultiThreading = true;

    // Compress
    ATI_TC_ConvertTexture(&srcTexture, &destTexture, &options, NULL, NULL, NULL);

    if (outputOptions.outputHandler != NULL) {
            outputOptions.outputHandler->writeData(destTexture.pData, destTexture.dwDataSize);
    }

    mem::free(destTexture.pData);
}

void AtiCompressorDXT5::compress(nvtt::InputFormat inputFormat, nvtt::AlphaMode alphaMode, uint w, uint h, uint d, void * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions)
{
    nvDebugCheck(d == 1);

    // Init source texture
    ATI_TC_Texture srcTexture;
    srcTexture.dwSize = sizeof(srcTexture);
    srcTexture.dwWidth = w;
    srcTexture.dwHeight = h;
    if (inputFormat == nvtt::InputFormat_BGRA_8UB)
    {
        srcTexture.dwPitch = w * 4;
        srcTexture.format = ATI_TC_FORMAT_ARGB_8888;
    }
    else
    {
        srcTexture.dwPitch = w * 16;
        srcTexture.format = ATI_TC_FORMAT_ARGB_32F;
    }
    srcTexture.dwDataSize = ATI_TC_CalculateBufferSize(&srcTexture);
    srcTexture.pData = (ATI_TC_BYTE*) data;

    // Init dest texture
    ATI_TC_Texture destTexture;
    destTexture.dwSize = sizeof(destTexture);
    destTexture.dwWidth = w;
    destTexture.dwHeight = h;
    destTexture.dwPitch = 0;
    destTexture.format = ATI_TC_FORMAT_DXT5;
    destTexture.dwDataSize = ATI_TC_CalculateBufferSize(&destTexture);
    destTexture.pData = (ATI_TC_BYTE*) mem::malloc(destTexture.dwDataSize);

    // Compress
    ATI_TC_ConvertTexture(&srcTexture, &destTexture, NULL, NULL, NULL, NULL);

    if (outputOptions.outputHandler != NULL) {
        outputOptions.outputHandler->writeData(destTexture.pData, destTexture.dwDataSize);
    }

    mem::free(destTexture.pData);
}

#endif // defined(HAVE_ATITC)

#if defined(HAVE_SQUISH)

void SquishCompressorDXT1::compress(nvtt::InputFormat inputFormat, nvtt::AlphaMode alphaMode, uint w, uint h, uint d, void * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions)
{
    nvDebugCheck(d == 1);
    nvDebugCheck(false);

#pragma message(NV_FILE_LINE "TODO: Convert input to fixed point ABGR format instead of ARGB")
    /*
    Image img(*image);
    int count = img.width() * img.height();
    for (int i = 0; i < count; i++)
    {
            Color32 c = img.pixel(i);
            img.pixel(i) = Color32(c.b, c.g, c.r, c.a);
    }

    int size = squish::GetStorageRequirements(img.width(), img.height(), squish::kDxt1);
    void * blocks = mem::malloc(size);

    squish::CompressImage((const squish::u8 *)img.pixels(), img.width(), img.height(), blocks, squish::kDxt1 | squish::kColourClusterFit);

    if (outputOptions.outputHandler != NULL) {
            outputOptions.outputHandler->writeData(blocks, size);
    }

    mem::free(blocks);
    */
}

#endif // defined(HAVE_SQUISH)


#if defined(HAVE_D3DX)

void D3DXCompressorDXT1::compress(nvtt::InputFormat inputFormat, nvtt::AlphaMode alphaMode, uint w, uint h, uint d, void * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions)
{
    nvDebugCheck(d == 1);

    IDirect3D9 * d3d = Direct3DCreate9(D3D_SDK_VERSION);

    D3DPRESENT_PARAMETERS presentParams;
    ZeroMemory(&presentParams, sizeof(presentParams));
    presentParams.Windowed = TRUE;
    presentParams.SwapEffect = D3DSWAPEFFECT_COPY;
    presentParams.BackBufferWidth = 8;
    presentParams.BackBufferHeight = 8;
    presentParams.BackBufferFormat = D3DFMT_UNKNOWN;

    HRESULT err;

    IDirect3DDevice9 * device = NULL;
    err = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, GetDesktopWindow(), D3DCREATE_SOFTWARE_VERTEXPROCESSING, &presentParams, &device);

    IDirect3DTexture9 * texture = NULL;
    err = D3DXCreateTexture(device, w, h, 1, 0, D3DFMT_DXT1, D3DPOOL_SYSTEMMEM, &texture);

    IDirect3DSurface9 * surface = NULL;
    err = texture->GetSurfaceLevel(0, &surface);

    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.bottom = h;
    rect.right = w;

    if (inputFormat == nvtt::InputFormat_BGRA_8UB)
    {
        err = D3DXLoadSurfaceFromMemory(surface, NULL, NULL, data, D3DFMT_A8R8G8B8, w * 4, NULL, &rect, D3DX_DEFAULT, 0);
    }
    else
    {
        err = D3DXLoadSurfaceFromMemory(surface, NULL, NULL, data, D3DFMT_A32B32G32R32F, w * 16, NULL, &rect, D3DX_DEFAULT, 0);
    }

    if (err != D3DERR_INVALIDCALL && err != D3DXERR_INVALIDDATA)
    {
        D3DLOCKED_RECT rect;
        ZeroMemory(&rect, sizeof(rect));

        err = surface->LockRect(&rect, NULL, D3DLOCK_READONLY);

	    if (outputOptions.outputHandler != NULL) {
	        int size = rect.Pitch * ((h + 3) / 4);
	        outputOptions.outputHandler->writeData(rect.pBits, size);
	    }

        err = surface->UnlockRect();
    }

    surface->Release();
    device->Release();
    d3d->Release();
}

#endif // defined(HAVE_D3DX)


#if defined(HAVE_STB)

void StbCompressorDXT1::compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    rgba.swizzle(2, 1, 0, 3); // Swap R and B
    stb_compress_dxt_block((unsigned char *)output, (unsigned char *)rgba.colors(), 0, 0);
}


#endif // defined(HAVE_STB)
