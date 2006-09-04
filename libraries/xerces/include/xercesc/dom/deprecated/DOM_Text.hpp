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
 * $Id: DOM_Text.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#ifndef DOM_Text_HEADER_GUARD_
#define DOM_Text_HEADER_GUARD_

#include <xercesc/util/XercesDefs.hpp>
#include "DOM_CharacterData.hpp"

XERCES_CPP_NAMESPACE_BEGIN


class TextImpl;


/**
 * The <code>Text</code> interface represents the textual content (termed
 * character  data in XML) of an <code>Element</code> or <code>Attr</code>.
 * If there is no markup inside an element's content, the text is contained
 * in a single object implementing the <code>Text</code> interface that is
 * the only child of the element. If there is markup, it is parsed into a
 * list of elements and <code>Text</code> nodes that form the list of
 * children of the element.
 * <p>When a document is first made available via the DOM, there is  only one
 * <code>Text</code> node for each block of text. Users may create  adjacent
 * <code>Text</code> nodes that represent the  contents of a given element
 * without any intervening markup, but should be aware that there is no way
 * to represent the separations between these nodes in XML, so they
 * will not (in general) persist between DOM editing sessions. The
 * <code>normalize()</code> method on <code>Element</code> merges any such
 * adjacent <code>Text</code> objects into a single node for each block of
 * text; this is  recommended before employing operations that depend on a
 * particular document structure, such as navigation with
 * <code>XPointers.</code>
 */
class DEPRECATED_DOM_EXPORT DOM_Text: public DOM_CharacterData {

    public:
    /** @name Constructors and assignment operator */
    //@{
    /**
      * Default constructor for DOM_Text.  The resulting object does not
      * refer to an actual Text node; it will compare == to 0, and is similar
      * to a null object reference variable in Java.  It may subsequently be
      * assigned to refer to an actual comment node.
      *
      */
    DOM_Text();

    /**
      * Copy constructor.  Creates a new <code>DOM_Text</code> that refers to the
      * same underlying node as the original.  See also DOM_Node::clone(),
      * which will copy the actual Text node, rather than just creating a new
      * reference to the original node.
      *
      * @param other The object to be copied.
      */
    DOM_Text(const DOM_Text &other);

    /**
      * Assignment operator.
      *
      * @param other The object to be copied.
      */
    DOM_Text & operator = (const DOM_Text &other);

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
      * @param val  Only a value of 0, or null, is allowed.
      */
    DOM_Text & operator = (const DOM_NullPtr *val);

    //@}
    /** @name Destructor. */
    //@{
	 /**
	  * Destructor for DOM_Text. The object being destroyed is the reference
      * object, not the underlying Comment node itself.
	  *
	  */
    ~DOM_Text();

    //@}
    /** @name Functions to modify the Text node. */
    //@{

    /**
     * Breaks this node into two nodes at the specified
     * offset, keeping both in the tree as siblings.
     *
     * This node then only
     * contains all the content up to the <code>offset</code> point. And a new
     * node of the same nodeType, which is inserted as the next sibling of this
     * node, contains all the content at and after the <code>offset</code>
     * point. When the <code>offset</code> is equal to the lenght of this node,
     * the new node has no data.
     * @param offset The offset at which to split, starting from 0.
     * @return The new <code>Text</code> node.
     * @exception DOMException
     *   INDEX_SIZE_ERR: Raised if the specified offset is negative or greater
     *   than the number of characters in <code>data</code>.
     *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
     */
    DOM_Text splitText(unsigned int offset);

    //@}
    /** @name Non-standard (not defined by the DOM specification) functions. */
    //@{

    /**
     *
     * Return true if this node contains ignorable whitespaces only.
     * @return True if this node contains ignorable whitespaces only.
     */
    bool isIgnorableWhitespace();

    //@}

protected:
    DOM_Text(TextImpl *);

    friend class DOM_Document;
    friend class RangeImpl;



};

XERCES_CPP_NAMESPACE_END

#endif


