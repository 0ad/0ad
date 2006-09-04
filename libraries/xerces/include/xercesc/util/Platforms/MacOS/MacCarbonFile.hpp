/*
 * Copyright 1999-2000,2004 The Apache Software Foundation.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * $Id: MacCarbonFile.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#pragma once

#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/util/Platforms/MacOS/MacOSPlatformUtils.hpp>

XERCES_CPP_NAMESPACE_BEGIN

//	Concrete file class implemented using raw Carbon file system calls.
class XMLMacCarbonFile : public XMLMacAbstractFile
{
    public:
        XMLMacCarbonFile() : mFileRefNum(0), mFileValid(false) {}
        virtual ~XMLMacCarbonFile();

        unsigned int currPos();
        void close();
        unsigned int size();
        bool open(const XMLCh* path, bool toWrite);
        bool open(const char* path, bool toWrite);
        unsigned int read(unsigned int byteCount, XMLByte* buffer);
        void write(long byteCount, const XMLByte* buffer);
        void reset();

    protected:
        void create(const XMLCh* const);
        bool openWithPermission(const XMLCh* const, int macPermission);

        short	mFileRefNum;
        bool	mFileValid;
};

XERCES_CPP_NAMESPACE_END
