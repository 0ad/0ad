#ifndef DOMText_HEADER_GUARD_
#define DOMText_HEADER_GUARD_

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
 * $Id: DOMText.hpp,v 1.8 2003/03/07 19:59:09 tng Exp $
 */

#include <xercesc/util/XercesDefs.hpp>
#include "DOMCharacterData.hpp"

XERCES_CPP_NAMESPACE_BEGIN


/**
 * The <code>DOMText</code> interface inherits from <code>DOMCharacterData</code>
 * and represents the textual content (termed character data in XML) of an
 * <code>DOMElement</code> or <code>DOMAttr</code>. If there is no markup inside
 * an element's content, the text is contained in a single object
 * implementing the <code>DOMText</code> interface that is the only child of
 * the element. If there is markup, it is parsed into the information items
 * (elements, comments, etc.) and <code>DOMText</code> nodes that form the list
 * of children of the element.
 * <p>When a document is first made available via the DOM, there is only one
 * <code>DOMText</code> node for each block of text. Users may create adjacent
 * <code>DOMText</code> nodes that represent the contents of a given element
 * without any intervening markup, but should be aware that there is no way
 * to represent the separations between these nodes in XML or HTML, so they
 * will not (in general) persist between DOM editing sessions. The
 * <code>normalize()</code> method on <code>DOMNode</code> merges any such
 * adjacent <code>DOMText</code> objects into a single node for each block of
 * text.
 * <p>See also the <a href='http://www.w3.org/TR/2000/REC-DOM-Level-2-Core-20001113'>Document Object Model (DOM) Level 2 Core Specification</a>.
 */
class CDOM_EXPORT DOMText: public DOMCharacterData {
protected:
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    /** @name Hidden constructors */
    //@{    
    DOMText() {};
    //@}

private:
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    /** @name Unimplemented constructors and operators */
    //@{
    DOMText(const DOMText &);
    DOMText & operator = (const DOMText &);
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
    virtual ~DOMText() {};
    //@}

    // -----------------------------------------------------------------------
    //  Virtual DOMText interface
    // -----------------------------------------------------------------------
    /** @name Functions introduced in DOM Level 1 */
    //@{
    /**
     * Breaks this node into two nodes at the specified <code>offset</code>,
     * keeping both in the tree as siblings. After being split, this node
     * will contain all the content up to the <code>offset</code> point. A
     * new node of the same type, which contains all the content at and
     * after the <code>offset</code> point, is returned. If the original
     * node had a parent node, the new node is inserted as the next sibling
     * of the original node. When the <code>offset</code> is equal to the
     * length of this node, the new node has no data.
     * @param offset The 16-bit unit offset at which to split, starting from
     *   <code>0</code>.
     * @return The new node, of the same type as this node.
     * @exception DOMException
     *   INDEX_SIZE_ERR: Raised if the specified offset is negative or greater
     *   than the number of 16-bit units in <code>data</code>.
     *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
     * @since DOM Level 1
     */
    virtual DOMText *splitText(XMLSize_t offset) = 0;
    //@}

    /** @name Functions introduced in DOM Level 3 */
    //@{
    /**
     * Returns whether this text node contains whitespace in element content,
     * often abusively called "ignorable whitespace".  An implementation can
     * only return <code>true</code> if, one way or another, it has access
     * to the relevant information (e.g., the DTD or schema).
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * <br> This attribute represents the property [element content
     * whitespace] defined in .
     * @since DOM Level 3
     */
    virtual bool     getIsWhitespaceInElementContent() const = 0;

    /**
     * Returns all text of <code>DOMText</code> nodes logically-adjacent text
     * nodes to this node, concatenated in document order.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @since DOM Level 3
     */
    virtual const XMLCh* getWholeText() = 0;

    /**
     * Substitutes the a specified text for the text of the current node and
     * all logically-adjacent text nodes.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * <br>This method returns the node in the hierarchy which received the
     * replacement text, which is null if the text was empty or is the
     * current node if the current node is not read-only or otherwise is a
     * new node of the same type as the current node inserted at the site of
     * the replacement. All logically-adjacent text nodes are removed
     * including the current node unless it was the recipient of the
     * replacement text.
     * <br>Where the nodes to be removed are read-only descendants of an
     * <code>DOMEntityReference</code>, the <code>DOMEntityReference</code> must
     * be removed instead of the read-only nodes. If any
     * <code>DOMEntityReference</code> to be removed has descendants that are
     * not <code>DOMEntityReference</code>, <code>DOMText</code>, or
     * <code>DOMCDATASection</code> nodes, the <code>replaceWholeText</code>
     * method must fail before performing any modification of the document,
     * raising a <code>DOMException</code> with the code
     * <code>NO_MODIFICATION_ALLOWED_ERR</code>.
     * @param content The content of the replacing <code>DOMText</code> node.
     * @return The <code>DOMText</code> node created with the specified content.
     * @exception DOMException
     *   NO_MODIFICATION_ALLOWED_ERR: Raised if one of the <code>DOMText</code>
     *   nodes being replaced is readonly.
     * @since DOM Level 3
     */
    virtual DOMText* replaceWholeText(const XMLCh* content) = 0;
    //@}

    // -----------------------------------------------------------------------
    // Non-standard extension
    // -----------------------------------------------------------------------
    /** @name Non-standard extension */
    //@{
    /**
     * Non-standard extension
     *
     * Return true if this node contains ignorable whitespaces only.
     * @return True if this node contains ignorable whitespaces only.
     */
    virtual bool isIgnorableWhitespace() const = 0;
    //@}

};


XERCES_CPP_NAMESPACE_END

#endif


