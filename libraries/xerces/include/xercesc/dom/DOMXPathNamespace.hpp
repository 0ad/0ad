#ifndef DOMXPathNamespace_HEADER_GUARD_
#define DOMXPathNamespace_HEADER_GUARD_

/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2001-2003 The Apache Software Foundation.  All rights
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

#include "DOMNode.hpp"

XERCES_CPP_NAMESPACE_BEGIN

class DOMElement;

/**
 * The <code>DOMXPathNamespace</code> interface is returned by <code>DOMXPathResult</code>
 * interfaces to represent the XPath namespace node type that DOM lacks. There is no
 * public constructor for this node type. Attempts to place it into a hierarchy or a 
 * NamedNodeMap result in a DOMException with the code HIERARCHY_REQUEST_ERR. This node
 * is read only, so methods or setting of attributes that would mutate the node result 
 * in a <code>DOMException</code> with the code NO_MODIFICATION_ALLOWED_ERR.
 * The core specification describes attributes of the <code>DOMNode</code> interface that 
 * are different for different node types but does not describe XPATH_NAMESPACE_NODE, 
 * so here is a description of those attributes for this node type. All attributes of
 * <code>DOMNode</code> not described in this section have a null or false value.
 * ownerDocument matches the ownerDocument of the ownerElement even if the element is later adopted.
 * nodeName is always the string "#namespace".
 * prefix is the prefix of the namespace represented by the node.
 * localName is the same as prefix.
 * nodeType is equal to XPATH_NAMESPACE_NODE.
 * namespaceURI is the namespace URI of the namespace represented by the node.
 * nodeValue is the same as namespaceURI.
 * adoptNode, cloneNode, and importNode fail on this node type by raising a DOMException with the code NOT_SUPPORTED_ERR.
 * Note: In future versions of the XPath specification, the definition of a namespace node may
 * be changed incomatibly, in which case incompatible changes to field values may be required to
 * implement versions beyond XPath 1.0.
 * @since DOM Level 3
 */
class CDOM_EXPORT DOMXPathNamespace : public DOMNode
{

protected:
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    /** @name Hidden constructors */
    //@{    
    DOMXPathNamespace() {};
    //@}

private:
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    /** @name Unimplemented constructors and operators */
    //@{
    DOMXPathNamespace(const DOMXPathNamespace &);
    DOMXPathNamespace& operator = (const  DOMXPathNamespace&);
    //@}

public:

    
    enum XPathNodeType {
        XPATH_NAMESPACE_NODE = 13
    };

    // -----------------------------------------------------------------------
    //  All constructors are hidden, just the destructor is available
    // -----------------------------------------------------------------------
    /** @name Destructor */
    //@{
    /**
     * Destructor
     *
     */
    virtual ~DOMXPathNamespace() {};
    //@}

    // -----------------------------------------------------------------------
    // Virtual DOMDocument interface
    // -----------------------------------------------------------------------
    /** @name Functions introduced in DOM Level 3 */
    //@{
    /**
     * The <code>DOMElement</code> on which the namespace was in scope when 
     * it was requested. This does not change on a returned namespace node
     * even if the document changes such that the namespace goes out of
     * scope on that element and this node is no longer found there by XPath.
     * @since DOM Level 3
     */
    virtual DOMElement     *getOwnerElement() const = 0;

    //@}
};

XERCES_CPP_NAMESPACE_END

#endif
