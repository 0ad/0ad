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
 * $Id: OutOfMemoryException.hpp,v 1.2 2004/01/29 11:48:46 cargilld Exp $
 */

#if !defined(OUT_OF_MEMORY_EXCEPTION_HPP)
#define OUT_OF_MEMORY_EXCEPTION_HPP

#include <xercesc/util/XMemory.hpp>
#include <xercesc/util/XMLExceptMsgs.hpp>
#include <xercesc/util/XMLUniDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN

const XMLCh gDefOutOfMemoryErrMsg[] =
{
        chLatin_O, chLatin_u, chLatin_t, chLatin_O
    ,   chLatin_f, chLatin_M, chLatin_e, chLatin_m
    ,   chLatin_o, chLatin_r, chLatin_y, chNull
};

class XMLUTIL_EXPORT OutOfMemoryException : public XMemory
{
public:
  
    OutOfMemoryException();
    ~OutOfMemoryException();
    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    XMLExcepts::Codes getCode() const;
    const XMLCh* getMessage() const;
    const XMLCh* getType() const;
    const char* getSrcFile() const;
    unsigned int getSrcLine() const;

    OutOfMemoryException(const OutOfMemoryException& toCopy);
    OutOfMemoryException& operator=(const OutOfMemoryException& toAssign);
};

// constructors/destructors...
inline OutOfMemoryException::OutOfMemoryException() {}
inline OutOfMemoryException::~OutOfMemoryException() {}
inline OutOfMemoryException::OutOfMemoryException(const OutOfMemoryException&) {}
inline OutOfMemoryException& OutOfMemoryException::operator=(const OutOfMemoryException&) 
{
    return *this;
}

// ---------------------------------------------------------------------------
//  OutOfMemoryException: Getter methods
// ---------------------------------------------------------------------------
inline XMLExcepts::Codes OutOfMemoryException::getCode() const
{
    return XMLExcepts::Out_Of_Memory;
}

inline const XMLCh* OutOfMemoryException::getMessage() const
{
    return gDefOutOfMemoryErrMsg;
}

inline const XMLCh* OutOfMemoryException::getType() const
{
    return gDefOutOfMemoryErrMsg;
}

inline const char* OutOfMemoryException::getSrcFile() const
{
    return "";
}
    
inline unsigned int OutOfMemoryException::getSrcLine() const {
    return 0;
}

XERCES_CPP_NAMESPACE_END

#endif
