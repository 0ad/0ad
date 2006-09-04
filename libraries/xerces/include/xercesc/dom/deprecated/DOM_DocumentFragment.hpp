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
 * $Id: DOM_DocumentFragment.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#ifndef DOM_DocumentFragment_HEADER_GUARD_
#define DOM_DocumentFragment_HEADER_GUARD_

#include <xercesc/util/XercesDefs.hpp>
#include "DOM_Node.hpp"

XERCES_CPP_NAMESPACE_BEGIN


class DocumentFragmentImpl;

/**
 * <code>DocumentFragment</code> is a "lightweight" or "minimal"
 * <code>Document</code> object.
 *
 * It is very common to want to be able to
 * extract a portion of a document's tree or to create a new fragment of a
 * document. Imagine implementing a user command like cut or rearranging a
 * document by moving fragments around. It is desirable to have an object
 * which can hold such fragments and it is quite natural to use a Node for
 * this purpose. While it is true that a <code>Document</code> object could
 * fulfil this role,  a <code>Document</code> object can potentially be a
 * heavyweight  object, depending on the underlying implementation. What is
 * really needed for this is a very lightweight object.
 * <code>DocumentFragment</code> is such an object.
 * <p>Furthermore, various operations -- such as inserting nodes as children
 * of another <code>Node</code> -- may take <code>DocumentFragment</code>
 * objects as arguments;  this results in all the child nodes of the
 * <code>DocumentFragment</code>  being moved to the child list of this node.
 * <p>The children of a <code>DocumentFragment</code> node are zero or more
 * nodes representing the tops of any sub-trees defining the structure of the
 * document. <code>DocumentFragment</code> nodes do not need to be
 * well-formed XML documents (although they do need to follow the rules
 * imposed upon well-formed XML parsed entities, which can have multiple top
 * nodes).  For example, a <code>DocumentFragment</code> might have only one
 * child and that child node could be a <code>Text</code> node. Such a
 * structure model  represents neither an HTML document nor a well-formed XML
 * document.
 * <p>When a <code>DocumentFragment</code> is inserted into a
 * <code>Document</code> (or indeed any other <code>Node</code> that may take
 * children) the children of the <code>DocumentFragment</code> and not the
 * <code>DocumentFragment</code>  itself are inserted into the
 * <code>Node</code>. This makes the <code>DocumentFragment</code> very
 * useful when the user wishes to create nodes that are siblings; the
 * <code>DocumentFragment</code> acts as the parent of these nodes so that the
 *  user can use the standard methods from the <code>Node</code>  interface,
 * such as <code>insertBefore()</code> and  <code>appendChild()</code>.
 */

class DEPRECATED_DOM_EXPORT DOM_DocumentFragment: public DOM_Node {

public:
    /** @name Constructors and assignment operators */
    //@{
    /**
    * Default constructor for <code>DOM_DocumentFragment</code>.  The resulting object does not
    * refer to an actual Document Fragment node; it will compare == to 0, and is similar
    * to a null object reference variable in Java.  It may subsequently be
    * assigned to refer to an actual Document Fragment node.
    * <p>
    * New document fragment nodes are created by DOM_Document::createDocumentFragment().
    *
    */

    DOM_DocumentFragment();

    /**
      * Copy constructor.  Creates a new <code>DOM_DocumentFragment</code> that refers to the
      *   same underlying node as the original.  See also DOM_Node::clone(),
      * which will copy the actual Document fragment node, rather than just creating a new
      * reference to the original node.
      *
      * @param other The object to be copied
      */
    DOM_DocumentFragment(const DOM_DocumentFragment &other);

    /**
      * Assignment operator
      *
      * @param other The object to be copied
      */
    DOM_DocumentFragment & operator = (const DOM_DocumentFragment &other);

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
    DOM_DocumentFragment & operator = (const DOM_NullPtr *val);

	//@}
    /** @name Destructor */
    //@{
	
    /**
      * Destructor.  The object being destroyed is the reference
      * object, not the underlying Comment node itself.
      *
      */
    ~DOM_DocumentFragment();

	//@}

protected:
    DOM_DocumentFragment(DocumentFragmentImpl *);

    friend class DOM_Document;
    friend class RangeImpl;
};

XERCES_CPP_NAMESPACE_END

#endif
