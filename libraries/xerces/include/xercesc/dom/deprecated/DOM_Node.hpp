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
 * $Id: DOM_Node.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#ifndef DOM_Node_HEADER_GUARD_
#define DOM_Node_HEADER_GUARD_

#include <xercesc/util/XercesDefs.hpp>
#include "DOMString.hpp"

XERCES_CPP_NAMESPACE_BEGIN

class DOM_NodeList;
class DOM_NamedNodeMap;
class DOM_Document;
class NodeImpl;

class DOM_NullPtr;  // A dummy class, with no implementation, that is
                    //  used as in overloaded functions as a way to
                    //  pass 0 or null.

/**
 * The <code>Node</code> interface is the primary datatype for the entire
 * Document Object Model.
 *
 * It represents a single node in the document tree.
 * While all objects implementing the <code>Node</code> interface expose
 * methods for dealing with children, not all objects implementing the
 * <code>Node</code> interface may have children. For example,
 * <code>Text</code> nodes may not have children, and adding children to such
 * nodes results in a <code>DOMException</code> being raised.
 * <p>The attributes <code>nodeName</code>, <code>nodeValue</code>  and
 * <code>attributes</code> are  included as a mechanism to get at node
 * information without  casting down to the specific derived interface. In
 * cases where  there is no obvious mapping of these attributes for a specific
 *  <code>nodeType</code> (e.g., <code>nodeValue</code> for an Element  or
 * <code>attributes</code>  for a Comment), this returns <code>null</code>.
 * Note that the  specialized interfaces may contain additional and more
 * convenient mechanisms to get and set the relevant information.
 */
class  DEPRECATED_DOM_EXPORT DOM_Node {

    public:
    /** @name Constructors and assignment operators */
    //@{
    /**
      * Default constructor for DOM_Node.  The resulting object does not
      * refer to an actual  node; it will compare == to 0, and is similar
      * to a null object reference variable in Java.  It may subsequently be
      * assigned to refer to an actual node.  "Acutal Nodes" will always
      * be of some derived type, such as Element or Attr.
      *
      */
    DOM_Node();

    /**
      * Copy constructor.
      *
      * @param other The object to be copied.
      */
    DOM_Node(const DOM_Node &other);

    /**
      * Assignment operator.
      *
      * @param other The source to be assigned.
      */
    DOM_Node & operator = (const DOM_Node &other);

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
    DOM_Node & operator = (const DOM_NullPtr *val);

   //@}
    /** @name Destructor. */
    //@{
	 /**
	  * Destructor for DOM_Node.  The object being destroyed is the reference
      * object, not the underlying node itself.
	  *
	  */
    ~DOM_Node();

    //@}
    /** @name Equality and Inequality operators. */
    //@{
    /**
     * The equality operator.  This compares to references to nodes, and
     * returns true if they both refer to the same underlying node.  It
     * is exactly analogous to Java's operator ==  on object reference
     * variables.  This operator can not be used to compare the values
     * of two different nodes in the document tree.
     *
     * @param other The object reference with which <code>this</code> object is compared
     * @returns True if both <code>DOM_Node</code>s refer to the same
     *  actual node, or are both null; return false otherwise.
     */
    bool operator == (const DOM_Node & other)const;

    /**
      *  Compare with a pointer.  Intended only to allow a convenient
      *    comparison with null.
      *
      */
    bool operator == (const DOM_NullPtr *other) const;

    /**
     * The inequality operator.  See operator ==.
     *
     */
    bool operator != (const DOM_Node & other) const;

     /**
      *  Compare with a pointer.  Intended only to allow a convenient
      *    comparison with null.
      *
      */
   bool operator != (const DOM_NullPtr * other) const;


    enum NodeType {
        ELEMENT_NODE                = 1,
        ATTRIBUTE_NODE              = 2,
        TEXT_NODE                   = 3,
        CDATA_SECTION_NODE          = 4,
        ENTITY_REFERENCE_NODE       = 5,
        ENTITY_NODE                 = 6,
        PROCESSING_INSTRUCTION_NODE = 7,
        COMMENT_NODE                = 8,
        DOCUMENT_NODE               = 9,
        DOCUMENT_TYPE_NODE          = 10,
        DOCUMENT_FRAGMENT_NODE      = 11,
        NOTATION_NODE               = 12,
        XML_DECL_NODE               = 13
    };

    //@}
    /** @name Get functions. */
    //@{

    /**
     * The name of this node, depending on its type; see the table above.
     */
    DOMString       getNodeName() const;

    /**
     * Gets the value of this node, depending on its type.
     *
     * @exception DOMException
     *   NO_MODIFICATION_ALLOWED_ERR: Raised when the node is readonly.
     * @exception DOMException
     *   DOMSTRING_SIZE_ERR: Raised when it would return more characters than
     *   fit in a <code>DOMString</code> variable on the implementation
     *   platform.
     */
    DOMString       getNodeValue() const;

    /**
     * An enum value representing the type of the underlying object.
     */
    short           getNodeType() const;

    /**
     * Gets the parent of this node.
     *
     * All nodes, except <code>Document</code>,
     * <code>DocumentFragment</code>, and <code>Attr</code> may have a parent.
     * However, if a node has just been created and not yet added to the tree,
     * or if it has been removed from the tree, a <code>null</code> DOM_Node
     * is returned.
     */
    DOM_Node        getParentNode() const;

    /**
     * Gets a <code>NodeList</code> that contains all children of this node.
     *
     * If there
     * are no children, this is a <code>NodeList</code> containing no nodes.
     * The content of the returned <code>NodeList</code> is "live" in the sense
     * that, for instance, changes to the children of the node object that
     * it was created from are immediately reflected in the nodes returned by
     * the <code>NodeList</code> accessors; it is not a static snapshot of the
     * content of the node. This is true for every <code>NodeList</code>,
     * including the ones returned by the <code>getElementsByTagName</code>
     * method.
     */
    DOM_NodeList    getChildNodes() const;
    /**
     * Gets the first child of this node.
     *
     * If there is no such node, this returns <code>null</code>.
     */
    DOM_Node        getFirstChild() const;

    /**
     * Gets the last child of this node.
     *
     * If there is no such node, this returns <code>null</code>.
     */
    DOM_Node        getLastChild() const;

    /**
     * Gets the node immediately preceding this node.
     *
     * If there is no such node, this returns <code>null</code>.
     */
    DOM_Node        getPreviousSibling() const;

    /**
     * Gets the node immediately following this node.
     *
     * If there is no such node, this returns <code>null</code>.
     */
    DOM_Node        getNextSibling() const;

    /**
     * Gets a <code>NamedNodeMap</code> containing the attributes of this node (if it
     * is an <code>Element</code>) or <code>null</code> otherwise.
     */
    DOM_NamedNodeMap  getAttributes() const;

    /**
     * Gets the <code>DOM_Document</code> object associated with this node.
     *
     * This is also
     * the <code>DOM_Document</code> object used to create new nodes. When this
     * node is a <code>DOM_Document</code> or a <code>DOM_DocumentType</code>
     * which is not used with any <code>DOM_Document</code> yet, this is
     * <code>null</code>.
     *
     */
    DOM_Document      getOwnerDocument() const;

    /**
      * Return the user data pointer.
      *
      * User data allows application programs
      * to attach extra data to DOM nodes, and can be set using the
      * function <code>DOM_Node::setUserData(p)</code>.
      * @return The user data pointer.
      */
    void              *getUserData() const;

    //@}
    /** @name Cloning function. */
    //@{

    /**
     * Returns a duplicate of this node.
     *
     * This function serves as a generic copy constructor for nodes.
     *
     * The duplicate node has no parent (
     * <code>parentNode</code> returns <code>null</code>.).
     * <br>Cloning an <code>Element</code> copies all attributes and their
     * values, including those generated by the  XML processor to represent
     * defaulted attributes, but this method does not copy any text it contains
     * unless it is a deep clone, since the text is contained in a child
     * <code>Text</code> node. Cloning any other type of node simply returns a
     * copy of this node.
     * @param deep If <code>true</code>, recursively clone the subtree under the
     *   specified node; if <code>false</code>, clone only the node itself (and
     *   its attributes, if it is an <code>Element</code>).
     * @return The duplicate node.
     */
    DOM_Node         cloneNode(bool deep) const;

    //@}
    /** @name Functions to modify the DOM Node. */
    //@{

    /**
     * Inserts the node <code>newChild</code> before the existing child node
     * <code>refChild</code>.
     *
     * If <code>refChild</code> is <code>null</code>,
     * insert <code>newChild</code> at the end of the list of children.
     * <br>If <code>newChild</code> is a <code>DocumentFragment</code> object,
     * all of its children are inserted, in the same order, before
     * <code>refChild</code>. If the <code>newChild</code> is already in the
     * tree, it is first removed.  Note that a <code>DOM_Node</code> that
     * has never been assigned to refer to an actual node is == null.
     * @param newChild The node to insert.
     * @param refChild The reference node, i.e., the node before which the new
     *   node must be inserted.
     * @return The node being inserted.
     * @exception DOMException
     *   HIERARCHY_REQUEST_ERR: Raised if this node is of a type that does not
     *   allow children of the type of the <code>newChild</code> node, or if
     *   the node to insert is one of this node's ancestors.
     *   <br>WRONG_DOCUMENT_ERR: Raised if <code>newChild</code> was created
     *   from a different document than the one that created this node.
     *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this node or the node being
     *   inserted is readonly.
     *   <br>NOT_FOUND_ERR: Raised if <code>refChild</code> is not a child of
     *   this node.
     */
    DOM_Node               insertBefore(const DOM_Node &newChild,
                                        const DOM_Node &refChild);


    /**
     * Replaces the child node <code>oldChild</code> with <code>newChild</code>
     * in the list of children, and returns the <code>oldChild</code> node.
     *
     * If <CODE>newChild</CODE> is a <CODE>DOM_DocumentFragment</CODE> object,
     * <CODE>oldChild</CODE> is replaced by all of the <CODE>DOM_DocumentFragment</CODE>
     * children, which are inserted in the same order.
     *
     * If the <code>newChild</code> is already in the tree, it is first removed.
     * @param newChild The new node to put in the child list.
     * @param oldChild The node being replaced in the list.
     * @return The node replaced.
     * @exception DOMException
     *   HIERARCHY_REQUEST_ERR: Raised if this node is of a type that does not
     *   allow children of the type of the <code>newChild</code> node, or it
     *   the node to put in is one of this node's ancestors.
     *   <br>WRONG_DOCUMENT_ERR: Raised if <code>newChild</code> was created
     *   from a different document than the one that created this node.
     *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this node or the new node is readonly.
     *   <br>NOT_FOUND_ERR: Raised if <code>oldChild</code> is not a child of
     *   this node.
     */
    DOM_Node       replaceChild(const DOM_Node &newChild,
                                const DOM_Node &oldChild);
    /**
     * Removes the child node indicated by <code>oldChild</code> from the list
     * of children, and returns it.
     *
     * @param oldChild The node being removed.
     * @return The node removed.
     * @exception DOMException
     *   NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
     *   <br>NOT_FOUND_ERR: Raised if <code>oldChild</code> is not a child of
     *   this node.
     */
    DOM_Node        removeChild(const DOM_Node &oldChild);

    /**
     * Adds the node <code>newChild</code> to the end of the list of children of
     * this node.
     *
     * If the <code>newChild</code> is already in the tree, it is
     * first removed.
     * @param newChild The node to add.If it is a  <code>DocumentFragment</code>
     *   object, the entire contents of the document fragment are moved into
     *   the child list of this node
     * @return The node added.
     * @exception DOMException
     *   HIERARCHY_REQUEST_ERR: Raised if this node is of a type that does not
     *   allow children of the type of the <code>newChild</code> node, or if
     *   the node to append is one of this node's ancestors.
     *   <br>WRONG_DOCUMENT_ERR: Raised if <code>newChild</code> was created
     *   from a different document than the one that created this node.
     *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this node or the node being
     *   appended is readonly.
     */
    DOM_Node        appendChild(const DOM_Node &newChild);

    //@}
    /** @name Query functions. */
    //@{

    /**
     *  This is a convenience method to allow easy determination of whether a
     * node has any children.
     *
     * @return  <code>true</code> if the node has any children,
     *   <code>false</code> if the node has no children.
     */
    bool             hasChildNodes() const;


    /**
     * Test whether this node is null.
     *
     * This C++ class, <code>DOM_Node<code>
     * functions much like an object reference to an underlying Node, and
     * this function tests for that reference being null.  Several DOM
     * APIs, <code>Node.getNextSibling()</code> for example, can return null, and
     * this function is used to test for that condition.
     *
     * <p>Operator == provides another way to perform this null test on a
     * DOM_Node.
     */
    bool                    isNull() const;

    //@}
    /** @name Set functions. */
    //@{


    /**
    * Sets the value of the node.
    *
    * Any node which can have a nodeValue (@see getNodeValue) will
    * also accept requests to set it to a string. The exact response to
    * this varies from node to node -- Attribute, for example, stores
    * its values in its children and has to replace them with a new Text
    * holding the replacement value.
    *
    * For most types of Node, value is null and attempting to set it
    * will throw DOMException(NO_MODIFICATION_ALLOWED_ERR). This will
    * also be thrown if the node is read-only.
    */
    void              setNodeValue(const DOMString &nodeValue);

    /**
      * Set the user data for a node.
      *
      * User data allows application programs
      * to attach extra data to DOM nodes, and can be retrieved using the
      * function <code>DOM_Node::getUserData(p)</code>.
      * <p>
      * Deletion of the user data remains the responsibility of the
      * application program; it will not be automatically deleted when
      * the nodes themselves are reclaimed.
      *
      * <p> Because DOM_Node is not designed to be subclassed, userdata
      * provides an alternative means for extending the the information
      * kept with nodes by an application program.
      *
      * @param p The pointer to be kept with the node.
      */
    void              setUserData(void *p);

    //@}
    /** @name Functions introduced in DOM Level 2. */
    //@{

    /**
     * Puts all <CODE>DOM_Text</CODE>
     * nodes in the full depth of the sub-tree underneath this <CODE>DOM_Node</CODE>,
     * including attribute nodes, into a "normal" form where only markup (e.g.,
     * tags, comments, processing instructions, CDATA sections, and entity
     * references) separates <CODE>DOM_Text</CODE>
     * nodes, i.e., there are neither adjacent <CODE>DOM_Text</CODE>
     * nodes nor empty <CODE>DOM_Text</CODE>
     * nodes. This can be used to ensure that the DOM view of a document is the
     * same as if it were saved and re-loaded, and is useful when operations
     * (such as XPointer lookups) that depend on a particular document tree
     * structure are to be used.
     * <P><B>Note:</B> In cases where the document contains <CODE>DOM_CDATASections</CODE>,
     * the normalize operation alone may not be sufficient, since XPointers do
     * not differentiate between <CODE>DOM_Text</CODE>
     * nodes and <CODE>DOM_CDATASection</CODE>
     * nodes.</P>
     *
     */
    void              normalize();

    /**
     * Tests whether the DOM implementation implements a specific
     * feature and that feature is supported by this node.
     *
     * @param feature The string of the feature to test. This is the same
     * name as what can be passed to the method <code>hasFeature</code> on
     * <code>DOM_DOMImplementation</code>.
     * @param version This is the version number of the feature to test. In
     * Level 2, version 1, this is the string "2.0". If the version is not
     * specified, supporting any version of the feature will cause the
     * method to return <code>true</code>.
     * @return Returns <code>true</code> if the specified feature is supported
     * on this node, <code>false</code> otherwise.
     */
    bool              isSupported(const DOMString &feature,
	                       const DOMString &version) const;

    /**
     * Get the <em>namespace URI</em> of
     * this node, or <code>null</code> if it is unspecified.
     * <p>
     * This is not a computed value that is the result of a namespace lookup
     * based on an examination of the namespace declarations in scope. It is
     * merely the namespace URI given at creation time.
     * <p>
     * For nodes of any type other than <CODE>ELEMENT_NODE</CODE> and
     * <CODE>ATTRIBUTE_NODE</CODE> and nodes created with a DOM Level 1 method,
     * such as <CODE>createElement</CODE> from the <CODE>DOM_Document</CODE>
     * interface, this is always <CODE>null</CODE>.
     *
     */
    DOMString         getNamespaceURI() const;

    /**
     * Get the <em>namespace prefix</em>
     * of this node, or <code>null</code> if it is unspecified.
     *
     */
    DOMString         getPrefix() const;

    /**
     * Returns the local part of the <em>qualified name</em> of this node.
     * <p>
     * For nodes created with a DOM Level 1 method, such as
     * <code>createElement</code> from the <code>DOM_Document</code> interface,
     * it is null.
     *
     */
    DOMString         getLocalName() const;

    /**
     * Set the <em>namespace prefix</em> of this node.
     * <p>
     * Note that setting this attribute, when permitted, changes
     * the <CODE>nodeName</CODE> attribute, which holds the <EM>qualified
     * name</EM>, as well as the <CODE>tagName</CODE> and <CODE>name</CODE>
     * attributes of the <CODE>DOM_Element</CODE> and <CODE>DOM_Attr</CODE>
     * interfaces, when applicable.
     * <p>
     * Note also that changing the prefix of an
     * attribute, that is known to have a default value, does not make a new
     * attribute with the default value and the original prefix appear, since the
     * <CODE>namespaceURI</CODE> and <CODE>localName</CODE> do not change.
     *
     * @param prefix The prefix of this node.
     * @exception DOMException
     *   INVALID_CHARACTER_ERR: Raised if the specified prefix contains
     *                          an illegal character.
     * <br>
     *   NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
     * <br>
     *   NAMESPACE_ERR: Raised if the specified <CODE>prefix</CODE> is
     *      malformed, if the <CODE>namespaceURI</CODE> of this node is
     *      <CODE>null</CODE>, if the specified prefix is "xml" and the
     *      <CODE>namespaceURI</CODE> of this node is different from
     *      "http://www.w3.org/XML/1998/namespace", if this node is an attribute
     *      and the specified prefix is "xmlns" and the
     *      <CODE>namespaceURI</CODE> of this node is different from
     *      "http://www.w3.org/2000/xmlns/", or if this node is an attribute and
     *      the <CODE>qualifiedName</CODE> of this node is "xmlns".
     */
    void              setPrefix(const DOMString &prefix);

    /**
     *  Returns whether this node (if it is an element) has any attributes.
     * @return <code>true</code> if this node has any attributes,
     *   <code>false</code> otherwise.
     */
    bool              hasAttributes() const;

    //@}

protected:
    NodeImpl   *fImpl;

    DOM_Node(NodeImpl *);

    friend class DOM_Document;
    friend class DocumentImpl;
    friend class TreeWalkerImpl;
    friend class NodeIteratorImpl;
    friend class DOM_NamedNodeMap;
    friend class DOM_NodeList;
    friend class DOMParser;
    friend class DOM_Entity;
    friend class RangeImpl;
    friend class CharacterDataImpl;
	friend class XUtil;

};

XERCES_CPP_NAMESPACE_END

#endif

