#ifndef DOMLocator_HEADER_GUARD_
#define DOMLocator_HEADER_GUARD_

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
 * $Log: DOMLocator.hpp,v $
 * Revision 1.7  2003/03/07 19:59:05  tng
 * [Bug 11692] Unimplement the hidden constructors and assignment operator to remove warnings from gcc.
 *
 * Revision 1.6  2002/11/04 15:09:24  tng
 * C++ Namespace Support.
 *
 * Revision 1.5  2002/08/22 15:04:57  tng
 * Remove unused parameter variables in inline functions.
 *
 * Revision 1.4  2002/06/06 20:53:06  tng
 * Documentation Fix: Update the API Documentation for DOM headers
 *
 * Revision 1.3  2002/05/30 19:24:48  knoaman
 * documentation update
 *
 * Revision 1.2  2002/05/27 18:28:26  tng
 * To get ready for 64 bit large file, use XMLSSize_t to represent line and column number.
 *
 * Revision 1.1  2002/05/23 15:47:24  knoaman
 * DOM L3 core - support for DOMError, DOMErrorHandler and DOMLocator
 *
 */

#include <xercesc/util/XercesDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN


class DOMNode;


/**
  * DOMLocator is an interface that describes a location. (e.g. where an error
  * occured).
  *
  * @see DOMError#DOMError
  * @since DOM Level 3
  */

class CDOM_EXPORT DOMLocator
{
protected:
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    /** @name Hidden constructors */
    //@{    
    DOMLocator() {};
    //@}
    
private:    
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    /** @name Unimplemented constructors and operators */
    //@{
    DOMLocator(const DOMLocator &);
    DOMLocator & operator = (const DOMLocator &);
    //@}

public:
    // -----------------------------------------------------------------------
    //  All constructors are hidden, just the destructor is available
    // -----------------------------------------------------------------------
    /** @name Destructor */
    //@{
    /**
     * Destructor
     *
     */
    virtual ~DOMLocator() {};
    //@}

    // -----------------------------------------------------------------------
    //  Virtual DOMLocator interface
    // -----------------------------------------------------------------------
    /** @name Functions introduced in DOM Level 3 */
    //@{
    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    /**
     * Get the line number where the error occured. The value is -1 if there is
     * no line number available.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @see #setLineNumber
     * @since DOM Level 3
     */
    virtual XMLSSize_t getLineNumber() const = 0;

    /**
     * Get the column number where the error occured. The value is -1 if there
     * is no column number available.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @see #setColumnNumber
     * @since DOM Level 3
     */
    virtual XMLSSize_t getColumnNumber() const = 0;

    /**
     * Get the byte or character offset into the input source, if we're parsing
     * a file or a byte stream then this will be the byte offset into that
     * stream, but if a character media is parsed then the offset will be the
     * character offset. The value is -1 if there is no offset available.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @see #setOffset
     * @since DOM Level 3
     */
    virtual XMLSSize_t getOffset() const = 0;

    /**
     * Get the DOMNode where the error occured, or <code>null</code> if there
     * is no node available.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @see #setErrorNode
     * @since DOM Level 3
     */
    virtual DOMNode* getErrorNode() const = 0;

    /**
     * Get the URI where the error occured, or <code>null</code> if there is no
     * URI available.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @see #setURI
     * @since DOM Level 3
     */
    virtual const XMLCh* getURI() const = 0;

    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    /**
     * Set the line number of the error
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @param lineNumber the line number to set
     *
     * @see #getLinNumner
     * @since DOM Level 3
     */
    virtual void setLineNumber(const XMLSSize_t lineNumber) = 0;

    /**
     * Set the column number of the error
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @param columnNumber the column number to set.
     *
     * @see #getColumnNumner
     * @since DOM Level 3
     */
    virtual void setColumnNumber(const XMLSSize_t columnNumber) = 0;

    /**
     * Set the byte/character offset.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @param offset the byte/characte offset to set.
     *
     * @see #getOffset
     * @since DOM Level 3
     */
    virtual void setOffset(const XMLSSize_t offset) = 0;

    /**
     * Set the DOMNode where the error occured
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @param errorNode the DOMNode to set
     *
     * @see #getErrorNode
     * @since DOM Level 3
     */
    virtual void setErrorNode(DOMNode* const errorNode) = 0;

    /**
     * Set the URI where the error occured
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @param uri the URI to set.
     *
     * @see #getURI
     * @since DOM Level 3
     */
    virtual void setURI(const XMLCh* const uri) = 0;

    //@}
};

XERCES_CPP_NAMESPACE_END

#endif
