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
 * $Id: DOM_NodeIterator.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#ifndef DOM_NodeIterator_HEADER_GUARD_
#define DOM_NodeIterator_HEADER_GUARD_

#include "DOM_NodeFilter.hpp"
#include "DOM_Node.hpp"

XERCES_CPP_NAMESPACE_BEGIN


class NodeIteratorImpl;

/**
 * NodeIterators are used to step through a set of nodes
 * e.g. the set of nodes in a NodeList, the document subtree governed by
 * a particular node, the results of a query, or any other set of nodes.
 * The set of nodes to be iterated is determined by the implementation
 * of the NodeIterator. DOM Level 2 specifies a single NodeIterator
 * implementation for document-order traversal of a document
 * subtree. Instances of these iterators are created by calling
 * <code>DocumentTraversal.createNodeIterator()</code>.
 *
 */
class DEPRECATED_DOM_EXPORT DOM_NodeIterator
{
    public:
        /** @name Constructors and assignment operator */
        //@{
        /**
          * Default constructor.
          */
        DOM_NodeIterator ();

        /**
          * Copy constructor.
          *
          * @param other The object to be copied.
          */
        DOM_NodeIterator(const DOM_NodeIterator &other);

        /**
          * Assignment operator.
          *
          * @param other The object to be copied.
          */
        DOM_NodeIterator & operator = (const DOM_NodeIterator &other);

        /**
          * Assignment operator.  This overloaded variant is provided for
          *   the sole purpose of setting a DOM_NodeIterator to null.
          *
          * @param val   Only a value of 0, or null, is allowed.
          */
        DOM_NodeIterator & operator = (const DOM_NullPtr *val);
        //@}

        /** @name Destructor. */
        //@{
	/**
	  * Destructor for DOM_NodeIterator.
	  */
        ~DOM_NodeIterator();
        //@}

        /** @name Equality and Inequality operators. */
        //@{
        /**
         * The equality operator.
         *
         * @param other The object reference with which <code>this</code> object is compared
         * @returns True if both <code>DOM_NodeIterator</code>s refer to the same
         *  actual node, or are both null; return false otherwise.
         */
        bool operator == (const DOM_NodeIterator & other)const;

        /**
          *  Compare with a pointer.  Intended only to allow a convenient
          *    comparison with null.
          */
        bool operator == (const DOM_NullPtr *other) const;

        /**
         * The inequality operator.  See operator ==.
         */
        bool operator != (const DOM_NodeIterator & other) const;

         /**
          *  Compare with a pointer.  Intended only to allow a convenient
          *    comparison with null.
          *
          */
        bool operator != (const DOM_NullPtr * other) const;
        //@}

        /** @name Get functions. */
        //@{
        /**
         * The root node of the <code>NodeIterator</code>, as specified when it
         * was created.
         */
        DOM_Node            getRoot();

        /**
          * Return which node types are presented via the iterator.
          * The available set of constants is defined in the DOM_NodeFilter interface.
          *
          */
        unsigned long       getWhatToShow();

        /**
          * Return The filter used to screen nodes.
          *
          */
        DOM_NodeFilter*     getFilter();

        /**
          * Return the expandEntityReferences flag.
          * The value of this flag determines whether the children of entity reference
          * nodes are visible to the DOM_NodeFilter. If false, they will be skipped over.
          *
          */
        bool getExpandEntityReferences();

        /**
          * Returns the next node in the set and advances the position of the iterator
          * in the set. After a DOM_NodeIterator is created, the first call to nextNode()
          * returns the first node in the set.
          *
          * @exception DOMException
          *   INVALID_STATE_ERR: Raised if this method is called after the
          *   <code>detach</code> method was invoked.
          */
        DOM_Node            nextNode();

        /**
          * Returns the previous node in the set and moves the position of the iterator
          * backwards in the set.
          *
          * @exception DOMException
          *   INVALID_STATE_ERR: Raised if this method is called after the
          *   <code>detach</code> method was invoked.
          */
        DOM_Node            previousNode();
        //@}

        /** @name Detaching functions. */
        //@{
        /**
          * Detaches the iterator from the set which it iterated over, releasing any
          * computational resources and placing the iterator in the INVALID state. After
          * <code>detach</code> has been invoked, calls to <code>nextNode</code> or
          * <code>previousNode</code> will raise the exception INVALID_STATE_ERR.
          *
          */
	void				detach();
        //@}

    protected:
        DOM_NodeIterator (NodeIteratorImpl* impl);

        friend class DOM_Document;

    private:
        NodeIteratorImpl*                 fImpl;
};

XERCES_CPP_NAMESPACE_END

#endif
