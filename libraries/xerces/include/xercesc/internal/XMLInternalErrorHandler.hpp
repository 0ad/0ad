/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2001 The Apache Software Foundation.  All rights
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
 * $Log: XMLInternalErrorHandler.hpp,v $
 * Revision 1.3  2004/01/29 11:46:30  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.2  2002/11/04 14:58:18  tng
 * C++ Namespace Support.
 *
 * Revision 1.1.1.1  2002/02/01 22:21:58  peiyongz
 * sane_include
 *
 * Revision 1.1  2001/07/26 17:04:10  tng
 * Schema: Process should stop after fatal error, and user throws need to be rethrown.
 *
 */

#if !defined(XMLINTERNALERRORHANDLER_HPP)
#define XMLINTERNALERRORHANDLER_HPP

#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/sax/ErrorHandler.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLInternalErrorHandler : public ErrorHandler
{
public:
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    XMLInternalErrorHandler(ErrorHandler* userHandler = 0) :
       fSawWarning(false),
       fSawError(false),
       fSawFatal(false),
       fUserErrorHandler(userHandler)
    {
    }

    ~XMLInternalErrorHandler()
    {
    }

    // -----------------------------------------------------------------------
    //  Implementation of the error handler interface
    // -----------------------------------------------------------------------
    void warning(const SAXParseException& toCatch);
    void error(const SAXParseException& toCatch);
    void fatalError(const SAXParseException& toCatch);
    void resetErrors();

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    bool getSawWarning() const;
    bool getSawError() const;
    bool getSawFatal() const;

    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fSawWarning
    //      This is set if we get any warning, and is queryable via a getter
    //      method.
    //
    //  fSawError
    //      This is set if we get any errors, and is queryable via a getter
    //      method.
    //
    //  fSawFatal
    //      This is set if we get any fatal, and is queryable via a getter
    //      method.
    //
    //  fUserErrorHandler
    //      This is the error handler from user
    // -----------------------------------------------------------------------
    bool    fSawWarning;
    bool    fSawError;
    bool    fSawFatal;
    ErrorHandler* fUserErrorHandler;

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XMLInternalErrorHandler(const XMLInternalErrorHandler&);
    XMLInternalErrorHandler& operator=(const XMLInternalErrorHandler&);
};

inline bool XMLInternalErrorHandler::getSawWarning() const
{
    return fSawWarning;
}

inline bool XMLInternalErrorHandler::getSawError() const
{
    return fSawError;
}

inline bool XMLInternalErrorHandler::getSawFatal() const
{
    return fSawFatal;
}

inline void XMLInternalErrorHandler::warning(const SAXParseException& toCatch)
{
    fSawWarning = true;
    if (fUserErrorHandler)
        fUserErrorHandler->warning(toCatch);
}

inline void XMLInternalErrorHandler::error(const SAXParseException& toCatch)
{
    fSawError = true;
    if (fUserErrorHandler)
        fUserErrorHandler->error(toCatch);
}

inline void XMLInternalErrorHandler::fatalError(const SAXParseException& toCatch)
{
    fSawFatal = true;
    if (fUserErrorHandler)
        fUserErrorHandler->fatalError(toCatch);
}

inline void XMLInternalErrorHandler::resetErrors()
{
    fSawWarning = false;
    fSawError = false;
    fSawFatal = false;
}

XERCES_CPP_NAMESPACE_END

#endif
