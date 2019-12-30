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

#include "CudaCompressorDXT.h"
#include "CudaUtils.h"

#include "nvcore/Debug.h"
#include "nvmath/Color.h"
#include "nvmath/Vector.inl"
#include "nvimage/Image.h"
#include "nvimage/ColorBlock.h"
#include "nvimage/BlockDXT.h"
#include "nvtt/CompressionOptions.h"
#include "nvtt/OutputOptions.h"
#include "nvtt/QuickCompressDXT.h"
#include "nvtt/OptimalCompressDXT.h"

#include <time.h>
#include <stdio.h>

#if defined HAVE_CUDA
#include <cuda_runtime_api.h>

#define MAX_BLOCKS 8192U // 32768, 65535 // @@ Limit number of blocks on slow devices to prevent hitting the watchdog timer.

extern "C" void setupOMatchTables(const void * OMatch5Src, size_t OMatch5Size, const void * OMatch6Src, size_t OMatch6Size);
extern "C" void setupCompressKernel(const float weights[3]);
extern "C" void bindTextureToArray(cudaArray * d_data);

extern "C" void compressKernelDXT1(uint firstBlock, uint blockNum, uint w, uint * d_result, uint * d_bitmaps);
extern "C" void compressKernelDXT1_Level4(uint blockNum, uint * d_data, uint * d_result, uint * d_bitmaps);
extern "C" void compressWeightedKernelDXT1(uint blockNum, uint * d_data, uint * d_result, uint * d_bitmaps);
extern "C" void compressKernelDXT3(uint firstBlock, uint blockNum, uint w, uint * d_result, uint * d_bitmaps);
//extern "C" void compressNormalKernelDXT1(uint blockNum, uint * d_data, uint * d_result, uint * d_bitmaps);
//extern "C" void compressKernelCTX1(uint blockNum, uint * d_data, uint * d_result, uint * d_bitmaps);

#include "BitmapTable.h"
#include "nvtt/SingleColorLookup.h"

#endif

using namespace nv;
using namespace nvtt;


CudaContext::CudaContext() : 
	bitmapTable(NULL), 
	bitmapTableCTX(NULL), 
	data(NULL), 
	result(NULL)
{
#if defined HAVE_CUDA
    // Allocate and upload bitmaps.
    cudaMalloc((void**) &bitmapTable, 992 * sizeof(uint));
    if (bitmapTable != NULL)
    {
        cudaMemcpy(bitmapTable, s_bitmapTable, 992 * sizeof(uint), cudaMemcpyHostToDevice);
    }

    cudaMalloc((void**) &bitmapTableCTX, 704 * sizeof(uint));
    if (bitmapTableCTX != NULL)
    {
        cudaMemcpy(bitmapTableCTX, s_bitmapTableCTX, 704 * sizeof(uint), cudaMemcpyHostToDevice);
    }

    // Allocate scratch buffers.
    cudaMalloc((void**) &data, MAX_BLOCKS * 64U);
    cudaMalloc((void**) &result, MAX_BLOCKS * 8U);

    // Init single color lookup contant tables.
	setupOMatchTables(OMatch5, sizeof(OMatch5), OMatch6, sizeof(OMatch6));
#endif
}

CudaContext::~CudaContext()
{
#if defined HAVE_CUDA
    // Free device mem allocations.
    cudaFree(bitmapTableCTX);
    cudaFree(bitmapTable);
    cudaFree(data);
    cudaFree(result);
#endif
}

bool CudaContext::isValid() const
{
#if defined HAVE_CUDA
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess)
    {
        nvDebug("*** CUDA Error: %s\n", cudaGetErrorString(err));
        return false;
    }
#endif
    return bitmapTable != NULL && bitmapTableCTX != NULL && data != NULL && result != NULL;
}


#if defined HAVE_CUDA

CudaCompressor::CudaCompressor(CudaContext & ctx) : m_ctx(ctx)
{

}

void CudaCompressor::compress(nvtt::AlphaMode alphaMode, uint w, uint h, uint d, const float * data, nvtt::TaskDispatcher * dispatcher, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions)
{
    nvDebugCheck(d == 1);
    nvDebugCheck(cuda::isHardwarePresent());

#if defined HAVE_CUDA

    // Allocate image as a cuda array.
    const uint count = w * h;
    Color32 * tmp = malloc<Color32>(count);
    for (uint i = 0; i < count; i++) {
        tmp[i].r = uint8(clamp(data[i + count*0], 0.0f, 1.0f) * 255);
        tmp[i].g = uint8(clamp(data[i + count*1], 0.0f, 1.0f) * 255);
        tmp[i].b = uint8(clamp(data[i + count*2], 0.0f, 1.0f) * 255);
        tmp[i].a = uint8(clamp(data[i + count*3], 0.0f, 1.0f) * 255);
    }

    cudaArray * d_image;
    cudaChannelFormatDesc channelDesc = cudaCreateChannelDesc(8, 8, 8, 8, cudaChannelFormatKindUnsigned);
    cudaMallocArray(&d_image, &channelDesc, w, h);

    cudaMemcpyToArray(d_image, 0, 0, tmp, count * sizeof(Color32), cudaMemcpyHostToDevice);

    free(tmp);

    // To avoid the copy we could keep the data in floating point format, but the channels are not interleaved like the kernel expects.
    /*
    cudaChannelFormatDesc channelDesc = cudaCreateChannelDesc(32, 32, 32, 32, cudaChannelFormatKindFloat);
    cudaMallocArray(&d_image, &channelDesc, w, h);

    const int imageSize = w * h * sizeof(float) * 4;
    cudaMemcpyToArray(d_image, 0, 0, data, imageSize, cudaMemcpyHostToDevice);
    */

    // Image size in blocks.
    const uint bw = (w + 3) / 4;
    const uint bh = (h + 3) / 4;
    const uint bs = blockSize();
    const uint blockNum = bw * bh;
    //const uint compressedSize = blockNum * bs;

    void * h_result = ::malloc(min(blockNum, MAX_BLOCKS) * bs);

    setup(d_image, compressionOptions);

    // Timer timer;
    // timer.start();

    uint bn = 0;
    while (bn != blockNum)
    {
        uint count = min(blockNum - bn, MAX_BLOCKS);

        compressBlocks(bn, count, bw, bh, alphaMode, compressionOptions, h_result);

        // Check for errors.
        cudaError_t err = cudaGetLastError();
        if (err != cudaSuccess)
        {
            //nvDebug("CUDA Error: %s\n", cudaGetErrorString(err));
            outputOptions.error(Error_CudaError);
        }

        // Output result.
        outputOptions.writeData(h_result, count * bs);

        bn += count;
    }

    //timer.stop();
    //printf("\rCUDA time taken: %.3f seconds\n", timer.elapsed() / CLOCKS_PER_SEC);

    free(h_result);
    cudaFreeArray(d_image);

#else
    outputOptions.error(Error_CudaError);
#endif
}

#if defined HAVE_CUDA

void CudaCompressorDXT1::setup(cudaArray * image, const nvtt::CompressionOptions::Private & compressionOptions)
{
    setupCompressKernel(compressionOptions.colorWeight.ptr());
    bindTextureToArray(image);
}

void CudaCompressorDXT1::compressBlocks(uint first, uint count, uint bw, uint bh, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    // Launch kernel.
    compressKernelDXT1(first, count, bw, m_ctx.result, m_ctx.bitmapTable);

    // Copy result to host.
    cudaMemcpy(output, m_ctx.result, count * 8, cudaMemcpyDeviceToHost);
}


void CudaCompressorDXT3::setup(cudaArray * image, const nvtt::CompressionOptions::Private & compressionOptions)
{
    setupCompressKernel(compressionOptions.colorWeight.ptr());
    bindTextureToArray(image);
}

void CudaCompressorDXT3::compressBlocks(uint first, uint count, uint bw, uint bh, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    // Launch kernel.
    compressKernelDXT3(first, count, bw, m_ctx.result, m_ctx.bitmapTable);

    // Copy result to host.
    cudaMemcpy(output, m_ctx.result, count * 16, cudaMemcpyDeviceToHost);
}


void CudaCompressorDXT5::setup(cudaArray * image, const nvtt::CompressionOptions::Private & compressionOptions)
{
    setupCompressKernel(compressionOptions.colorWeight.ptr());
    bindTextureToArray(image);
}

void CudaCompressorDXT5::compressBlocks(uint first, uint count, uint bw, uint bh, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    /*// Launch kernel.
    compressKernelDXT5(first, count, bw, m_ctx.result, m_ctx.bitmapTable);

    // Copy result to host.
    cudaMemcpy(output, m_ctx.result, count * 16, cudaMemcpyDeviceToHost);*/

    // Launch kernel.
    if (alphaMode == AlphaMode_Transparency)
    {
    //	compressWeightedKernelDXT1(first, count, bw, m_ctx.result, m_ctx.bitmapTable);
    }
    else
    {
    //	compressKernelDXT1_Level4(first, count, w, m_ctx.result, m_ctx.bitmapTable);
    }

    // Compress alpha in parallel with the GPU.
    for (uint i = 0; i < count; i++)
    {
        //ColorBlock rgba(blockLinearImage + (first + i) * 16);
        //OptimalCompress::compressDXT3A(rgba, alphaBlocks + i);
    }

    // Copy result to host.
    cudaMemcpy(output, m_ctx.result, count * 8, cudaMemcpyDeviceToHost);

    // @@ Interleave color and alpha blocks.

}

#endif // defined HAVE_CUDA




// @@ This code is very repetitive and needs to be cleaned up.

#if 0


/*
// Convert linear image to block linear.
static void convertToBlockLinear(const Image * image, uint * blockLinearImage)
{
	const uint w = (image->width() + 3) / 4;
	const uint h = (image->height() + 3) / 4;

	for(uint by = 0; by < h; by++) {
		for(uint bx = 0; bx < w; bx++) {
			const uint bw = min(image->width() - bx * 4, 4U);
			const uint bh = min(image->height() - by * 4, 4U);

			for (uint i = 0; i < 16; i++) {
				const int x = (i % 4) % bw;
				const int y = (i / 4) % bh;
				blockLinearImage[(by * w + bx) * 16 + i] = image->pixel(bx * 4 + x, by * 4 + y).u;
			}
		}
	}
}
*/


/// Compress image using CUDA.
void CudaCompressor::compressDXT3(const CompressionOptions::Private & compressionOptions, const OutputOptions::Private & outputOptions)
{
	nvDebugCheck(cuda::isHardwarePresent());
#if defined HAVE_CUDA

	// Image size in blocks.
	const uint w = (m_image->width() + 3) / 4;
	const uint h = (m_image->height() + 3) / 4;

	uint imageSize = w * h * 16 * sizeof(Color32);
    uint * blockLinearImage = (uint *) malloc(imageSize);
	convertToBlockLinear(m_image, blockLinearImage);

	const uint blockNum = w * h;
	const uint compressedSize = blockNum * 8;

	AlphaBlockDXT3 * alphaBlocks = NULL;
	alphaBlocks = (AlphaBlockDXT3 *)malloc(min(compressedSize, MAX_BLOCKS * 8U));

	setupCompressKernel(compressionOptions.colorWeight.ptr());
	
	clock_t start = clock();

	uint bn = 0;
	while(bn != blockNum)
	{
		uint count = min(blockNum - bn, MAX_BLOCKS);

	    cudaMemcpy(m_ctx.data, blockLinearImage + bn * 16, count * 64, cudaMemcpyHostToDevice);

		// Launch kernel.
		if (m_alphaMode == AlphaMode_Transparency)
		{
			compressWeightedKernelDXT1(count, m_ctx.data, m_ctx.result, m_ctx.bitmapTable);
		}
		else
		{
			compressKernelDXT1_Level4(count, m_ctx.data, m_ctx.result, m_ctx.bitmapTable);
		}

		// Compress alpha in parallel with the GPU.
		for (uint i = 0; i < count; i++)
		{
			ColorBlock rgba(blockLinearImage + (bn + i) * 16);
			OptimalCompress::compressDXT3A(rgba, alphaBlocks + i);
		}

		// Check for errors.
		cudaError_t err = cudaGetLastError();
		if (err != cudaSuccess)
		{
			nvDebug("CUDA Error: %s\n", cudaGetErrorString(err));
			outputOptions.error(Error_CudaError);
		}

		// Copy result to host, overwrite swizzled image.
		cudaMemcpy(blockLinearImage, m_ctx.result, count * 8, cudaMemcpyDeviceToHost);

		// Output result.
		for (uint i = 0; i < count; i++)
		{
			outputOptions.writeData(alphaBlocks + i, 8);
			outputOptions.writeData(blockLinearImage + i * 2, 8);
		}

		bn += count;
	}

	clock_t end = clock();
	//printf("\rCUDA time taken: %.3f seconds\n", float(end-start) / CLOCKS_PER_SEC);

	free(alphaBlocks);
	free(blockLinearImage);

#else
	outputOptions.error(Error_CudaError);
#endif
}


/// Compress image using CUDA.
void CudaCompressor::compressDXT5(const CompressionOptions::Private & compressionOptions, const OutputOptions::Private & outputOptions)
{
	nvDebugCheck(cuda::isHardwarePresent());
#if defined HAVE_CUDA

	// Image size in blocks.
	const uint w = (m_image->width() + 3) / 4;
	const uint h = (m_image->height() + 3) / 4;

	uint imageSize = w * h * 16 * sizeof(Color32);
    uint * blockLinearImage = (uint *) malloc(imageSize);
	convertToBlockLinear(m_image, blockLinearImage);

	const uint blockNum = w * h;
	const uint compressedSize = blockNum * 8;

	AlphaBlockDXT5 * alphaBlocks = NULL;
	alphaBlocks = (AlphaBlockDXT5 *)malloc(min(compressedSize, MAX_BLOCKS * 8U));

	setupCompressKernel(compressionOptions.colorWeight.ptr());
	
	clock_t start = clock();

	uint bn = 0;
	while(bn != blockNum)
	{
		uint count = min(blockNum - bn, MAX_BLOCKS);

	    cudaMemcpy(m_ctx.data, blockLinearImage + bn * 16, count * 64, cudaMemcpyHostToDevice);

		// Launch kernel.
		if (m_alphaMode == AlphaMode_Transparency)
		{
			compressWeightedKernelDXT1(count, m_ctx.data, m_ctx.result, m_ctx.bitmapTable);
		}
		else
		{
			compressKernelDXT1_Level4(count, m_ctx.data, m_ctx.result, m_ctx.bitmapTable);
		}

		// Compress alpha in parallel with the GPU.
		for (uint i = 0; i < count; i++)
		{
			ColorBlock rgba(blockLinearImage + (bn + i) * 16);
			QuickCompress::compressDXT5A(rgba, alphaBlocks + i);
		}

		// Check for errors.
		cudaError_t err = cudaGetLastError();
		if (err != cudaSuccess)
		{
			nvDebug("CUDA Error: %s\n", cudaGetErrorString(err));
			outputOptions.error(Error_CudaError);
		}

		// Copy result to host, overwrite swizzled image.
		cudaMemcpy(blockLinearImage, m_ctx.result, count * 8, cudaMemcpyDeviceToHost);

		// Output result.
		for (uint i = 0; i < count; i++)
		{
			outputOptions.writeData(alphaBlocks + i, 8);
			outputOptions.writeData(blockLinearImage + i * 2, 8);
		}

		bn += count;
	}

	clock_t end = clock();
	//printf("\rCUDA time taken: %.3f seconds\n", float(end-start) / CLOCKS_PER_SEC);

	free(alphaBlocks);
	free(blockLinearImage);

#else
	outputOptions.error(Error_CudaError);
#endif
}


void CudaCompressor::compressDXT1n(const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions)
{
	nvDebugCheck(cuda::isHardwarePresent());
#if defined HAVE_CUDA

	// Image size in blocks.
	const uint w = (m_image->width() + 3) / 4;
	const uint h = (m_image->height() + 3) / 4;

	uint imageSize = w * h * 16 * sizeof(Color32);
    uint * blockLinearImage = (uint *) malloc(imageSize);
	convertToBlockLinear(m_image, blockLinearImage);	// @@ Do this in parallel with the GPU, or in the GPU!

	const uint blockNum = w * h;
	const uint compressedSize = blockNum * 8;

	clock_t start = clock();

	setupCompressKernel(compressionOptions.colorWeight.ptr());
	
	// TODO: Add support for multiple GPUs.
	uint bn = 0;
	while(bn != blockNum)
	{
		uint count = min(blockNum - bn, MAX_BLOCKS);

	    cudaMemcpy(m_ctx.data, blockLinearImage + bn * 16, count * 64, cudaMemcpyHostToDevice);

		// Launch kernel.
		compressNormalKernelDXT1(count, m_ctx.data, m_ctx.result, m_ctx.bitmapTable);

		// Check for errors.
		cudaError_t err = cudaGetLastError();
		if (err != cudaSuccess)
		{
			nvDebug("CUDA Error: %s\n", cudaGetErrorString(err));
			outputOptions.error(Error_CudaError);
		}

		// Copy result to host, overwrite swizzled image.
		cudaMemcpy(blockLinearImage, m_ctx.result, count * 8, cudaMemcpyDeviceToHost);

		// Output result.
		outputOptions.writeData(blockLinearImage, count * 8);

		bn += count;
	}

	clock_t end = clock();
	//printf("\rCUDA time taken: %.3f seconds\n", float(end-start) / CLOCKS_PER_SEC);

	free(blockLinearImage);

#else
	outputOptions.error(Error_CudaError);
#endif
}


void CudaCompressor::compressCTX1(const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions)
{
	nvDebugCheck(cuda::isHardwarePresent());
#if defined HAVE_CUDA

	// Image size in blocks.
	const uint w = (m_image->width() + 3) / 4;
	const uint h = (m_image->height() + 3) / 4;

	uint imageSize = w * h * 16 * sizeof(Color32);
    uint * blockLinearImage = (uint *) malloc(imageSize);
	convertToBlockLinear(m_image, blockLinearImage);	// @@ Do this in parallel with the GPU, or in the GPU!

	const uint blockNum = w * h;
	const uint compressedSize = blockNum * 8;

	clock_t start = clock();

	setupCompressKernel(compressionOptions.colorWeight.ptr());
	
	// TODO: Add support for multiple GPUs.
	uint bn = 0;
	while(bn != blockNum)
	{
		uint count = min(blockNum - bn, MAX_BLOCKS);

	    cudaMemcpy(m_ctx.data, blockLinearImage + bn * 16, count * 64, cudaMemcpyHostToDevice);

		// Launch kernel.
		compressKernelCTX1(count, m_ctx.data, m_ctx.result, m_ctx.bitmapTableCTX);

		// Check for errors.
		cudaError_t err = cudaGetLastError();
		if (err != cudaSuccess)
		{
			nvDebug("CUDA Error: %s\n", cudaGetErrorString(err));

			outputOptions.error(Error_CudaError);
		}

		// Copy result to host, overwrite swizzled image.
		cudaMemcpy(blockLinearImage, m_ctx.result, count * 8, cudaMemcpyDeviceToHost);

		// Output result.
		outputOptions.writeData(blockLinearImage, count * 8);

		bn += count;
	}

	clock_t end = clock();
	//printf("\rCUDA time taken: %.3f seconds\n", float(end-start) / CLOCKS_PER_SEC);

	free(blockLinearImage);

#else
	outputOptions.error(Error_CudaError);
#endif
}


void CudaCompressor::compressDXT5n(const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions)
{
	nvDebugCheck(cuda::isHardwarePresent());
#if defined HAVE_CUDA

	// @@ TODO

#else
	outputOptions.error(Error_CudaError);
#endif
}

#endif // 0

#endif // defined HAVE_CUDA
