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
 * $Id: DOM_NodeIterator.hpp,v 1.3 2002/11/04 15:04:44 tng Exp $
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
class CDOM_EXPORT DOM_NodeIterator
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
