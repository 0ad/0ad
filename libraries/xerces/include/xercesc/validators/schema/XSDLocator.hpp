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
 * $Id: XSDLocator.hpp,v 1.5 2003/05/16 03:15:51 knoaman Exp $
 */


/**
  * A Locator implementation
  */

#ifndef XSDLOCATOR_HPP
#define XSDLOCATOR_HPP

#include <xercesc/util/XMemory.hpp>
#include <xercesc/sax/Locator.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class VALIDATORS_EXPORT XSDLocator: public XMemory, public Locator
{
public:

    /** @name Constructors and Destructor */
    //@{
    /** Default constructor */
    XSDLocator();

    /** Destructor */
    virtual ~XSDLocator()
    {
    }

    //@}

    /** @name The locator interface */
    //@{
  /**
    * Return the public identifier for the current document event.
    * <p>This will be the public identifier
    * @return A string containing the public identifier, or
    *         null if none is available.
    * @see #getSystemId
    */
    virtual const XMLCh* getPublicId() const;

  /**
    * Return the system identifier for the current document event.
    *
    * <p>If the system identifier is a URL, the parser must resolve it
    * fully before passing it to the application.</p>
    *
    * @return A string containing the system identifier, or null
    *         if none is available.
    * @see #getPublicId
    */
    virtual const XMLCh* getSystemId() const;

  /**
    * Return the line number where the current document event ends.
    * Note that this is the line position of the first character
    * after the text associated with the document event.
    * @return The line number, or -1 if none is available.
    * @see #getColumnNumber
    */
    virtual XMLSSize_t getLineNumber() const;

  /**
    * Return the column number where the current document event ends.
    * Note that this is the column number of the first
    * character after the text associated with the document
    * event.  The first column in a line is position 1.
    * @return The column number, or -1 if none is available.
    * @see #getLineNumber
    */
    virtual XMLSSize_t getColumnNumber() const;
    //@}

    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    void setValues(const XMLCh* const systemId,
                   const XMLCh* const publicId,
                   const XMLSSize_t lineNo, const XMLSSize_t columnNo);

private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and destructor
    // -----------------------------------------------------------------------
    XSDLocator(const XSDLocator&);
    XSDLocator& operator=(const XSDLocator&);

    // -----------------------------------------------------------------------
    //  Private data members
    // -----------------------------------------------------------------------
    XMLSSize_t fLineNo;
    XMLSSize_t fColumnNo;
    const XMLCh* fSystemId;
    const XMLCh* fPublicId;
};

// ---------------------------------------------------------------------------
//  XSDLocator: Getter methods
// ---------------------------------------------------------------------------
inline XMLSSize_t XSDLocator::getLineNumber() const
{
    return fLineNo;
}

inline XMLSSize_t XSDLocator::getColumnNumber() const
{
    return fColumnNo;
}

inline const XMLCh* XSDLocator::getPublicId() const
{
    return fPublicId;
}

inline const XMLCh* XSDLocator::getSystemId() const
{
    return fSystemId;
}

XERCES_CPP_NAMESPACE_END

#endif
