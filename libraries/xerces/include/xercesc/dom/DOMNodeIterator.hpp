#ifndef DOMNodeIterator_HEADER_GUARD_
#define DOMNodeIterator_HEADER_GUARD_

/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2001-2002 The Apache Software Foundation.  All rights
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
 * originally based on software copyright (c) 2001, International
 * Business Machines, Inc., http://www.ibm.com .  For more information
 * on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * $Id: DOMNodeIterator.hpp,v 1.7 2003/03/07 19:59:07 tng Exp $
 */

#include "DOMNodeFilter.hpp"
#include "DOMNode.hpp"

XERCES_CPP_NAMESPACE_BEGIN


/**
 * <code>DOMNodeIterators</code> are used to step through a set of nodes, e.g.
 * the set of nodes in a <code>DOMNodeList</code>, the document subtree
 * governed by a particular <code>DOMNode</code>, the results of a query, or
 * any other set of nodes. The set of nodes to be iterated is determined by
 * the implementation of the <code>DOMNodeIterator</code>. DOM Level 2
 * specifies a single <code>DOMNodeIterator</code> implementation for
 * document-order traversal of a document subtree. Instances of these
 * <code>DOMNodeIterators</code> are created by calling
 * <code>DOMDocumentTraversal</code><code>.createNodeIterator()</code>.
 * <p>See also the <a href='http://www.w3.org/TR/2000/REC-DOM-Level-2-Traversal-Range-20001113'>Document Object Model (DOM) Level 2 Traversal and Range Specification</a>.
 * @since DOM Level 2
 */
class CDOM_EXPORT DOMNodeIterator
{
protected:
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    /** @name Hidden constructors */
    //@{    
    DOMNodeIterator() {};
    //@}

private:    
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    /** @name Unimplemented constructors and operators */
    //@{
    DOMNodeIterator(const DOMNodeIterator &);
    DOMNodeIterator & operator = (const DOMNodeIterator &);
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
    virtual ~DOMNodeIterator() {};
    //@}

    // -----------------------------------------------------------------------
    //  Virtual DOMNodeFilter interface
    // -----------------------------------------------------------------------
    /** @name Functions introduced in DOM Level 2 */
    //@{
    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    /**
     * The <code>root</code> node of the <code>DOMNodeIterator</code>, as specified
     * when it was created.
     * @since DOM Level 2
     */
    virtual DOMNode*           getRoot() = 0;
    /**
     * Return which node types are presented via the iterator.
     * This attribute determines which node types are presented via the
     * <code>DOMNodeIterator</code>. The available set of constants is defined
     * in the <code>DOMNodeFilter</code> interface.  Nodes not accepted by
     * <code>whatToShow</code> will be skipped, but their children may still
     * be considered. Note that this skip takes precedence over the filter,
     * if any.
     * @since DOM Level 2
     *
     */
    virtual unsigned long      getWhatToShow() = 0;

    /**
     * The <code>DOMNodeFilter</code> used to screen nodes.
     *
     * @since DOM Level 2
     */
    virtual DOMNodeFilter*     getFilter() = 0;

    /**
     * Return the expandEntityReferences flag.
     * The value of this flag determines whether the children of entity
     * reference nodes are visible to the <code>DOMNodeIterator</code>. If
     * false, these children  and their descendants will be rejected. Note
     * that this rejection takes precedence over <code>whatToShow</code> and
     * the filter. Also note that this is currently the only situation where
     * <code>DOMNodeIterators</code> may reject a complete subtree rather than
     * skipping individual nodes.
     * <br>
     * <br> To produce a view of the document that has entity references
     * expanded and does not expose the entity reference node itself, use
     * the <code>whatToShow</code> flags to hide the entity reference node
     * and set <code>expandEntityReferences</code> to true when creating the
     * <code>DOMNodeIterator</code>. To produce a view of the document that has
     * entity reference nodes but no entity expansion, use the
     * <code>whatToShow</code> flags to show the entity reference node and
     * set <code>expandEntityReferences</code> to false.
     *
     * @since DOM Level 2
     */
    virtual bool               getExpandEntityReferences() = 0;

    // -----------------------------------------------------------------------
    //  Query methods
    // -----------------------------------------------------------------------
    /**
     * Returns the next node in the set and advances the position of the
     * <code>DOMNodeIterator</code> in the set. After a
     * <code>DOMNodeIterator</code> is created, the first call to
     * <code>nextNode()</code> returns the first node in the set.
     * @return The next <code>DOMNode</code> in the set being iterated over, or
     *   <code>null</code> if there are no more members in that set.
     * @exception DOMException
     *   INVALID_STATE_ERR: Raised if this method is called after the
     *   <code>detach</code> method was invoked.
     * @since DOM Level 2
     */
    virtual DOMNode*           nextNode() = 0;

    /**
     * Returns the previous node in the set and moves the position of the
     * <code>DOMNodeIterator</code> backwards in the set.
     * @return The previous <code>DOMNode</code> in the set being iterated over,
     *   or <code>null</code> if there are no more members in that set.
     * @exception DOMException
     *   INVALID_STATE_ERR: Raised if this method is called after the
     *   <code>detach</code> method was invoked.
     * @since DOM Level 2
     */
    virtual DOMNode*           previousNode() = 0;

    /**
     * Detaches the <code>DOMNodeIterator</code> from the set which it iterated
     * over, releasing any computational resources and placing the
     * <code>DOMNodeIterator</code> in the INVALID state. After
     * <code>detach</code> has been invoked, calls to <code>nextNode</code>
     * or <code>previousNode</code> will raise the exception
     * INVALID_STATE_ERR.
     * @since DOM Level 2
     */
    virtual void               detach() = 0;
    //@}

    // -----------------------------------------------------------------------
    //  Non-standard Extension
    // -----------------------------------------------------------------------
    /** @name Non-standard Extension */
    //@{
    /**
     * Called to indicate that this NodeIterator is no longer in use
     * and that the implementation may relinquish any resources associated with it.
     * (release() will call detach() where appropriate)
     *
     * Access to a released object will lead to unexpected result.
     */
    virtual void               release() = 0;
    //@}
};

XERCES_CPP_NAMESPACE_END

#endif
