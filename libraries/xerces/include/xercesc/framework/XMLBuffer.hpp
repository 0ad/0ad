/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2003 The Apache Software Foundation.  All rights
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
 * $Log: XMLBuffer.hpp,v $
 * Revision 1.6  2004/01/29 11:46:29  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.5  2003/05/16 21:36:55  knoaman
 * Memory manager implementation: Modify constructors to pass in the memory manager.
 *
 * Revision 1.4  2003/05/15 18:26:07  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.3  2002/11/04 15:00:21  tng
 * C++ Namespace Support.
 *
 * Revision 1.2  2002/08/21 18:54:52  tng
 * [Bug 11869] Add the const modifier (XMLBuffer.hpp).
 *
 * Revision 1.1.1.1  2002/02/01 22:21:51  peiyongz
 * sane_include
 *
 * Revision 1.6  2001/06/27 20:29:09  tng
 * [Bug 2365] Huge performance problem with the parser in XMLScanner::sendCharData() .  By David Bertoni.
 *
 * Revision 1.5  2000/03/02 19:54:24  roddey
 * This checkin includes many changes done while waiting for the
 * 1.1.0 code to be finished. I can't list them all here, but a list is
 * available elsewhere.
 *
 * Revision 1.4  2000/02/24 20:00:22  abagchi
 * Swat for removing Log from API docs
 *
 * Revision 1.3  2000/02/15 01:21:30  roddey
 * Some initial documentation improvements. More to come...
 *
 * Revision 1.2  2000/02/06 07:47:47  rahulj
 * Year 2K copyright swat.
 *
 * Revision 1.1.1.1  1999/11/09 01:08:29  twl
 * Initial checkin
 *
 * Revision 1.2  1999/11/08 20:44:36  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */


#if !defined(XMLBUFFER_HPP)
#define XMLBUFFER_HPP

#include <xercesc/util/XMemory.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/MemoryManager.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
 *  XMLBuffer is a lightweight, expandable Unicode text buffer. Since XML is
 *  inherently theoretically unbounded in terms of the sizes of things, we
 *  very often need to have expandable buffers. The primary concern here is
 *  that appends of characters and other buffers or strings be very fast, so
 *  it always maintains the current buffer size.
 *
 *  The buffer is not nul terminated until some asks to see the raw buffer
 *  contents. This also avoids overhead during append operations.
 */
 class XMLPARSER_EXPORT XMLBuffer : public XMemory
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------

    /** @name Constructor */
    //@{
    XMLBuffer(int capacity = 1023
              , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager) :

        fUsed(false)
        , fIndex(0)
        , fCapacity(capacity)
        , fMemoryManager(manager)
        , fBuffer(0)
    {
        // Buffer is one larger than capacity, to allow for zero term
        fBuffer = (XMLCh*) manager->allocate((capacity+1) * sizeof(XMLCh)); //new XMLCh[fCapacity+1];

        // Keep it null terminated
        fBuffer[0] = XMLCh(0);
    }
    //@}

    /** @name Destructor */
    //@{
    ~XMLBuffer()
    {
        fMemoryManager->deallocate(fBuffer); //delete [] fBuffer;
    }
    //@}

    // -----------------------------------------------------------------------
    //  Buffer Management
    // -----------------------------------------------------------------------
    void append(const XMLCh toAppend)
    {
        if (fIndex == fCapacity)
            expand();

        // Put in char and bump the index
        fBuffer[fIndex++] = toAppend;
    }

    void append
    (
        const   XMLCh* const    chars
        , const unsigned int    count = 0
    );

    const XMLCh* getRawBuffer() const
    {
        fBuffer[fIndex] = 0;
        return fBuffer;
    }

    XMLCh* getRawBuffer()
    {
        fBuffer[fIndex] = 0;
        return fBuffer;
    }

    void reset()
    {
        fIndex = 0;
        fBuffer[0] = 0;
    }

    void set
    (
        const   XMLCh* const    chars
        , const unsigned int    count = 0
    );


    // -----------------------------------------------------------------------
    //  Getters
    // -----------------------------------------------------------------------
    bool getInUse() const
    {
        return fUsed;
    }

    unsigned int getLen() const
    {
        return fIndex;
    }

    bool isEmpty() const
    {
        return (fIndex == 0);
    }


    // -----------------------------------------------------------------------
    //  Setters
    // -----------------------------------------------------------------------
    void setInUse(const bool newValue)
    {
        fUsed = newValue;
    }


private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XMLBuffer(const XMLBuffer&);
    XMLBuffer& operator=(const XMLBuffer&);

    // -----------------------------------------------------------------------
    //  Declare our friends
    // -----------------------------------------------------------------------
    friend class XMLBufBid;


    // -----------------------------------------------------------------------
    //  Private helpers
    // -----------------------------------------------------------------------
    void expand();
    void insureCapacity(const unsigned int extraNeeded);


    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fBuffer
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
    //  fUsed
    //      Indicates whether this buffer is in use or not.
    // -----------------------------------------------------------------------
    bool            fUsed;
    unsigned int    fIndex;
    unsigned int    fCapacity;
    MemoryManager*  fMemoryManager;
    XMLCh*          fBuffer;
};

XERCES_CPP_NAMESPACE_END

#endif
