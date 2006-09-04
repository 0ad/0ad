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
 * $Id: DOM_DOMImplementation.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#ifndef DOM_DOMImplementation_HEADER_GUARD_
#define DOM_DOMImplementation_HEADER_GUARD_

#include <xercesc/util/PlatformUtils.hpp>
#include "DOMString.hpp"

XERCES_CPP_NAMESPACE_BEGIN


class DOM_Document;
class DOM_DocumentType;

/**
 *   This class provides a way to query the capabilities of an implementation
 *   of the DOM
 */


class DEPRECATED_DOM_EXPORT DOM_DOMImplementation {
 private:
    DOM_DOMImplementation(const DOM_DOMImplementation &other);

 public:
/** @name Constructors and assignment operators */
//@{
 /**
   * Construct a DOM_Implementation reference variable, which should
   * be assigned to the return value from
   * <code>DOM_Implementation::getImplementation()</code>.
   */
    DOM_DOMImplementation();

 /**
   * Assignment operator
   *
   */
    DOM_DOMImplementation & operator = (const DOM_DOMImplementation &other);
//@}

  /** @name Destructor */
  //@{
  /**
    * Destructor.  The object being destroyed is a reference to the DOMImplemenentation,
    * not the underlying DOMImplementation object itself, which is owned by
    * the implementation code.
    *
    */

    ~DOM_DOMImplementation();
	//@}

  /** @name Getter functions */
  //@{

 /**
   * Test if the DOM implementation implements a specific feature.
   *
   * @param feature The string of the feature to test (case-insensitive). The legal
   *        values are defined throughout this specification. The string must be
   *        an <EM>XML name</EM> (see also Compliance).
   * @param version This is the version number of the package name to test.
   *   In Level 1, this is the string "1.0". If the version is not specified,
   *   supporting any version of the  feature will cause the method to return
   *   <code>true</code>.
   * @return <code>true</code> if the feature is implemented in the specified
   *   version, <code>false</code> otherwise.
   */
 bool  hasFeature(const DOMString &feature,  const DOMString &version);


  /** Return a reference to a DOM_Implementation object for this
    *  DOM implementation.
    *
    * Intended to support applications that may be
    * using DOMs retrieved from several different sources, potentially
    * with different underlying implementations.
    */
 static DOM_DOMImplementation &getImplementation();

 //@}

    /** @name Functions introduced in DOM Level 2. */
    //@{
    /**
     * Creates an empty <code>DOM_DocumentType</code> node.
     * Entity declarations and notations are not made available.
     * Entity reference expansions and default attribute additions
     * do not occur. It is expected that a future version of the DOM
     * will provide a way for populating a <code>DOM_DocumentType</code>.
     *
     * @param qualifiedName The <em>qualified name</em>
     * of the document type to be created.
     * @param publicId The external subset public identifier.
     * @param systemId The external subset system identifier.
     * @return A new <code>DOM_DocumentType</code> node with
     * <code>Node.ownerDocument</code> set to <code>null</code>.
     * @exception DOMException
     *   INVALID_CHARACTER_ERR: Raised if the specified qualified name
     *      contains an illegal character.
     * <br>
     *   NAMESPACE_ERR: Raised if the <code>qualifiedName</code> is malformed.
     */
    DOM_DocumentType createDocumentType(const DOMString &qualifiedName,
	const DOMString &publicId, const DOMString &systemId);

    /**
     * Creates an XML <code>DOM_Document</code> object of the specified type
     * with its document element.
     *
     * @param namespaceURI The <em>namespace URI</em> of
     * the document element to create, or <code>null</code>.
     * @param qualifiedName The <em>qualified name</em>
     * of the document element to be created.
     * @param doctype The type of document to be created or <code>null</code>.
     * <p>When <code>doctype</code> is not <code>null</code>, its
     * <code>Node.ownerDocument</code> attribute is set to the document
     * being created.
     * @return A new <code>DOM_Document</code> object.
     * @exception DOMException
     *   INVALID_CHARACTER_ERR: Raised if the specified qualified name
     *      contains an illegal character.
     * <br>
     *   NAMESPACE_ERR: Raised if the <CODE>qualifiedName</CODE> is
     *      malformed, or if the <CODE>qualifiedName</CODE> has a prefix that is
     *      "xml" and the <CODE>namespaceURI</CODE> is different from
     *      "http://www.w3.org/XML/1998/namespace".
     * <br>
     *   WRONG_DOCUMENT_ERR: Raised if <code>doctype</code> has already
     *   been used with a different document.
     */
    DOM_Document createDocument(const DOMString &namespaceURI,
	const DOMString &qualifiedName, const DOM_DocumentType &doctype,
	MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    //@}

    // -----------------------------------------------------------------------
    //  Notification that lazy data has been deleted
    // -----------------------------------------------------------------------
	static void reinitDOM_DOMImplementation();
};

XERCES_CPP_NAMESPACE_END

#endif
