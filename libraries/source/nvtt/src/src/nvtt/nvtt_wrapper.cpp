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

#include "nvtt.h"
#include "nvtt_wrapper.h"

#include "OutputOptions.h"

// An OutputHandler that sets and calls function pointers, rather than
// requiring interfaces to derive from OutputHandler itself
struct HandlerProxy : public nvtt::OutputHandler
{
public:

    HandlerProxy() {}

    nvttBeginImageHandler beginImageHandler;
    nvttOutputHandler writeDataHandler;
    nvttEndImageHandler endImageHandler;

    virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel)
    {
        if (beginImageHandler != NULL) 
        {
            beginImageHandler(size, width, height, depth, face, miplevel);
        }
    }


    virtual bool writeData(const void * data, int size)
    {
        if (writeDataHandler != NULL)
        {
            return writeDataHandler(data, size);
        }
        return false;
    }

    virtual void endImage()
    {
        if (endImageHandler != NULL) 
        {
            endImageHandler();
        }
    }
};


// InputOptions class.
NvttInputOptions * nvttCreateInputOptions()
{
    return new nvtt::InputOptions();
}

void nvttDestroyInputOptions(NvttInputOptions * inputOptions)
{
    delete inputOptions;
}

void nvttSetInputOptionsTextureLayout(NvttInputOptions * inputOptions, NvttTextureType type, int w, int h, int d)
{
    inputOptions->setTextureLayout((nvtt::TextureType)type, w, h, d);
}

void nvttResetInputOptionsTextureLayout(NvttInputOptions * inputOptions)
{
    inputOptions->resetTextureLayout();
}

NvttBoolean nvttSetInputOptionsMipmapData(NvttInputOptions * inputOptions, const void * data, int w, int h, int d, int face, int mipmap)
{
    return (NvttBoolean)inputOptions->setMipmapData(data, w, h, d, face, mipmap);
}

void nvttSetInputOptionsFormat(NvttInputOptions * inputOptions, NvttInputFormat format)
{
    inputOptions->setFormat((nvtt::InputFormat)format);
}

void nvttSetInputOptionsAlphaMode(NvttInputOptions * inputOptions, NvttAlphaMode alphaMode)
{
    inputOptions->setAlphaMode((nvtt::AlphaMode)alphaMode);
}

void nvttSetInputOptionsGamma(NvttInputOptions * inputOptions, float inputGamma, float outputGamma)
{
    inputOptions->setGamma(inputGamma, outputGamma);
}

void nvttSetInputOptionsWrapMode(NvttInputOptions * inputOptions, NvttWrapMode mode)
{
    inputOptions->setWrapMode((nvtt::WrapMode)mode);
}

void nvttSetInputOptionsMipmapFilter(NvttInputOptions * inputOptions, NvttMipmapFilter filter)
{
    inputOptions->setMipmapFilter((nvtt::MipmapFilter)filter);
}

void nvttSetInputOptionsMipmapGeneration(NvttInputOptions * inputOptions, NvttBoolean enabled, int maxLevel)
{
    inputOptions->setMipmapGeneration(enabled != NVTT_False, maxLevel);
}

void nvttSetInputOptionsKaiserParameters(NvttInputOptions * inputOptions, float width, float alpha, float stretch)
{
    inputOptions->setKaiserParameters(width, alpha, stretch);
}

void nvttSetInputOptionsNormalMap(NvttInputOptions * inputOptions, NvttBoolean b)
{
    inputOptions->setNormalMap(b != NVTT_False);
}

void nvttSetInputOptionsConvertToNormalMap(NvttInputOptions * inputOptions, NvttBoolean convert)
{
    inputOptions->setConvertToNormalMap(convert != NVTT_False);
}

void nvttSetInputOptionsHeightEvaluation(NvttInputOptions * inputOptions, float redScale, float greenScale, float blueScale, float alphaScale)
{
    inputOptions->setHeightEvaluation(redScale, greenScale, blueScale, alphaScale);
}

void nvttSetInputOptionsNormalFilter(NvttInputOptions * inputOptions, float small, float medium, float big, float large)
{
    inputOptions->setNormalFilter(small, medium, big, large);
}

void nvttSetInputOptionsNormalizeMipmaps(NvttInputOptions * inputOptions, NvttBoolean b)
{
    inputOptions->setNormalizeMipmaps(b != NVTT_False);
}

void nvttSetInputOptionsMaxExtents(NvttInputOptions * inputOptions, int dim)
{
    inputOptions->setMaxExtents(dim);
}

void nvttSetInputOptionsRoundMode(NvttInputOptions * inputOptions, NvttRoundMode mode)
{
    inputOptions->setRoundMode((nvtt::RoundMode)mode);
}


// CompressionOptions class.
NvttCompressionOptions * nvttCreateCompressionOptions()
{
    return new nvtt::CompressionOptions();
}

void nvttDestroyCompressionOptions(NvttCompressionOptions * compressionOptions)
{
    delete compressionOptions;
}

void nvttSetCompressionOptionsFormat(NvttCompressionOptions * compressionOptions, NvttFormat format)
{
    compressionOptions->setFormat((nvtt::Format)format);
}

void nvttSetCompressionOptionsQuality(NvttCompressionOptions * compressionOptions, NvttQuality quality)
{
    compressionOptions->setQuality((nvtt::Quality)quality);
}

void nvttSetCompressionOptionsColorWeights(NvttCompressionOptions * compressionOptions, float red, float green, float blue, float alpha)
{
    compressionOptions->setColorWeights(red, green, blue, alpha);
}

/*void nvttEnableCompressionOptionsCudaCompression(NvttCompressionOptions * compressionOptions, NvttBoolean enable)
{
compressionOptions->enableCudaCompression(enable != NVTT_False);
}*/

void nvttSetCompressionOptionsPixelFormat(NvttCompressionOptions * compressionOptions, unsigned int bitcount, unsigned int rmask, unsigned int gmask, unsigned int bmask, unsigned int amask)
{
    compressionOptions->setPixelFormat(bitcount, rmask, gmask, bmask, amask);
}

void nvttSetCompressionOptionsQuantization(NvttCompressionOptions * compressionOptions, NvttBoolean colorDithering, NvttBoolean alphaDithering, NvttBoolean binaryAlpha, int alphaThreshold)
{
    compressionOptions->setQuantization(colorDithering != NVTT_False, alphaDithering != NVTT_False, binaryAlpha != NVTT_False, alphaThreshold);
}


// OutputOptions class.
NvttOutputOptions * nvttCreateOutputOptions()
{
    nvtt::OutputOptions * outputOptions = new nvtt::OutputOptions();
    HandlerProxy * handlerProxy = new HandlerProxy();

    outputOptions->m.wrapperProxy = handlerProxy;

    return outputOptions;
}

void nvttDestroyOutputOptions(NvttOutputOptions * outputOptions)
{
	HandlerProxy * handlerProxy = (HandlerProxy *)outputOptions->m.wrapperProxy;
	delete handlerProxy;
    delete outputOptions;
}

void nvttSetOutputOptionsFileName(NvttOutputOptions * outputOptions, const char * fileName)
{
    outputOptions->setFileName(fileName);
}

void nvttSetOutputOptionsOutputHeader(NvttOutputOptions * outputOptions, NvttBoolean b)
{
    outputOptions->setOutputHeader(b != NVTT_False);
}
/*
void nvttSetOutputOptionsErrorHandler(NvttOutputOptions * outputOptions, nvttErrorHandler errorHandler)
{
    outputOptions->setErrorHandler(errorHandler);
}
*/

void nvttSetOutputOptionsOutputHandler(NvttOutputOptions * outputOptions, nvttBeginImageHandler beginImageHandler, nvttOutputHandler writeDataHandler, nvttEndImageHandler endImageHandler)
{
	HandlerProxy * handler = (HandlerProxy *)outputOptions->m.wrapperProxy;

	handler->beginImageHandler = beginImageHandler;
	handler->writeDataHandler = writeDataHandler;
    handler->endImageHandler = endImageHandler;

    if(beginImageHandler == NULL && writeDataHandler == NULL && endImageHandler == NULL)
    {
		outputOptions->setOutputHandler(NULL);
    }
    else
    {
		outputOptions->setOutputHandler(handler);
    }
}


// Compressor class.
NvttCompressor * nvttCreateCompressor()
{
    return new nvtt::Compressor();
}

void nvttDestroyCompressor(NvttCompressor * compressor)
{
    delete compressor;
}

NvttBoolean nvttCompress(const NvttCompressor * compressor, const NvttInputOptions * inputOptions, const NvttCompressionOptions * compressionOptions, const NvttOutputOptions * outputOptions)
{
    return (NvttBoolean)compressor->process(*inputOptions, *compressionOptions, *outputOptions);
}

int nvttEstimateSize(const NvttCompressor * compressor, const NvttInputOptions * inputOptions, const NvttCompressionOptions * compressionOptions)
{
    return compressor->estimateSize(*inputOptions, *compressionOptions);
}


// Global functions.
const char * nvttErrorString(NvttError e)
{
    return nvtt::errorString((nvtt::Error)e);
}

unsigned int nvttVersion()
{
    return nvtt::version();
}
