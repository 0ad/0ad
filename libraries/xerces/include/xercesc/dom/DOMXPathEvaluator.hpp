#ifndef DOMXPathEvaluator_HEADER_GUARD_
#define DOMXPathEvaluator_HEADER_GUARD_

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

class DOMXPathNSResolver;
class DOMXPathExpression;
class DOMNode;

/**
 * The evaluation of XPath expressions is provided by <code>DOMXPathEvaluator</code>. 
 * In a DOM implementation which supports the XPath 3.0 feature, the <code>DOMXPathEvaluator</code>
 * interface will be implemented on the same object which implements the Document interface permitting
 * it to be obtained by casting or by using the DOM Level 3 getInterface method. In this case the
 * implementation obtained from the Document supports the XPath DOM module and is compatible 
 * with the XPath 1.0 specification.
 * Evaluation of expressions with specialized extension functions or variables may not
 * work in all implementations and is, therefore, not portable. XPathEvaluator implementations
 * may be available from other sources that could provide specific support for specialized extension
 * functions or variables as would be defined by other specifications.
 * @since DOM Level 3
 */
class CDOM_EXPORT DOMXPathEvaluator
{

protected:
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    /** @name Hidden constructors */
    //@{    
    DOMXPathEvaluator() {};
    //@}

private:
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    /** @name Unimplemented constructors and operators */
    //@{
    DOMXPathEvaluator(const DOMXPathEvaluator &);
    DOMXPathEvaluator& operator = (const  DOMXPathEvaluator&);
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
    virtual ~DOMXPathEvaluator() {};
    //@}

    // -----------------------------------------------------------------------
    // Virtual DOMDocument interface
    // -----------------------------------------------------------------------
    /** @name Functions introduced in DOM Level 3 */
    //@{

    /**
     * Creates a parsed XPath expression with resolved namespaces. This is useful
     * when an expression will be reused in an application since it makes it 
     * possible to compile the expression string into a more efficient internal 
     * form and preresolve all namespace prefixes which occur within the expression.
     * @param expression of type XMLCh - The XPath expression string to be parsed.
     * @param resolver of type <code>XPathNSResolver</code> - The resolver permits 
     * translation of all prefixes, including the xml namespace prefix, within the XPath expression
     * into appropriate namespace URIs. If this is specified as null, any namespace 
     * prefix within the expression will result in <code>DOMException</code> being thrown with the
     * code NAMESPACE_ERR.
     * @return <code>XPathExpression</code> The compiled form of the XPath expression.
     * @exception <code>XPathException</code>
     * INVALID_EXPRESSION_ERR: Raised if the expression is not legal according to the 
     * rules of the <code>DOMXPathEvaluator</code>.
     * @exception DOMException
     * NAMESPACE_ERR: Raised if the expression contains namespace prefixes which cannot
     * be resolved by the specified <code>XPathNSResolver</code>.
     * @since DOM Level 3
     */
    virtual const DOMXPathExpression*    createExpression(const XMLCh *expression, const DOMXPathNSResolver *resolver) = 0;


    /** Adapts any DOM node to resolve namespaces so that an XPath expression can be
     * easily evaluated relative to the context of the node where it appeared within
     * the document. This adapter works like the DOM Level 3 method lookupNamespaceURI
     * on nodes in resolving the namespaceURI from a given prefix using the current 
     * information available in the node's hierarchy at the time lookupNamespaceURI 
     * is called. also correctly resolving the implicit xml prefix.
     * @param nodeResolver of type <code>DOMNode</code> The node to be used as a context 
     * for namespace resolution.
     * @return <code>XPathNSResolver</code> <code>XPathNSResolver</code> which resolves namespaces 
     * with respect to the definitions in scope for a specified node.
     */
    virtual const DOMXPathNSResolver*    createNSResolver(DOMNode *nodeResolver) = 0;


    /**
     * Evaluates an XPath expression string and returns a result of the specified 
     * type if possible.
     * @param expression of type XMLCh The XPath expression string to be parsed 
     * and evaluated.
     * @param contextNode of type <code>DOMNode</code> The context is context node 
     * for the evaluation 
     * of this XPath expression. If the <code>DOMXPathEvaluator</code> was obtained by
     * casting the <code>DOMDocument</code> then this must be owned by the same 
     * document and must be a <code>DOMDocument</code>, <code>DOMElement</code>, 
     * <code>DOMAttribute</code>, <code>DOMText</code>, <code>DOMCDATASection</code>, 
     * <code>DOMComment</code>, <code>DOMProcessingInstruction</code>, or 
     * <code>XPathNamespace</code> node. If the context node is a <code>DOMText</code> or 
     * a <code>DOMCDATASection</code>, then the context is interpreted as the whole 
     * logical text node as seen by XPath, unless the node is empty in which case it
     * may not serve as the XPath context.
     * @param resolver of type <code>XPathNSResolver</code> The resolver permits 
     * translation of all prefixes, including the xml namespace prefix, within 
     * the XPath expression into appropriate namespace URIs. If this is specified 
     * as null, any namespace prefix within the expression will result in 
     * <code>DOMException</code> being thrown with the code NAMESPACE_ERR.
     * @param type of type unsigned short - If a specific type is specified, then 
     * the result will be returned as the corresponding type.
     * For XPath 1.0 results, this must be one of the codes of the <code>XPathResult</code>
     * interface.
     * @param result of type void* - The result specifies a specific result object
     * which may be reused and returned by this method. If this is specified as 
     * null or the implementation does not reuse the specified result, a new result
     * object will be constructed and returned.
     * For XPath 1.0 results, this object will be of type <code>XPathResult</code>.
     * @return void* The result of the evaluation of the XPath expression.
     * For XPath 1.0 results, this object will be of type <code>XPathResult</code>.
     * @exception <code>XPathException</code>
     * INVALID_EXPRESSION_ERR: Raised if the expression is not legal 
     * according to the rules of the <code>DOMXPathEvaluator</code>
     * TYPE_ERR: Raised if the result cannot be converted to return the specified type.
     * @exception DOMException
     * NAMESPACE_ERR: Raised if the expression contains namespace prefixes 
     * which cannot be resolved by the specified <code>XPathNSResolver</code>.
     * WRONG_DOCUMENT_ERR: The Node is from a document that is not supported 
     * by this <code>DOMXPathEvaluator</code>.
     * NOT_SUPPORTED_ERR: The Node is not a type permitted as an XPath context 
     * node or the request type is not permitted by this <code>DOMXPathEvaluator</code>.
     */
    virtual void* evaluate(const XMLCh *expression, DOMNode *contextNode, const DOMXPathNSResolver *resolver, 
                           unsigned short type, void* result) = 0;

    //@}
};

XERCES_CPP_NAMESPACE_END

#endif
