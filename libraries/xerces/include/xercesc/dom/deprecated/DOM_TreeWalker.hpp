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
 * $Id: DOM_TreeWalker.hpp,v 1.3 2002/11/04 15:04:44 tng Exp $
 */

#ifndef DOM_TreeWalker_HEADER_GUARD_
#define DOM_TreeWalker_HEADER_GUARD_

#include "DOM_Node.hpp"
#include "DOM_NodeFilter.hpp"

XERCES_CPP_NAMESPACE_BEGIN


class TreeWalkerImpl;


/**
 * <code>DOM_TreeWalker</code> objects are used to navigate a document tree or
 * subtree using the view of the document defined by its <code>whatToShow</code>
 * flags and any filters that are defined for the <code>DOM_TreeWalker</code>. Any
 * function which performs navigation using a <code>DOM_TreeWalker</code> will
 * automatically support any view defined by a <code>DOM_TreeWalker</code>.
 *
 * Omitting nodes from the logical view of a subtree can result in a structure that is
 * substantially different from the same subtree in the complete, unfiltered document. Nodes
 * that are siblings in the DOM_TreeWalker view may be children of different, widely separated
 * nodes in the original view. For instance, consider a Filter that skips all nodes except for
 * Text nodes and the root node of a document. In the logical view that results, all text
 * nodes will be siblings and appear as direct children of the root node, no matter how
 * deeply nested the structure of the original document.
 *
 */
class CDOM_EXPORT DOM_TreeWalker {
    public:
        /** @name Constructors and assignment operator */
        //@{
        /**
          * Default constructor.
          */
        DOM_TreeWalker();

        /**
          * Copy constructor.
          *
          * @param other The object to be copied.
          */
        DOM_TreeWalker(const DOM_TreeWalker &other);

        /**
          * Assignment operator.
          *
          * @param other The object to be copied.
          */
        DOM_TreeWalker & operator = (const DOM_TreeWalker &other);

        /**
          * Assignment operator.  This overloaded variant is provided for
          *   the sole purpose of setting a DOM_NodeIterator to null.
          *
          * @param val  Only a value of 0, or null, is allowed.
          */
        DOM_TreeWalker & operator = (const DOM_NullPtr *val);
        //@}

        /** @name Destructor. */
        //@{
	/**
	  * Destructor for DOM_TreeWalker.
	  */
        ~DOM_TreeWalker();
        //@}

        /** @name Equality and Inequality operators. */
        //@{
        /**
         * The equality operator.
         *
         * @param other The object reference with which <code>this</code> object is compared
         * @returns True if both <code>DOM_TreeWalker</code>s refer to the same
         *  actual node, or are both null; return false otherwise.
         */
        bool operator == (const DOM_TreeWalker & other)const;

        /**
          *  Compare with a pointer.  Intended only to allow a convenient
          *    comparison with null.
          */
        bool operator == (const DOM_NullPtr *other) const;

        /**
         * The inequality operator.  See operator ==.
         */
        bool operator != (const DOM_TreeWalker & other) const;

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
         * The <code>root</code> node of the <code>TreeWalker</code>, as specified
         * when it was created.
         */
        DOM_Node          getRoot();

        /**
          * Return which node types are presented via the DOM_TreeWalker.
          * These constants are defined in the DOM_NodeFilter interface.
          *
          */
        unsigned long   	getWhatToShow();

        /**
          * Return The filter used to screen nodes.
          *
          */
        DOM_NodeFilter*		getFilter();

        /**
          * Return the expandEntityReferences flag.
          * The value of this flag determines whether the children of entity reference
          * nodes are visible to the DOM_TreeWalker. If false, they will be skipped over.
          *
          */
        bool getExpandEntityReferences();

        /**
          * Return the node at which the DOM_TreeWalker is currently positioned.
          *
          */
        DOM_Node		getCurrentNode();

        /**
          * Moves to and returns the closest visible ancestor node of the current node.
          * If the search for parentNode attempts to step upward from the DOM_TreeWalker's root
          * node, or if it fails to find a visible ancestor node, this method retains the
          * current position and returns null.
          *
          */
        DOM_Node		parentNode();

        /**
          * Moves the <code>DOM_TreeWalker</code> to the first child of the current node,
          * and returns the new node. If the current node has no children, returns
          * <code>null</code>, and retains the current node.
          *
          */
        DOM_Node		firstChild();

        /**
          * Moves the <code>DOM_TreeWalker</code> to the last child of the current node, and
          * returns the new node. If the current node has no children, returns
          * <code>null</code>, and retains the current node.
          *
          */
        DOM_Node		lastChild();

        /**
          * Moves the <code>DOM_TreeWalker</code> to the previous sibling of the current
          * node, and returns the new node. If the current node has no previous sibling,
          * returns <code>null</code>, and retains the current node.
          *
          */
        DOM_Node		previousSibling();

        /**
          * Moves the <code>DOM_TreeWalker</code> to the next sibling of the current node,
          * and returns the new node. If the current node has no next sibling, returns
          * <code>null</code>, and retains the current node.
          *
          */
        DOM_Node		nextSibling();

        /**
          * Moves the <code>DOM_TreeWalker</code> to the previous visible node in document
          * order relative to the current node, and returns the new node. If the current
          * node has no previous node,
          * or if the search for previousNode attempts to step upward from the DOM_TreeWalker's
          * root node, returns <code>null</code>, and retains the current node.
          *
          */
        DOM_Node		previousNode();

        /**
          * Moves the <code>DOM_TreeWalker</code> to the next visible node in document order
          * relative to the current node, and returns the new node. If the current node has
          * no next node,
          * or if the search for nextNode attempts to step upward from the DOM_TreeWalker's
          * root node, returns <code>null</code>, and retains the current node.
          *
          */
        DOM_Node		nextNode();
        //@}

        /** @name Set functions. */
        //@{
        /**
          * Set the node at which the DOM_TreeWalker is currently positioned.
          *
          */
        void			setCurrentNode(DOM_Node currentNode);
        //@}


    protected:
        DOM_TreeWalker(TreeWalkerImpl* impl);

        friend class DOM_Document;

    private:
        TreeWalkerImpl*         fImpl;
};

XERCES_CPP_NAMESPACE_END

#endif
