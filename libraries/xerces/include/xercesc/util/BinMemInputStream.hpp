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
 *
 * Revision 1.5  2004/09/08 13:56:21  peiyongz
 * Apache License Version 2.0
 *
 * Revision 1.4  2004/01/29 11:48:46  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.3  2003/05/16 03:11:22  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.2  2002/11/04 15:22:03  tng
 * C++ Namespace Support.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:10  peiyongz
 * sane_include
 *
 * Revision 1.3  2000/02/24 20:05:24  abagchi
 * Swat for removing Log from API docs
 *
 * Revision 1.2  2000/02/06 07:48:01  rahulj
 * Year 2K copyright swat.
 *
 * Revision 1.1.1.1  1999/11/09 01:04:07  twl
 * Initial checkin
 *
 * Revision 1.3  1999/11/08 20:45:04  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */

#if !defined(BINMEMINPUTSTREAM_HPP)
#define BINMEMINPUTSTREAM_HPP

#include <xercesc/util/BinInputStream.hpp>
#include <xercesc/util/PlatformUtils.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLUTIL_EXPORT BinMemInputStream : public BinInputStream
{
public :
    // -----------------------------------------------------------------------
    //  Class specific types
    // -----------------------------------------------------------------------
    enum BufOpts
    {
        BufOpt_Adopt
        , BufOpt_Copy
        , BufOpt_Reference
    };


    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    BinMemInputStream
    (
        const   XMLByte* const  initData
        , const unsigned int    capacity
        , const BufOpts         bufOpt = BufOpt_Copy
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    );
    virtual ~BinMemInputStream();


    // -----------------------------------------------------------------------
    //  Stream management methods
    // -----------------------------------------------------------------------
    void reset();


    // -----------------------------------------------------------------------
    //  Implementation of the input stream interface
    // -----------------------------------------------------------------------
    virtual unsigned int curPos() const;

    virtual unsigned int readBytes
    (
                XMLByte* const  toFill
        , const unsigned int    maxToRead
    );

    inline unsigned int getSize() const;
    
private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    BinMemInputStream(const BinMemInputStream&);
    BinMemInputStream& operator=(const BinMemInputStream&); 
    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fBuffer
    //      The buffer of bytes that we are streaming.
    //
    //  fBufOpt
    //      Indicates the ownership status of the buffer. The caller can have
    //      us adopt it (we delete it), reference it, or just make our own
    //      copy of it.
    //
    //  fCapacity
    //      The size of the buffer being streamed.
    //
    //  fCurIndex
    //      The current index where the next byte will be read from. When it
    //      hits fCapacity, we are done.
    // -----------------------------------------------------------------------
    const XMLByte*  fBuffer;
    BufOpts         fBufOpt;
    unsigned int    fCapacity;
    unsigned int    fCurIndex;
    MemoryManager*  fMemoryManager;
};


// ---------------------------------------------------------------------------
//  BinMemInputStream: Stream management methods
// ---------------------------------------------------------------------------
inline void BinMemInputStream::reset()
{
    fCurIndex = 0;
}


// ---------------------------------------------------------------------------
//  BinMemInputStream: Implementation of the input stream interface
// ---------------------------------------------------------------------------
inline unsigned int BinMemInputStream::curPos() const
{
    return fCurIndex;
}

inline unsigned int BinMemInputStream::getSize() const
{
    return fCapacity;
}

XERCES_CPP_NAMESPACE_END

#endif
