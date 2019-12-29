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

#include "OutputOptions.h"

using namespace nvtt;


OutputOptions::OutputOptions() : m(*new OutputOptions::Private())
{
    reset();
}

OutputOptions::~OutputOptions()
{
    // Cleanup output handler.
    setOutputHandler(NULL);

    delete &m;
}

/// Set default output options.
void OutputOptions::reset()
{
    m.fileName.reset();
    m.fileHandle = NULL;

    m.outputHandler = NULL;
    m.errorHandler = NULL;

    m.outputHeader = true;
    m.container = Container_DDS;
    m.version = 0;
    m.srgb = false;
    m.deleteOutputHandler = false;
}


/// Set output file name.
void OutputOptions::setFileName(const char * fileName)
{
    if (m.deleteOutputHandler)
    {
        delete m.outputHandler;
    }

    m.fileName = fileName;
    m.fileHandle = NULL;
    m.outputHandler = NULL;
    m.deleteOutputHandler = false;

    DefaultOutputHandler * oh = new DefaultOutputHandler(fileName);
    if (oh->stream.isError()) {
        delete oh;
    }
    else {
        m.deleteOutputHandler = true;
        m.outputHandler = oh;
    }
}

/// Set output file handle.
void OutputOptions::setFileHandle(void * fp)
{
    if (m.deleteOutputHandler) {
        delete m.outputHandler;
    }

    m.fileName.reset();
    m.fileHandle = (FILE *)fp;
    m.outputHandler = NULL;
    m.deleteOutputHandler = false;

    DefaultOutputHandler * oh = new DefaultOutputHandler(m.fileHandle);
    if (oh->stream.isError()) {
        delete oh;
    }
    else {
        m.deleteOutputHandler = true;
        m.outputHandler = oh;
    }
}


/// Set output handler.
void OutputOptions::setOutputHandler(OutputHandler * outputHandler)
{
    if (m.deleteOutputHandler) {
        delete m.outputHandler;
    }

    m.fileName.reset();
    m.fileHandle = NULL;
    m.outputHandler = outputHandler;
    m.deleteOutputHandler = false;
}

/// Set error handler.
void OutputOptions::setErrorHandler(ErrorHandler * errorHandler)
{
    m.errorHandler = errorHandler;
}

/// Set output header.
void OutputOptions::setOutputHeader(bool outputHeader)
{
    m.outputHeader = outputHeader;
}

/// Set container.
void OutputOptions::setContainer(Container container)
{
    m.container = container;
}

/// Set user version.
void OutputOptions::setUserVersion(int version)
{
    m.version = version;
}

/// Set SRGB flag.
void OutputOptions::setSrgbFlag(bool b)
{
    m.srgb = b;
}

bool OutputOptions::Private::hasValidOutputHandler() const
{
    if (!fileName.isNull() || fileHandle != NULL)
    {
        return outputHandler != NULL;
    }

    return true;
}

void OutputOptions::Private::beginImage(int size, int width, int height, int depth, int face, int miplevel) const
{
    if (outputHandler != NULL) outputHandler->beginImage(size, width, height, depth, face, miplevel);
}

bool OutputOptions::Private::writeData(const void * data, int size) const
{
    return outputHandler == NULL || outputHandler->writeData(data, size);
}

void OutputOptions::Private::endImage() const
{
    if (outputHandler != NULL) outputHandler->endImage();
}

void OutputOptions::Private::error(Error e) const
{
    if (errorHandler != NULL) errorHandler->error(e);
}
