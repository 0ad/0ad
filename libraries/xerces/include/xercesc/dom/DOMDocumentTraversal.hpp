#ifndef DOMDocumentTraversal_HEADER_GUARD_
#define DOMDocumentTraversal_HEADER_GUARD_

/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2002 The Apache Software Foundation.  All rights
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
 * originally based on software copyright (c) 2002, International
 * Business Machines, Inc., http://www.ibm.com .  For more information
 * on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * $Id: DOMDocumentTraversal.hpp,v 1.6 2003/03/07 19:59:02 tng Exp $
*/

#include <xercesc/util/XercesDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN


class DOMNode;
class DOMNodeFilter;
class DOMNodeIterator;
class DOMTreeWalker;


/**
 * <code>DOMDocumentTraversal</code> contains methods that create
 * <code>DOMNodeIterators</code> and <code>DOMTreeWalkers</code> to traverse a
 * node and its children in document order (depth first, pre-order
 * traversal, which is equivalent to the order in which the start tags occur
 * in the text representation of the document). In DOMs which support the
 * Traversal feature, <code>DOMDocumentTraversal</code> will be implemented by
 * the same objects that implement the DOMDocument interface.
 * <p>See also the <a href='http://www.w3.org/TR/2000/REC-DOM-Level-2-Traversal-Range-20001113'>Document Object Model (DOM) Level 2 Traversal and Range Specification</a>.
 * @since DOM Level 2
 */
class CDOM_EXPORT DOMDocumentTraversal {

protected:
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    /** @name Hidden constructors */
    //@{    
    DOMDocumentTraversal() {};
    //@}

private:
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    /** @name Unimplemented constructors and operators */
    //@{
    DOMDocumentTraversal(const DOMDocumentTraversal &);
    DOMDocumentTraversal & operator = (const DOMDocumentTraversal &);
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
    virtual ~DOMDocumentTraversal() {};
    //@}

    // -----------------------------------------------------------------------
    //  Virtual DOMDocumentRange interface
    // -----------------------------------------------------------------------
    /** @name Functions introduced in DOM Level 2 */
    //@{
    /**
     * Creates a NodeIterator object.   (DOM2)
     *
     * NodeIterators are used to step through a set of nodes, e.g. the set of nodes in a NodeList, the
     * document subtree governed by a particular node, the results of a query, or any other set of nodes.
     * The set of nodes to be iterated is determined by the implementation of the NodeIterator. DOM Level 2
     * specifies a single NodeIterator implementation for document-order traversal of a document subtree.
     * Instances of these iterators are created by calling <code>DOMDocumentTraversal.createNodeIterator()</code>.
     *
     * To produce a view of the document that has entity references expanded and does not
     * expose the entity reference node itself, use the <code>whatToShow</code> flags to hide the entity
     * reference node and set expandEntityReferences to true when creating the iterator. To
     * produce a view of the document that has entity reference nodes but no entity expansion,
     * use the <code>whatToShow</code> flags to show the entity reference node and set
     * expandEntityReferences to false.
     *
     * @param root The root node of the DOM tree
     * @param whatToShow This attribute determines which node types are presented via the iterator.
     * @param filter The filter used to screen nodes
     * @param entityReferenceExpansion The value of this flag determines whether the children of entity reference nodes are
     *                   visible to the iterator. If false, they will be skipped over.
     * @since DOM Level 2
     */

    virtual DOMNodeIterator *createNodeIterator(DOMNode         *root,
                                                   unsigned long    whatToShow,
                                                   DOMNodeFilter* filter,
                                                   bool             entityReferenceExpansion) = 0;
    /**
     * Creates a TreeWalker object.   (DOM2)
     *
     * TreeWalker objects are used to navigate a document tree or subtree using the view of the document defined
     * by its whatToShow flags and any filters that are defined for the TreeWalker. Any function which performs
     * navigation using a TreeWalker will automatically support any view defined by a TreeWalker.
     *
     * Omitting nodes from the logical view of a subtree can result in a structure that is substantially different from
     * the same subtree in the complete, unfiltered document. Nodes that are siblings in the TreeWalker view may
     * be children of different, widely separated nodes in the original view. For instance, consider a Filter that skips
     * all nodes except for DOMText nodes and the root node of a document. In the logical view that results, all text
     * nodes will be siblings and appear as direct children of the root node, no matter how deeply nested the
     * structure of the original document.
     *
     * To produce a view of the document that has entity references expanded
     * and does not expose the entity reference node itself, use the whatToShow
     * flags to hide the entity reference node and set <code>expandEntityReferences</code> to
     * true when creating the TreeWalker. To produce a view of the document
     * that has entity reference nodes but no entity expansion, use the
     * <code>whatToShow</code> flags to show the entity reference node and set
     * <code>expandEntityReferences</code> to false
     *
     * @param root The root node of the DOM tree
     * @param whatToShow This attribute determines which node types are presented via the tree-walker.
     * @param filter The filter used to screen nodes
     * @param entityReferenceExpansion The value of this flag determines whether the children of entity reference nodes are
     *                   visible to the tree-walker. If false, they will be skipped over.
     * @since DOM Level 2
     */

    virtual DOMTreeWalker  *createTreeWalker(DOMNode        *root,
                                               unsigned long     whatToShow,
                                               DOMNodeFilter  *filter,
                                               bool              entityReferenceExpansion) = 0;

    //@}
};


XERCES_CPP_NAMESPACE_END

#endif
