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

#ifndef NV_TT_OUTPUTOPTIONS_H
#define NV_TT_OUTPUTOPTIONS_H

#include "nvtt.h"

#include "nvcore/StrLib.h" // Path
#include "nvcore/StdStream.h"


namespace nvtt
{

	struct DefaultOutputHandler : public nvtt::OutputHandler
	{
		DefaultOutputHandler(const char * fileName) : stream(fileName) {}
        DefaultOutputHandler(FILE * fp) : stream(fp, false) {}
		
		virtual ~DefaultOutputHandler() {}
		
		virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel)
		{
			// ignore.
		}
		
		// Output data.
		virtual bool writeData(const void * data, int size)
		{
			stream.serialize(const_cast<void *>(data), size);

			//return !stream.isError();
			return true;
		}

		virtual void endImage()
		{
			// ignore.
		}

		nv::StdOutputStream stream;
	};


	struct OutputOptions::Private
	{
		nv::Path fileName;
        FILE * fileHandle;
		
		OutputHandler * outputHandler;
		ErrorHandler * errorHandler;

		bool outputHeader;
		Container container;
        int version;
        bool srgb;
        bool deleteOutputHandler;

        void * wrapperProxy;    // For the C/C# wrapper.
		
		bool hasValidOutputHandler() const;

		void beginImage(int size, int width, int height, int depth, int face, int miplevel) const;
		bool writeData(const void * data, int size) const;
        void endImage() const;
		void error(Error e) const;
	};

	
} // nvtt namespace


#endif // NV_TT_OUTPUTOPTIONS_H
