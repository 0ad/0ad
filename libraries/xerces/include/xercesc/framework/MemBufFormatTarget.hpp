/*
 * Copyright 1999-2002,2004 The Apache Software Foundation.
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
 * $Id: MemBufFormatTarget.hpp 191054 2005-06-17 02:56:35Z jberry $
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

