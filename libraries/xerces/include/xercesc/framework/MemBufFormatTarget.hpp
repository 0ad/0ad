/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2002 The Apache Software Foundation.  All rights
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
 * $Id: MemBufFormatTarget.hpp,v 1.7 2003/05/16 21:36:55 knoaman Exp $
 * $Log: MemBufFormatTarget.hpp,v $
 * Revision 1.7  2003/05/16 21:36:55  knoaman
 * Memory manager implementation: Modify constructors to pass in the memory manager.
 *
 * Revision 1.6  2003/05/15 18:26:07  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.5  2002/11/04 15:00:21  tng
 * C++ Namespace Support.
 *
 * Revision 1.4  2002/08/12 21:38:22  peiyongz
 * Bug#11462: MemBufFormatTarget issue(2) .., proposed patch from
 *                      Esmond Pitt (pitte@anz.com)
 *
 * Revision 1.3  2002/07/22 23:23:15  tng
 * DOM L3: MemBufFormatTarget stores fDataBuf as XMLByte directly, consistent design as MemBufInputSource
 *
 * Revision 1.2  2002/06/05 15:47:13  peiyongz
 * data member changed, reset() added.
 *
 * Revision 1.1  2002/05/28 22:40:46  peiyongz
 * DOM3 Save Interface: DOMWriter/DOMWriterFilter
 *
 */

#ifndef MemBufFormatTarget_HEADER_GUARD_
#define MemBufFormatTarget_HEADER_GUARD_

#include <xercesc/framework/XMLFormatter.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/*
 * The MemBufFormatTarget is a derivative from XMLFormatTarget, which user code
 * may plug into DOMWriter to retrieve the serialized XML stream (from DOM Tree)
 * in a memory buffer.
 *
 * The MemBufFormatTarget is initalized to have a memory buffer of 1023 upon
 * construction, which grows as needed. The buffer will be deleted when
 * MemBufFormatTarget is destructed; or will be reset when the reset() function
 * is called.
 *
 * The MemBufFormatTarget returns a NULL terminated XMLByte stream upon request,
 * through the method getRawBuffer(), and user should make its own copy of the
 * returned buffer if it intends to keep it independent on the state of the
 * MemBufFormatTarget.
 */

class XMLPARSER_EXPORT MemBufFormatTarget : public XMLFormatTarget {
public:

    /** @name constructors and destructor */
    //@{
    MemBufFormatTarget
    (
          int                  initCapacity = 1023
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    ) ;
    ~MemBufFormatTarget();
    //@}

    // -----------------------------------------------------------------------
    //  Implementations of the format target interface
    // -----------------------------------------------------------------------
    virtual void writeChars(const XMLByte* const toWrite
                          , const unsigned int   count
                          , XMLFormatter* const  formatter);

    // -----------------------------------------------------------------------
    //  Getter
    // -----------------------------------------------------------------------
    /** @name getRawBuffer */
    //@{
    /**
     * Returned the internal raw buffer.
     *
     */
    //@}
    const XMLByte* getRawBuffer() const;

    /** @name getLen */
    //@{
    /**
     * Returned the length of the raw buffer.
     *
     */
    //@}
    unsigned int getLen() const
    {
        return fIndex;
    }

    /** @name reset */
    //@{
    /**
     * Reset the internal string buffer.
     *
     */
    void reset();
    //@}

private:
    // -----------------------------------------------------------------------
    //  Unimplemented methods.
    // -----------------------------------------------------------------------
    MemBufFormatTarget(const MemBufFormatTarget&);
    MemBufFormatTarget& operator=(const MemBufFormatTarget&);

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

