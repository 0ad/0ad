#ifndef DOMNodeFilter_HEADER_GUARD_
#define DOMNodeFilter_HEADER_GUARD_

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
 * $Id: DOMNodeFilter.hpp,v 1.6 2003/03/07 19:59:06 tng Exp $
 */

#include "DOMNode.hpp"

XERCES_CPP_NAMESPACE_BEGIN


/**
 * Filters are objects that know how to "filter out" nodes. If a
 * <code>DOMNodeIterator</code> or <code>DOMTreeWalker</code> is given a
 * <code>DOMNodeFilter</code>, it applies the filter before it returns the next
 * node. If the filter says to accept the node, the traversal logic returns
 * it; otherwise, traversal looks for the next node and pretends that the
 * node that was rejected was not there.
 * <p>The DOM does not provide any filters. <code>DOMNodeFilter</code> is just an
 * interface that users can implement to provide their own filters.
 * <p><code>DOMNodeFilters</code> do not need to know how to traverse from node
 * to node, nor do they need to know anything about the data structure that
 * is being traversed. This makes it very easy to write filters, since the
 * only thing they have to know how to do is evaluate a single node. One
 * filter may be used with a number of different kinds of traversals,
 * encouraging code reuse.
 * <p>See also the <a href='http://www.w3.org/TR/2000/REC-DOM-Level-2-Traversal-Range-20001113'>Document Object Model (DOM) Level 2 Traversal and Range Specification</a>.
 * @since DOM Level 2
 */

class CDOM_EXPORT DOMNodeFilter
{
protected:
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    /** @name Hidden constructors */
    //@{    
    DOMNodeFilter() {};
    //@}    

private:
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    /** @name Unimplemented constructors and operators */
    //@{
    DOMNodeFilter(const DOMNodeFilter &);
    DOMNodeFilter & operator = (const DOMNodeFilter &);
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
    virtual ~DOMNodeFilter() {};
    //@}

    // -----------------------------------------------------------------------
    //  Class Types
    // -----------------------------------------------------------------------
    /** @name Public Contants */
    //@{
    /**
     * Constants returned by acceptNode.
     *
     * <p><code>FILTER_ACCEPT:</code>
     * Accept the node. Navigation methods defined for
     * <code>DOMNodeIterator</code> or <code>DOMTreeWalker</code> will return this
     * node.</p>
     *
     * <p><code>FILTER_REJECT:</code>
     * Reject the node. Navigation methods defined for
     * <code>DOMNodeIterator</code> or <code>DOMTreeWalker</code> will not return
     * this node. For <code>DOMTreeWalker</code>, the children of this node
     * will also be rejected. <code>DOMNodeIterators</code> treat this as a
     * synonym for <code>FILTER_SKIP.</code></p>
     *
     * <p><code>FILTER_SKIP:</code>
     * Skip this single node. Navigation methods defined for
     * <code>DOMNodeIterator</code> or <code>DOMTreeWalker</code> will not return
     * this node. For both <code>DOMNodeIterator</code> and
     * <code>DOMTreeWalker</code>, the children of this node will still be
     * considered.</p>
     *
     * @since DOM Level 2
     */
    enum FilterAction {FILTER_ACCEPT = 1,
                       FILTER_REJECT = 2,
                       FILTER_SKIP   = 3};

    /**
     * Constants for whatToShow
     *
     * <p><code>SHOW_ALL:</code>
     * Show all <code>DOMNode(s)</code>.</p>
     *
     * <p><code>SHOW_ELEMENT:</code>
     * Show <code>DOMElement</code> nodes.</p>
     *
     * <p><code>SHOW_ATTRIBUTE:</code>
     * Show <code>DOMAttr</code> nodes. This is meaningful only when creating an
     * <code>DOMNodeIterator</code> or <code>DOMTreeWalker</code> with an
     * attribute node as its <code>root</code>; in this case, it means that
     * the attribute node will appear in the first position of the iteration
     * or traversal. Since attributes are never children of other nodes,
     * they do not appear when traversing over the document tree.</p>
     *
     * <p><code>SHOW_TEXT:</code>
     * Show <code>DOMText</code> nodes.</p>
     *
     * <p><code>SHOW_CDATA_SECTION:</code>
     * Show <code>DOMCDATASection</code> nodes.</p>
     *
     * <p><code>SHOW_ENTITY_REFERENCE:</code>
     * Show <code>DOMEntityReference</code> nodes.</p>
     *
     * <p><code>SHOW_ENTITY:</code>
     * Show <code>DOMEntity</code> nodes. This is meaningful only when creating
     * an <code>DOMNodeIterator</code> or <code>DOMTreeWalker</code> with an
     * <code>DOMEntity</code> node as its <code>root</code>; in this case, it
     * means that the <code>DOMEntity</code> node will appear in the first
     * position of the traversal. Since entities are not part of the
     * document tree, they do not appear when traversing over the document
     * tree.</p>
     *
     * <p><code>SHOW_PROCESSING_INSTRUCTION:</code>
     * Show <code>DOMProcessingInstruction</code> nodes.</p>
     *
     * <p><code>SHOW_COMMENT:</code>
     * Show <code>DOMComment</code> nodes.</p>
     *
     * <p><code>SHOW_DOCUMENT:</code>
     * Show <code>DOMDocument</code> nodes.</p>
     *
     * <p><code>SHOW_DOCUMENT_TYPE:</code>
     * Show <code>DOMDocumentType</code> nodes.</p>
     *
     * <p><code>SHOW_DOCUMENT_FRAGMENT:</code>
     * Show <code>DOMDocumentFragment</code> nodes.</p>
     *
     * <p><code>SHOW_NOTATION:</code>
     * Show <code>DOMNotation</code> nodes. This is meaningful only when creating
     * an <code>DOMNodeIterator</code> or <code>DOMTreeWalker</code> with a
     * <code>DOMNotation</code> node as its <code>root</code>; in this case, it
     * means that the <code>DOMNotation</code> node will appear in the first
     * position of the traversal. Since notations are not part of the
     * document tree, they do not appear when traversing over the document
     * tree.</p>
     *
     * @since DOM Level 2
     */
    enum ShowType {
        SHOW_ALL                       = 0x0000FFFF,
        SHOW_ELEMENT                   = 0x00000001,
        SHOW_ATTRIBUTE                 = 0x00000002,
        SHOW_TEXT                      = 0x00000004,
        SHOW_CDATA_SECTION             = 0x00000008,
        SHOW_ENTITY_REFERENCE          = 0x00000010,
        SHOW_ENTITY                    = 0x00000020,
        SHOW_PROCESSING_INSTRUCTION    = 0x00000040,
        SHOW_COMMENT                   = 0x00000080,
        SHOW_DOCUMENT                  = 0x00000100,
        SHOW_DOCUMENT_TYPE             = 0x00000200,
        SHOW_DOCUMENT_FRAGMENT         = 0x00000400,
        SHOW_NOTATION                  = 0x00000800
    };
    //@}

    // -----------------------------------------------------------------------
    //  Virtual DOMNodeFilter interface
    // -----------------------------------------------------------------------
    /** @name Functions introduced in DOM Level 2 */
    //@{
    /**
     * Test whether a specified node is visible in the logical view of a
     * <code>DOMTreeWalker</code> or <code>DOMNodeIterator</code>. This function
     * will be called by the implementation of <code>DOMTreeWalker</code> and
     * <code>DOMNodeIterator</code>; it is not normally called directly from
     * user code. (Though you could do so if you wanted to use the same
     * filter to guide your own application logic.)
     * @param node The node to check to see if it passes the filter or not.
     * @return A constant to determine whether the node is accepted,
     *   rejected, or skipped, as defined above.
     * @since DOM Level 2
     */
    virtual short acceptNode (const DOMNode* node) const =0;
    //@}

};

XERCES_CPP_NAMESPACE_END

#endif
