/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2000 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Xerces" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache\@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation, and was
 * originally based on software copyright (c) 1999, International
 * Business Machines, Inc., http://www.ibm.com .  For more information
 * on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * $Log: BinMemInputStream.hpp,v $
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

XERCES_CPP_NAMESPACE_END

#endif
