/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2002 The Apache Software Foundation.  All rights
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
  * $Id: XSDErrorReporter.hpp,v 1.7 2003/12/24 15:24:16 cargilld Exp $
  */


#if !defined(XSDERRORREPORTER_HPP)
#define XSDERRORREPORTER_HPP

#include <xercesc/util/XMemory.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class Locator;
class XMLErrorReporter;


/**
 *  This class reports schema errors
 */
class VALIDATORS_EXPORT XSDErrorReporter : public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constructors are hidden, only the virtual destructor is exposed
    // -----------------------------------------------------------------------
    XSDErrorReporter(XMLErrorReporter* const errorReporter = 0);

    virtual ~XSDErrorReporter()
    {
    }

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    bool getExitOnFirstFatal() const;

    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    void setErrorReporter(XMLErrorReporter* const errorReporter);
    void setExitOnFirstFatal(const bool newValue);

    // -----------------------------------------------------------------------
    //  Report error methods
    // -----------------------------------------------------------------------
    void emitError(const unsigned int toEmit,
                   const XMLCh* const msgDomain,
                   const Locator* const aLocator);
    void emitError(const unsigned int toEmit,
                   const XMLCh* const msgDomain,
                   const Locator* const aLocator,
                   const XMLCh* const text1,
                   const XMLCh* const text2 = 0,
                   const XMLCh* const text3 = 0,
                   const XMLCh* const text4 = 0,
                   MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
                   );

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and destructor
    // -----------------------------------------------------------------------
    XSDErrorReporter(const XSDErrorReporter&);
    XSDErrorReporter& operator=(const XSDErrorReporter&);

    // -----------------------------------------------------------------------
    //  Private data members
    // -----------------------------------------------------------------------
    bool              fExitOnFirstFatal;
    XMLErrorReporter* fErrorReporter;
};


// ---------------------------------------------------------------------------
//  XSDErrorReporter: Getter methods
// ---------------------------------------------------------------------------
inline bool XSDErrorReporter::getExitOnFirstFatal() const
{
    return fExitOnFirstFatal;
}

// ---------------------------------------------------------------------------
//  XSDErrorReporter: Setter methods
// ---------------------------------------------------------------------------
inline void XSDErrorReporter::setExitOnFirstFatal(const bool newValue)
{
    fExitOnFirstFatal = newValue;
}

inline void XSDErrorReporter::setErrorReporter(XMLErrorReporter* const errorReporter)
{
    fErrorReporter = errorReporter;
}

XERCES_CPP_NAMESPACE_END

#endif
