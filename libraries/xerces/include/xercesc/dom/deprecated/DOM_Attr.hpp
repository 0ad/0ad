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
 * $Id: DOM_Attr.hpp 176026 2004-09-08 13:57:07Z peiyongz $
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
class DEPRECATED_DOM_EXPORT DOM_Attr: public DOM_Node {

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


