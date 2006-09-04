/*
 * Copyright 2003,2004 The Apache Software Foundation.
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
 * $Id: BinMemOutputStream.hpp 191054 2005-06-17 02:56:35Z jberry $
 */

#ifndef BINMEMOUTPUTSTREAM_HEADER_GUARD_
#define BINMEMOUTPUTSTREAM_HEADER_GUARD_

#include <xercesc/framework/BinOutputStream.hpp>
#include <xercesc/util/PlatformUtils.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLUTIL_EXPORT BinMemOutputStream : public BinOutputStream 
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------

    ~BinMemOutputStream();

    BinMemOutputStream
    (
         int                     initCapacity = 1023
       , MemoryManager* const    manager      = XMLPlatformUtils::fgMemoryManager
    );

    // -----------------------------------------------------------------------
    //  Implementation of the output stream interface
    // -----------------------------------------------------------------------
    virtual unsigned int curPos() const;

    virtual void writeBytes
    (
      const XMLByte*     const      toGo
    , const unsigned int            maxToWrite
    ) ;

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    const XMLByte* getRawBuffer() const;

    unsigned int getSize() const;
    void reset();

private :
    // -----------------------------------------------------------------------
    //  Unimplemented methods.
    // -----------------------------------------------------------------------
    BinMemOutputStream(const BinMemOutputStream&);
    BinMemOutputStream& operator=(const BinMemOutputStream&);

    // -----------------------------------------------------------------------
    //  Private helpers
    // -----------------------------------------------------------------------
    void insureCapacity(const unsigned int extraNeeded);

    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fDataBuf
    //      The pointer to the buffer data. Its grown as needed. Its always
    //      one larger than fCapacity, to leave room for the null terminator.
    //
    //  fIndex
    //      The current index into the buffer, as characters are appended
    //      to it. If its zero, then the buffer is empty.
    //
    //  fCapacity
    //      The current capacity of the buffer. Its actually always one
    //      larger, to leave room for the null terminator.
    //
    // -----------------------------------------------------------------------
    MemoryManager*  fMemoryManager;
    XMLByte*        fDataBuf;
    unsigned int    fIndex;
    unsigned int    fCapacity;

};


XERCES_CPP_NAMESPACE_END

#endif

