#ifndef DOMXPathExpression_HEADER_GUARD_
#define DOMXPathExpression_HEADER_GUARD_

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

#include <xercesc/util/XercesDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class DOMNode;
/**
 * The <code>DOMXPathExpression</code> interface represents a parsed and resolved XPath expression.
 * @since DOM Level 3
 */
class CDOM_EXPORT DOMXPathExpression
{

protected:
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    /** @name Hidden constructors */
    //@{    
    DOMXPathExpression() {};
    //@}

private:
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    /** @name Unimplemented constructors and operators */
    //@{
    DOMXPathExpression(const DOMXPathExpression &);
    DOMXPathExpression& operator = (const  DOMXPathExpression&);
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
    virtual ~DOMXPathExpression() {};
    //@}

    // -----------------------------------------------------------------------
    // Virtual DOMDocument interface
    // -----------------------------------------------------------------------
    /** @name Functions introduced in DOM Level 3 */
    //@{  

    /**
     * Evaluates this XPath expression and returns a result.
     * @param contextNode of type <code>DOMNode</code> The context is context
     * node for the evaluation of this XPath expression.
     * If the XPathEvaluator was obtained by casting the Document then this must 
     * be owned by the same document and must be a <code>DOMDocument</code>, <code>DOMElement</code>, 
     * <code>DOMAttribute</code>, <code>DOMText</code>, <code>DOMCDATASection</code>, 
     * <code>DOMComment</code>, <code>DOMProcessingInstruction</code>, or 
     * <code>XPathNamespace</code>. If the context node is a <code>DOMText</code> or a 
     * <code>DOMCDATASection</code>, then the context is interpreted as the whole logical
     * text node as seen by XPath, unless the node is empty in which case it may not
     * serve as the XPath context.
     * @param type of type unsigned short If a specific type is specified, then the result
     * will be coerced to return the specified type relying on XPath conversions and fail 
     * if the desired coercion is not possible. This must be one of the type codes of <code>XPathResult</code>.
     * @param result of type void* The result specifies a specific result object which
     * may be reused and returned by this method. If this is specified as nullor the 
     * implementation does not reuse the specified result, a new result object will be constructed
     * and returned.
     * For XPath 1.0 results, this object will be of type <code>XPathResult</code>.
     * @return void* The result of the evaluation of the XPath expression.
     * For XPath 1.0 results, this object will be of type <code>XPathResult</code>.
     * @exception <code>XPathException</code>
     * TYPE_ERR: Raised if the result cannot be converted to return the specified type.
     * @exception <code>DOMException</code>
     * WRONG_DOCUMENT_ERR: The <code>DOMNode</code> is from a document that is not supported by 
     * the <code>XPathEvaluator</code> that created this <code>DOMXPathExpression</code>.
     * NOT_SUPPORTED_ERR: The Node is not a type permitted as an XPath context node or the 
     * request type is not permitted by this <code>DOMXPathExpression</code>.
     */

    virtual void*          evaluate(DOMNode *contextNode, unsigned short type, void* result) const = 0;
    //@}
};

XERCES_CPP_NAMESPACE_END

#endif
