#ifndef DOMImplementation_HEADER_GUARD_
#define DOMImplementation_HEADER_GUARD_

/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2001-2002 The Apache Software Foundation.  All rights
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
 * originally based on software copyright (c) 2001, International
 * Business Machines, Inc., http://www.ibm.com .  For more information
 * on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * $Id: DOMImplementation.hpp,v 1.13 2003/12/01 23:23:25 neilg Exp $
 */

#include <xercesc/dom/DOMImplementationLS.hpp>
#include <xercesc/dom/DOMException.hpp>
#include <xercesc/dom/DOMRangeException.hpp>
#include <xercesc/util/PlatformUtils.hpp>

XERCES_CPP_NAMESPACE_BEGIN


class DOMDocument;
class DOMDocumentType;

/**
 * The <code>DOMImplementation</code> interface provides a number of methods
 * for performing operations that are independent of any particular instance
 * of the document object model.
 */

class CDOM_EXPORT DOMImplementation : public DOMImplementationLS
{
protected:
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    /** @name Hidden constructors */
    //@{    
        DOMImplementation() {};                                      // no plain constructor
    //@}

private:
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    /** @name Unimplemented constructors and operators */
    //@{
        DOMImplementation(const DOMImplementation &);   // no copy construtor.
        DOMImplementation & operator = (const DOMImplementation &);  // No Assignment
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
    virtual ~DOMImplementation() {};
    //@}

    // -----------------------------------------------------------------------
    // Virtual DOMImplementation interface
    // -----------------------------------------------------------------------
    /** @name Functions introduced in DOM Level 1 */
    //@{
    /**
     * Test if the DOM implementation implements a specific feature.
     * @param feature The name of the feature to test (case-insensitive). The
     *   values used by DOM features are defined throughout the DOM Level 2
     *   specifications and listed in the  section. The name must be an XML
     *   name. To avoid possible conflicts, as a convention, names referring
     *   to features defined outside the DOM specification should be made
     *   unique.
     * @param version This is the version number of the feature to test. In
     *   Level 2, the string can be either "2.0" or "1.0". If the version is
     *   not specified, supporting any version of the feature causes the
     *   method to return <code>true</code>.
     * @return <code>true</code> if the feature is implemented in the
     *   specified version, <code>false</code> otherwise.
     * @since DOM Level 1
     */
    virtual bool  hasFeature(const XMLCh *feature,  const XMLCh *version) const = 0;
    //@}

    // -----------------------------------------------------------------------
    // Functions introduced in DOM Level 2
    // -----------------------------------------------------------------------
    /** @name Functions introduced in DOM Level 2 */
    //@{
    /**
     * Creates an empty <code>DOMDocumentType</code> node. Entity declarations
     * and notations are not made available. Entity reference expansions and
     * default attribute additions do not occur. It is expected that a
     * future version of the DOM will provide a way for populating a
     * <code>DOMDocumentType</code>.
     * @param qualifiedName The qualified name of the document type to be
     *   created.
     * @param publicId The external subset public identifier.
     * @param systemId The external subset system identifier.
     * @return A new <code>DOMDocumentType</code> node with
     *   <code>ownerDocument</code> set to <code>null</code>.
     * @exception DOMException
     *   INVALID_CHARACTER_ERR: Raised if the specified qualified name
     *   contains an illegal character.
     *   <br>NAMESPACE_ERR: Raised if the <code>qualifiedName</code> is
     *   malformed.
     *   <br>NOT_SUPPORTED_ERR: May be raised by DOM implementations which do
     *   not support the <code>"XML"</code> feature, if they choose not to
     *   support this method. Other features introduced in the future, by
     *   the DOM WG or in extensions defined by other groups, may also
     *   demand support for this method; please consult the definition of
     *   the feature to see if it requires this method.
     * @since DOM Level 2
     */
    virtual  DOMDocumentType *createDocumentType(const XMLCh *qualifiedName,
                                                 const XMLCh *publicId,
                                                 const XMLCh *systemId) = 0;

    /**
     * Creates a DOMDocument object of the specified type with its document
     * element.
     * @param namespaceURI The namespace URI of the document element to
     *   create.
     * @param qualifiedName The qualified name of the document element to be
     *   created.
     * @param doctype The type of document to be created or <code>null</code>.
     *   When <code>doctype</code> is not <code>null</code>, its
     *   <code>ownerDocument</code> attribute is set to the document
     *   being created.
     * @param manager    Pointer to the memory manager to be used to
     *                   allocate objects.
     * @return A new <code>DOMDocument</code> object.
     * @exception DOMException
     *   INVALID_CHARACTER_ERR: Raised if the specified qualified name
     *   contains an illegal character.
     *   <br>NAMESPACE_ERR: Raised if the <code>qualifiedName</code> is
     *   malformed, if the <code>qualifiedName</code> has a prefix and the
     *   <code>namespaceURI</code> is <code>null</code>, or if the
     *   <code>qualifiedName</code> has a prefix that is "xml" and the
     *   <code>namespaceURI</code> is different from "
     *   http://www.w3.org/XML/1998/namespace" , or if the DOM
     *   implementation does not support the <code>"XML"</code> feature but
     *   a non-null namespace URI was provided, since namespaces were
     *   defined by XML.
     *   <br>WRONG_DOCUMENT_ERR: Raised if <code>doctype</code> has already
     *   been used with a different document or was created from a different
     *   implementation.
     *   <br>NOT_SUPPORTED_ERR: May be raised by DOM implementations which do
     *   not support the "XML" feature, if they choose not to support this
     *   method. Other features introduced in the future, by the DOM WG or
     *   in extensions defined by other groups, may also demand support for
     *   this method; please consult the definition of the feature to see if
     *   it requires this method.
     * @since DOM Level 2
     */

    virtual DOMDocument *createDocument(const XMLCh *namespaceURI,
                                        const XMLCh *qualifiedName,
                                        DOMDocumentType *doctype,
                                        MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager) = 0;

    //@}
    // -----------------------------------------------------------------------
    // Functions introduced in DOM Level 3
    // -----------------------------------------------------------------------
    /** @name Functions introduced in DOM Level 3 */
    //@{
    /**
     * This method makes available a <code>DOMImplementation</code>'s
     * specialized interface (see ).
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @param feature The name of the feature requested (case-insensitive).
     * @return Returns an alternate <code>DOMImplementation</code> which
     *   implements the specialized APIs of the specified feature, if any,
     *   or <code>null</code> if there is no alternate
     *   <code>DOMImplementation</code> object which implements interfaces
     *   associated with that feature. Any alternate
     *   <code>DOMImplementation</code> returned by this method must
     *   delegate to the primary core <code>DOMImplementation</code> and not
     *   return results inconsistent with the primary
     *   <code>DOMImplementation</code>
     * @since DOM Level 3
     */
    virtual DOMImplementation* getInterface(const XMLCh* feature) = 0;

    //@}

    // -----------------------------------------------------------------------
    // Non-standard extension
    // -----------------------------------------------------------------------
    /** @name Non-standard extension */
    //@{
    /**
     * Non-standard extension
     *
     * Create a completely empty document that has neither a root element or a doctype node.
     */
    virtual DOMDocument *createDocument(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager) = 0;

    /**
     * Non-standard extension
     *
     *  Factory method for getting a DOMImplementation object.
     *     The DOM implementation retains ownership of the returned object.
     *     Application code should NOT delete it.
     */
    static DOMImplementation *getImplementation();

    /**
     * Non-standard extension
     *
     *  Load the default error text message for DOMException.
      * @param msgToLoad The DOM ExceptionCode id to be processed
      * @param toFill    The buffer that will hold the output on return. The
      *         size of this buffer should at least be 'maxChars + 1'.
      * @param maxChars  The maximum number of output characters that can be
      *         accepted. If the result will not fit, it is an error.
      * @return <code>true</code> if the message is successfully loaded
     */
    static bool loadDOMExceptionMsg
    (
        const   DOMException::ExceptionCode  msgToLoad
        ,       XMLCh* const                 toFill
        , const unsigned int                 maxChars
    );

    /**
     * Non-standard extension
     *
     *  Load the default error text message for DOMRangeException.
      * @param msgToLoad The DOM RangeExceptionCode id to be processed
      * @param toFill    The buffer that will hold the output on return. The
      *         size of this buffer should at least be 'maxChars + 1'.
      * @param maxChars  The maximum number of output characters that can be
      *         accepted. If the result will not fit, it is an error.
      * @return <code>true</code> if the message is successfully loaded
     */
    static bool loadDOMExceptionMsg
    (
        const   DOMRangeException::RangeExceptionCode  msgToLoad
        ,       XMLCh* const                           toFill
        , const unsigned int                           maxChars
    );
    //@}

};

XERCES_CPP_NAMESPACE_END

#endif
