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

#include "CompressionOptions.h"
#include "nvimage/DirectDrawSurface.h"
#include "nvmath/Vector.inl"

using namespace nv;
using namespace nvtt;


/// Constructor. Sets compression options to the default values.
CompressionOptions::CompressionOptions() : m(*new CompressionOptions::Private())
{
    reset();
}


/// Destructor.
CompressionOptions::~CompressionOptions()
{
    delete &m;
}


/// Set default compression options.
void CompressionOptions::reset()
{
    m.format = Format_DXT1;
    m.quality = Quality_Normal;
    m.colorWeight.set(1.0f, 1.0f, 1.0f, 1.0f);

    m.bitcount = 32;
    m.bmask = 0x000000FF;
    m.gmask = 0x0000FF00;
    m.rmask = 0x00FF0000;
    m.amask = 0xFF000000;

    m.rsize = 8;
    m.gsize = 8;
    m.bsize = 8;
    m.asize = 8;
	
    m.pixelType = PixelType_UnsignedNorm;
    m.pitchAlignment = 1;

    m.enableColorDithering = false;
    m.enableAlphaDithering = false;
    m.binaryAlpha = false;
    m.alphaThreshold = 127;

    m.decoder = Decoder_D3D10;
}


/// Set desired compression format.
void CompressionOptions::setFormat(Format format)
{
    m.format = format;
}


/// Set compression quality settings.
void CompressionOptions::setQuality(Quality quality)
{
    m.quality = quality;
}


/// Set the weights of each color channel. 
/// The choice for these values is subjective. In most cases uniform color weights
/// (1.0, 1.0, 1.0) work very well. A popular choice is to use the NTSC luma encoding 
/// weights (0.2126, 0.7152, 0.0722), but I think that blue contributes to our 
/// perception more than a 7%. A better choice in my opinion is (3, 4, 2).
void CompressionOptions::setColorWeights(float red, float green, float blue, float alpha/*=1.0f*/)
{
//    float total = red + green + blue;
//    float x = red / total;
//    float y = green / total;
//    m.colorWeight.set(x, y, 1.0f - x - y);
    m.colorWeight.set(red, green, blue, alpha);
}


/// Set color mask to describe the RGB/RGBA format.
void CompressionOptions::setPixelFormat(uint bitCount, uint rmask, uint gmask, uint bmask, uint amask)
{
    // Validate arguments.
    nvCheck(bitCount <= 32);
    nvCheck((rmask & gmask) == 0);
    nvCheck((rmask & bmask) == 0);
    nvCheck((rmask & amask) == 0);
    nvCheck((gmask & bmask) == 0);
    nvCheck((gmask & amask) == 0);
    nvCheck((bmask & amask) == 0);

    if (bitCount != 32)
    {
        uint maxMask = (1 << bitCount);
        nvCheck(maxMask > rmask);
        nvCheck(maxMask > gmask);
        nvCheck(maxMask > bmask);
        nvCheck(maxMask > amask);
    }

    m.bitcount = bitCount;
    m.rmask = rmask;
    m.gmask = gmask;
    m.bmask = bmask;
    m.amask = amask;

    m.rsize = 0;
    m.gsize = 0;
    m.bsize = 0;
    m.asize = 0;
}

void CompressionOptions::setPixelFormat(uint8 rsize, uint8 gsize, uint8 bsize, uint8 asize)
{
    nvCheck(rsize <= 32 && gsize <= 32 && bsize <= 32 && asize <= 32);

    m.bitcount = 0;
    m.rmask = 0;
    m.gmask = 0;
    m.bmask = 0;
    m.amask = 0;

    m.rsize = rsize;
    m.gsize = gsize;
    m.bsize = bsize;
    m.asize = asize;
}

/// Set pixel type.
void CompressionOptions::setPixelType(PixelType pixelType)
{
    m.pixelType = pixelType;
}


/// Set pitch alignment in bytes.
void CompressionOptions::setPitchAlignment(int pitchAlignment)
{
    nvDebugCheck(pitchAlignment > 0 && isPowerOfTwo(pitchAlignment));
    m.pitchAlignment = pitchAlignment;
}


/// Use external compressor.
void CompressionOptions::setExternalCompressor(const char * name)
{
    m.externalCompressor = name;
}

/// Set quantization options.
/// @warning Do not enable dithering unless you know what you are doing. Quantization 
/// introduces errors. It's better to let the compressor quantize the result to 
/// minimize the error, instead of quantizing the data before handling it to
/// the compressor.
void CompressionOptions::setQuantization(bool colorDithering, bool alphaDithering, bool binaryAlpha, int alphaThreshold/*= 127*/)
{
    nvCheck(alphaThreshold >= 0 && alphaThreshold < 256);
    m.enableColorDithering = colorDithering;
    m.enableAlphaDithering = alphaDithering;
    m.binaryAlpha = binaryAlpha;
    m.alphaThreshold = alphaThreshold;
}

/// Set target decoder to optimize for.
void CompressionOptions::setTargetDecoder(Decoder decoder)
{
    m.decoder = decoder;
}



// Translate to and from D3D formats.
unsigned int CompressionOptions::d3d9Format() const
{
    if (m.format == Format_RGB) {
        if (m.pixelType == PixelType_UnsignedNorm) {
            
            uint bitcount = m.bitcount;
            uint rmask = m.rmask;
            uint gmask = m.gmask;
            uint bmask = m.bmask;
            uint amask = m.amask;

            if (bitcount == 0) {
                bitcount = m.rsize + m.gsize + m.bsize + m.asize;
                rmask = ((1 << m.rsize) - 1) << (m.asize + m.bsize + m.gsize);
                gmask = ((1 << m.gsize) - 1) << (m.asize + m.bsize);
                bmask = ((1 << m.bsize) - 1) << m.asize;
                amask = ((1 << m.asize) - 1) << 0;
            }

            if (bitcount <= 32) {
                return nv::findD3D9Format(bitcount, rmask, gmask, bmask, amask);
            }
            else {
                //if (m.rsize == 16 && m.gsize == 16 && m.bsize == 0 && m.asize == 0) return D3DFMT_G16R16;
                if (m.rsize == 16 && m.gsize == 16 && m.bsize == 16 && m.asize == 16) return D3DFMT_A16B16G16R16;
            }
        }
        else if (m.pixelType == PixelType_Float) {
            if (m.rsize == 16 && m.gsize == 0 && m.bsize == 0 && m.asize == 0) return D3DFMT_R16F;
            if (m.rsize == 32 && m.gsize == 0 && m.bsize == 0 && m.asize == 0) return D3DFMT_R32F;
            if (m.rsize == 16 && m.gsize == 16 && m.bsize == 0 && m.asize == 0) return D3DFMT_G16R16F;
            if (m.rsize == 32 && m.gsize == 32 && m.bsize == 0 && m.asize == 0) return D3DFMT_G32R32F;
            if (m.rsize == 16 && m.gsize == 16 && m.bsize == 16 && m.asize == 16) return D3DFMT_A16B16G16R16F;
            if (m.rsize == 32 && m.gsize == 32 && m.bsize == 32 && m.asize == 32) return D3DFMT_A32B32G32R32F;
        }

        return 0;
    }
    else {
        uint d3d9_formats[] = {
            0,              // Format_RGB,
            FOURCC_DXT1,    // Format_DXT1
            FOURCC_DXT1,    // Format_DXT1a
            FOURCC_DXT3,    // Format_DXT3
            FOURCC_DXT5,    // Format_DXT5
            FOURCC_DXT5,    // Format_DXT5n
            FOURCC_ATI1,    // Format_BC4
            FOURCC_ATI2,    // Format_BC5
            FOURCC_DXT1,    // Format_DXT1n
		    0,              // Format_CTX1
            MAKEFOURCC('B', 'C', '6', 'H'),     // Format_BC6
            MAKEFOURCC('B', 'C', '7', 'L'),     // Format_BC7
            //FOURCC_ATI2,    // Format_BC5_Luma
            FOURCC_DXT5,    // Format_BC3_RGBM
        };

        NV_COMPILER_CHECK(NV_ARRAY_SIZE(d3d9_formats) == Format_Count);

        return d3d9_formats[m.format];
    }
}

/*
bool CompressionOptions::setDirect3D9Format(unsigned int format)
{
}

unsigned int CompressionOptions::dxgiFormat() const
{
}

bool CompressionOptions::setDXGIFormat(unsigned int format)
{
}
*/
