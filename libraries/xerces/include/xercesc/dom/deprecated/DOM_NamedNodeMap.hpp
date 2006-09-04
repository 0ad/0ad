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
 * $Id: DOM_NamedNodeMap.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#ifndef DOM_NamedNodeMap_HEADER_GUARD_
#define DOM_NamedNodeMap_HEADER_GUARD_

#include <xercesc/util/XercesDefs.hpp>

#include "DOM_Node.hpp"

XERCES_CPP_NAMESPACE_BEGIN


class NamedNodeMapImpl;

/**
*  <code>NamedNodeMap</code>s  are used to
* represent collections of nodes that can be accessed by name.
*
* Note that <code>NamedNodeMap</code> does not inherit from <code>NodeList</code>;
* <code>NamedNodeMap</code>s are not maintained in any particular order.
* Nodes contained in a <code>NamedNodeMap</code> may
* also be accessed by an ordinal index, but this is simply to allow
* convenient enumeration of the contents, and
* does not imply that the DOM specifies an order to these Nodes.
*/
class DEPRECATED_DOM_EXPORT DOM_NamedNodeMap {
private:
    void     *fImpl;
	short    flagElem;

	static const unsigned short NNM_ELEMENT;
	static const unsigned short NNM_OTHER;	

public:
    /** @name Constructors and assignment operator */
    //@{
    /**
      * Default constructor for DOM_NamedNodeMap.  The resulting object does not
      * refer to an actual DOM_NamedNodeMap node; it will compare == to 0, and is similar
      * to a null object reference variable in Java. NamedNopes are instantiated
      * by these methods:  DOM_Node::getAttributes, DOM_DocumentType::getEntities
      * andDOM_DocumentType::getNotations
      *
      */
    DOM_NamedNodeMap();

    /**
      * Copy constructor.  Creates a new <code>DOM_NamedNodeMap</code> reference object
      * that refers to the same underlying NamedNodeMap as the original.
      *
      * @param other The object to be copied.
      */
    DOM_NamedNodeMap(const DOM_NamedNodeMap &other);

    /**
      * Assignment operator.
      *
      * @param other The object to be copied.
      */
    DOM_NamedNodeMap & operator = (const DOM_NamedNodeMap &other);

    /**
      * Assignment operator.
      *
      * @param other The object to be copied.
      */
    DOM_NamedNodeMap & operator = (const DOM_NullPtr *other);

    //@}
    /** @name Destructor. */
    //@{
    /**
      * Destructor for DOM_NamedNodeMap.  The object being destroyed is the reference
      * object, not the underlying NamedNodeMap itself.
      *
      * <p>Like most other DOM types in this implementation, memory management
      * of named node maps is automatic.  Instances of DOM_NamedNodeMap function
      * as references to an underlying heap based implementation object,
      * and should never be explicitly new-ed or deleted in application code, but
      * should appear only as local variables or function parameters.
	  *
	  */
    ~DOM_NamedNodeMap();

    //@}

    /** @Comparisons. */
    //@{
    /**
     *  Test whether this NamedNodeMap reference refers to the same underlying
     *  named node map as the other reference object.  This does not
     *  compare the contents of two different objects.
     *
     *  @param other The value to be compared
     *  @return Returns true if the underlying named node map is same
     */
    bool operator == (const DOM_NamedNodeMap &other) const;

    /**
     *  Test whether this NamedNodeMap reference refers to a different underlying
     *  named node map as the other reference object.  This does not
     *  compare the contents of two different objects.
     *
     *  @param other The value to be compared
     *  @return Returns true if the underlying named node map is different
     */
    bool operator != (const DOM_NamedNodeMap &other) const;


    /**
     *  Use this comparison operator to test whether a Named Node Map reference
     *  is null.
     *
     *  @param p The value to be compared, which must be 0 or null.
     *  @return Returns true if the named node map is null
     */
    bool operator == (const DOM_NullPtr *p) const;

    /**
     *  Use this comparison operator to test whether a Named Node Map reference
     *  is not null.
     *
     *  @param p The value to be compared, which must not be 0 or null.
     *  @return Returns true if the named node map is not null
     */
    bool operator != (const DOM_NullPtr *p) const;

    //@}

    /** @name Set functions. */
    //@{

    /**
    * Adds a node using its <code>nodeName</code> attribute.
    *
    * <br>As the <code>nodeName</code> attribute is used to derive the name
    * which the node must be stored under, multiple nodes of certain types
    * (those that have a "special" string value) cannot be stored as the names
    * would clash. This is seen as preferable to allowing nodes to be aliased.
    * @param arg A node to store in a named node map. The node will later be
    *   accessible using the value of the <code>nodeName</code> attribute of
    *   the node. If a node with that name is already present in the map, it
    *   is replaced by the new one.
    * @return If the new <code>Node</code> replaces an existing node the
    *   replaced <code>Node</code> is returned,
    *   otherwise <code>null</code> is returned.
    * @exception DOMException
    *   WRONG_DOCUMENT_ERR: Raised if <code>arg</code> was created from a
    *   different document than the one that created the
    *   <code>NamedNodeMap</code>.
    *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this
    *   <code>NamedNodeMap</code> is readonly.
    *   <br>INUSE_ATTRIBUTE_ERR: Raised if <code>arg</code> is an
    *   <code>Attr</code> that is already an attribute of another
    *   <code>Element</code> object. The DOM user must explicitly clone
    *   <code>Attr</code> nodes to re-use them in other elements.
    */
    DOM_Node               setNamedItem(DOM_Node arg);

    //@}
    /** @name Get functions. */
    //@{

    /**
    * Returns the <code>index</code>th item in the map.
    *
    * If <code>index</code>
    * is greater than or equal to the number of nodes in the map, this returns
    * <code>null</code>.
    * @param index Index into the map.
    * @return The node at the <code>index</code>th position in the
    *   <code>NamedNodeMap</code>, or <code>null</code> if that is not a valid
    *   index.
    */
    DOM_Node               item(unsigned int index) const;

    /**
    * Retrieves a node specified by name.
    *
    * @param name The <code>nodeName</code> of a node to retrieve.
    * @return A <code>DOM_Node</code> (of any type) with the specified <code>nodeName</code>, or
    *   <code>null</code> if it does not identify any node in
    *   the map.
    */
    DOM_Node               getNamedItem(const DOMString &name) const;

    /**
    * The number of nodes in the map.
    *
    * The range of valid child node indices is
    * 0 to <code>length-1</code> inclusive.
    */
    unsigned int           getLength() const;

    //@}
    /** @name Functions to change the node collection. */
    //@{

    /**
    * Removes a node specified by name.
    *
    * If the removed node is an
    * <code>Attr</code> with a default value it is immediately replaced.
    * @param name The <code>nodeName</code> of a node to remove.
    * @return The node removed from the map or <code>null</code> if no node
    *   with such a name exists.
    * @exception DOMException
    *   NOT_FOUND_ERR: Raised if there is no node named <code>name</code> in
    *   the map.
    * <br>
    *   NO_MODIFICATION_ALLOWED_ERR: Raised if this <code>NamedNodeMap</code>
    *   is readonly.
    */
    DOM_Node               removeNamedItem(const DOMString &name);

    //@}
    /** @name Functions introduced in DOM Level 2. */
    //@{

    /**
     * Retrieves a node specified by local name and namespace URI.
     *
     * @param namespaceURI The <em>namespace URI</em> of
     *    the node to retrieve.
     * @param localName The <em>local name</em> of the node to retrieve.
     * @return A <code>DOM_Node</code> (of any type) with the specified
     *    local name and namespace URI, or <code>null</code> if they do not
     *    identify any node in the map.
     */
    DOM_Node               getNamedItemNS(const DOMString &namespaceURI,
	const DOMString &localName);

    /**
     * Adds a node using its <CODE>namespaceURI</CODE> and <CODE>localName</CODE>.
     *
     * @param arg A node to store in a named node map. The node will later be
     *       accessible using the value of the <CODE>namespaceURI</CODE> and
     *       <CODE>localName</CODE> attribute of the node. If a node with those
     *       namespace URI and local name is already present in the map, it is
     *       replaced by the new one.
     * @return If the new <code>DOM_Node</code> replaces an existing node the
     *   replaced <code>DOM_Node</code> is returned,
     *   otherwise <code>null</code> is returned.
     * @exception DOMException
     *   WRONG_DOCUMENT_ERR: Raised if <code>arg</code> was created from a
     *   different document than the one that created the
     *   <code>DOM_NamedNodeMap</code>.
     *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this
     *   <code>vNamedNodeMap</code> is readonly.
     *   <br>INUSE_ATTRIBUTE_ERR: Raised if <code>arg</code> is an
     *   <code>DOM_Attr</code> that is already an attribute of another
     *   <code>DOM_Element</code> object. The DOM user must explicitly clone
     *   <code>DOM_Attr</code> nodes to re-use them in other elements.
     */
    DOM_Node               setNamedItemNS(DOM_Node arg);

    /**
     * Removes a node specified by local name and namespace URI.
     *
     * @param namespaceURI The <em>namespace URI</em> of
     *    the node to remove.
     * @param localName The <em>local name</em> of the
     *    node to remove. When this <code>DOM_NamedNodeMap</code> contains the
     *    attributes attached to an element, as returned by the attributes
     *    attribute of the <code>DOM_Node</code> interface, if the removed
     *    attribute is known to have a default value, an attribute
     *    immediately appears containing the default value
     *    as well as the corresponding namespace URI, local name, and prefix.
     * @return The node removed from the map if a node with such a local name
     *    and namespace URI exists.
     * @exception DOMException
     *   NOT_FOUND_ERR: Raised if there is no node named <code>name</code> in
     *   the map.
     * <br>
     *   NO_MODIFICATION_ALLOWED_ERR: Raised if this <code>DOM_NamedNodeMap</code>
     *   is readonly.
     */
    DOM_Node               removeNamedItemNS(const DOMString &namespaceURI,
	const DOMString &localName);

    //@}

 protected:

    DOM_NamedNodeMap(NamedNodeMapImpl *impl);
	DOM_NamedNodeMap(NodeImpl *impl);

    friend class DOM_DocumentType;
    friend class DOM_Node;


};

XERCES_CPP_NAMESPACE_END

#endif

