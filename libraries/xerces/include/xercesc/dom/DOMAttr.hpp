#ifndef DOMAttr_HEADER_GUARD_
#define DOMAttr_HEADER_GUARD_

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
 * $Id: DOMAttr.hpp,v 1.8 2003/03/07 19:58:58 tng Exp $
 */


#include <xercesc/util/XercesDefs.hpp>
#include "DOMNode.hpp"

XERCES_CPP_NAMESPACE_BEGIN

class DOMElement;
class DOMTypeInfo;

/**
 * The <code>DOMAttr</code> class refers to an attribute of an XML element.
 *
 * Typically the allowable values for the
 * attribute are defined in a documenttype definition.
 * <p><code>DOMAttr</code> objects inherit the <code>DOMNode</code>  interface, but
 * since attributes are not actually child nodes of the elements they are associated with, the
 * DOM does not consider them part of the document  tree.  Thus, the
 * <code>DOMNode</code> attributes <code>parentNode</code>,
 * <code>previousSibling</code>, and <code>nextSibling</code> have a  null
 * value for <code>DOMAttr</code> objects. The DOM takes the  view that
 * attributes are properties of elements rather than having a  separate
 * identity from the elements they are associated with;  this should make it
 * more efficient to implement such features as default attributes associated
 * with all elements of a  given type.  Furthermore, attribute nodes
 * may not be immediate children of a <code>DOMDocumentFragment</code>. However,
 * they can be associated with <code>DOMElement</code> nodes contained within a
 * <code>DOMDocumentFragment</code>. In short, users of the DOM
 * need to be aware that  <code>DOMAttr</code> nodes have some things in  common
 * with other objects inheriting the <code>DOMNode</code> interface, but they
 * also are quite distinct.
 *
 * @since DOM Level 1
 */
class CDOM_EXPORT DOMAttr: public DOMNode {
protected:
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    /** @name Hidden constructors */
    //@{    
    DOMAttr() {};
    //@}

private:    
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    /** @name Unimplemented constructors and operators */
    //@{
    DOMAttr(const DOMAttr &);
    DOMAttr & operator = (const DOMAttr &);
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
    virtual ~DOMAttr() {};
    //@}

    // -----------------------------------------------------------------------
    //  Virtual DOMAttr interface
    // -----------------------------------------------------------------------
    /** @name Functions introduced in DOM Level 1 */
    //@{
    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    /**
     * Returns the name of this attribute.
     * @since DOM Level 1
     */
    virtual const XMLCh *       getName() const = 0;

    /**
     *
     * Returns true if the attribute received its value explicitly in the
     * XML document, or if a value was assigned programatically with
     * the setValue function.  Returns false if the attribute value
     * came from the default value declared in the document's DTD.
     * @since DOM Level 1
     */
    virtual bool            getSpecified() const = 0;

    /**
     * Returns the value of the attribute.
     *
     * The value of the attribute is returned as a string.
     * Character and general entity references are replaced with their values.
     * @since DOM Level 1
     */
    virtual const XMLCh *       getValue() const = 0;

    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    /**
     * Sets the value of the attribute.  A text node with the unparsed contents
     * of the string will be created.
     *
     * @param value The value of the DOM attribute to be set
     * @since DOM Level 1
     */
    virtual void            setValue(const XMLCh *value) = 0;
    //@}

    /** @name Functions introduced in DOM Level 2. */
    //@{
    /**
     * The <code>DOMElement</code> node this attribute is attached to or
     * <code>null</code> if this attribute is not in use.
     *
     * @since DOM Level 2
     */
    virtual DOMElement     *getOwnerElement() const = 0;
    //@}

    /** @name Functions introduced in DOM Level 3. */
    //@{
    /**
     * Returns whether this attribute is known to be of type ID or not. 
     * When it is and its value is unique, the ownerElement of this attribute 
     * can be retrieved using getElementById on Document.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @return <code>bool</code> stating if this <code>DOMAttr</code> is an ID
     * @since DOM level 3
     */
    virtual bool            isId() const = 0;


    /**
     * Returns the type information associated with this attribute.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @return the <code>DOMTypeInfo</code> associated with this attribute
     * @since DOM level 3
     */
    virtual const DOMTypeInfo * getTypeInfo() const = 0;

    //@}

};

XERCES_CPP_NAMESPACE_END

#endif


