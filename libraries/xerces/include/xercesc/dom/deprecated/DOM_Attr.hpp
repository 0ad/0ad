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
 * $Id: DOM_Attr.hpp,v 1.3 2002/11/04 15:04:44 tng Exp $
 */

#ifndef DOM_Attr_HEADER_GUARD_
#define DOM_Attr_HEADER_GUARD_

#include <xercesc/util/XercesDefs.hpp>
#include "DOM_Node.hpp"
#include "DOM_Element.hpp"

XERCES_CPP_NAMESPACE_BEGIN


class AttrImpl;

/**
* The <code>DOM_Attr</code> class refers to an attribute of an XML element.
*
* Typically the allowable values for the
* attribute are defined in a documenttype definition.
* <p><code>DOM_Attr</code> objects inherit the <code>DOM_Node</code>  interface, but
* since attributes are not actually child nodes of the elements they are associated with, the
* DOM does not consider them part of the document  tree.  Thus, the
* <code>DOM_Node</code> attributes <code>parentNode</code>,
* <code>previousSibling</code>, and <code>nextSibling</code> have a  null
* value for <code>DOM_Attr</code> objects. The DOM takes the  view that
* attributes are properties of elements rather than having a  separate
* identity from the elements they are associated with;  this should make it
* more efficient to implement such features as default attributes associated
* with all elements of a  given type.  Furthermore, attribute nodes
* may not be immediate children of a <code>DocumentFragment</code>. However,
* they can be associated with <code>Element</code> nodes contained within a
* <code>DocumentFragment</code>. In short, users of the DOM
* need to be aware that  <code>Attr</code> nodes have some things in  common
* with other objects inheriting the <code>Node</code> interface, but they
* also are quite distinct.
*
*/
class CDOM_EXPORT DOM_Attr: public DOM_Node {

public:
  /** @name Constructors and assignment operators */
  //@{
  /**
    * Default constructor for DOM_Attr.  The resulting object does not
    * refer to any Attribute; it will compare == to 0, and is similar
    * to a null object reference variable in Java.
    *
    */
    DOM_Attr();

public:

  /**
    * Copy constructor.  Creates a new <code>DOM_Attr</code> that refers to the
    *   same underlying Attribute as the original.  See also DOM_Node::clone(),
    * which will copy an actual attribute, rather than just creating a new
    * reference to the original attribute.
    *
    * @param other The source attribute reference object
    */
    DOM_Attr(const DOM_Attr &other);

  /**
    * Assignment operator
    *
    * @param other The source attribute object
    */
    DOM_Attr & operator = (const DOM_Attr &other);

    /**
      * Assignment operator.  This overloaded variant is provided for
      *   the sole purpose of setting a DOM_Node reference variable to
      *   zero.  Nulling out a reference variable in this way will decrement
      *   the reference count on the underlying Node object that the variable
      *   formerly referenced.  This effect is normally obtained when reference
      *   variable goes out of scope, but zeroing them can be useful for
      *   global instances, or for local instances that will remain in scope
      *   for an extended time,  when the storage belonging to the underlying
      *   node needs to be reclaimed.
      *
      * @param val   Only a value of 0, or null, is allowed.
      */
    DOM_Attr & operator = (const DOM_NullPtr *val);



	//@}
  /** @name Destructor */
  //@{
	
  /**
    * Destructor.  The object being destroyed is a reference to the Attribute
    * "node", not the underlying attribute itself.
    *
    */
    ~DOM_Attr();
	//@}

  /** @name Getter functions */
  //@{
    /**
    * Returns the name of this attribute.
    */
    DOMString       getName() const;

    /**
    *
    * Returns true if the attribute received its value explicitly in the
    * XML document, or if a value was assigned programatically with
    * the setValue function.  Returns false if the attribute value
    * came from the default value declared in the document's DTD.
    */
    bool            getSpecified() const;

    /**
	* Returns the value of the attribute.
	*
    * The value of the attribute is returned as a string.
    * Character and general entity references are replaced with their values.
    */
    DOMString       getValue() const;

	//@}
  /** @name Setter functions */
  //@{
    /**
	* Sets the value of the attribute.  A text node with the unparsed contents
    * of the string will be created.
	*
    * @param value The value of the DOM attribute to be set
    */
    void            setValue(const DOMString &value);
	//@}

    /** @name Functions introduced in DOM Level 2. */
    //@{
    /**
     * The <code>DOM_Element</code> node this attribute is attached to or
     * <code>null</code> if this attribute is not in use.
     *
     */
    DOM_Element     getOwnerElement() const;
    //@}

protected:
    DOM_Attr(AttrImpl *attr);

    friend class DOM_Element;
    friend class DOM_Document;

};

XERCES_CPP_NAMESPACE_END

#endif


