#ifndef DOMXPathResult_HEADER_GUARD_
#define DOMXPathResult_HEADER_GUARD_

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
 * The <code>DOMXPathResult</code> interface represents the result of the
 * evaluation of an XPath 1.0 expression within the context of a particular
 * node. Since evaluation of an XPath expression can result in various result
 * types, this object makes it possible to discover and manipulate the type
 * and value of the result.
 * @since DOM Level 3
 */
class CDOM_EXPORT DOMXPathResult
{

protected:
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    /** @name Hidden constructors */
    //@{    
    DOMXPathResult() {};
    //@}

private:
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    /** @name Unimplemented constructors and operators */
    //@{
    DOMXPathResult(const DOMXPathResult &);
    DOMXPathResult& operator = (const  DOMXPathResult&);
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
    virtual ~DOMXPathResult() {};
    //@}

    // -----------------------------------------------------------------------
    //  Class Types
    // -----------------------------------------------------------------------
    /** @name Public Contants */
    //@{
    /**    ANY_TYPE
     * This code does not represent a specific type. An evaluation of an XPath 
     * expression will never produce this type. If this type is requested, then 
     * the evaluation returns whatever type naturally results from evaluation 
     * of the expression.
     * If the natural result is a node set when ANY_TYPE was requested, then 
     * UNORDERED_NODE_ITERATOR_TYPE is always the resulting type. Any other 
     * representation of a node set must be explicitly requested.
     * ANY_UNORDERED_NODE_TYPE
     * The result is a node set as defined by [XPath 1.0] and will be accessed
     * as a single node, which may be nullif the node set is empty. Document
     * modification does not invalidate the node, but may mean that the result 
     * node no longer corresponds to the current document. This is a convenience
     * that permits optimization since the implementation can stop once any node 
     * in the resulting set has been found.
     * If there is more than one node in the actual result, the single node 
     * returned might not be the first in document order.
     * BOOLEAN_TYPE
     * The result is a boolean as defined by [XPath 1.0]. Document modification 
     * does not invalidate the boolean, but may mean that reevaluation would not 
     * yield the same boolean.
     * FIRST_ORDERED_NODE_TYPE
     * The result is a node set as defined by [XPath 1.0] and will be accessed
     * as a single node, which may be null if the node set is empty. Document 
     * modification does not invalidate the node, but may mean that the result 
     * node no longer corresponds to the current document. This is a convenience
     * that permits optimization since the implementation can stop once the first
     * node in document order of the resulting set has been found.
     * If there are more than one node in the actual result, the single node 
     * returned will be the first in document order.
     * NUMBER_TYPE
     * The result is a number as defined by [XPath 1.0]. Document modification does
     * not invalidate the number, but may mean that reevaluation would not yield the 
     * same number.
     * ORDERED_NODE_ITERATOR_TYPE
     * The result is a node set as defined by [XPath 1.0] that will be accessed 
     * iteratively, which will produce document-ordered nodes. Document modification 
     * invalidates the iteration.
     * ORDERED_NODE_SNAPSHOT_TYPE
     * The result is a node set as defined by [XPath 1.0] that will be accessed as a
     * snapshot list of nodes that will be in original document order. Document 
     * modification does not invalidate the snapshot but may mean that reevaluation would
     * not yield the same snapshot and nodes in the snapshot may have been altered, moved, 
     * or removed from the document.
     * STRING_TYPE
     * The result is a string as defined by [XPath 1.0]. Document modification does not
     * invalidate the string, but may mean that the string no longer corresponds to the 
     * current document.
     * UNORDERED_NODE_ITERATOR_TYPE
     * The result is a node set as defined by [XPath 1.0] that will be accessed iteratively, 
     * which may not produce nodes in a particular order. Document modification invalidates the iteration.
     * This is the default type returned if the result is a node set and ANY_TYPE is requested.
     * UNORDERED_NODE_SNAPSHOT_TYPE
     * The result is a node set as defined by [XPath 1.0] that will be accessed as a
     * snapshot list of nodes that may not be in a particular order. Document modification
     * does not invalidate the snapshot but may mean that reevaluation would not yield the same
     * snapshot and nodes in the snapshot may have been altered, moved, or removed from the document.
     */
	enum resultType {
                ANY_TYPE = 0,
                NUMBER_TYPE = 1,
                STRING_TYPE = 2,
                BOOLEAN_TYPE = 3,
                UNORDERED_NODE_ITERATOR_TYPE = 4,
                ORDERED_NODE_ITERATOR_TYPE = 5,
                UNORDERED_NODE_SNAPSHOT_TYPE = 6,
                ORDERED_NODE_SNAPSHOT_TYPE = 7,
                ANY_UNORDERED_NODE_TYPE = 8,
                FIRST_ORDERED_NODE_TYPE = 9
    };
    //@}


    // -----------------------------------------------------------------------
    // Virtual DOMDocument interface
    // -----------------------------------------------------------------------
    /** @name Functions introduced in DOM Level 3 */
    //@{


    /** 
     * Returns the boolean value of this result
     * @return booleanValue of type boolean, readonly
     * The value of this boolean result.
     * @exception XPathException
     * TYPE_ERR: raised if resultType is not BOOLEAN_TYPE.
     */
	virtual bool getBooleanValue() const = 0;

    /** 
     * Returns the state of the iterator
     * @return invalidIteratorState 
     * Signifies that the iterator has become invalid. True if resultType is 
     * UNORDERED_NODE_ITERATOR_TYPE or ORDERED_NODE_ITERATOR_TYPE and the 
     * document has been modified since this result was returned.
     * @exception XPathException 
     * TYPE_ERR: raised if resultType is not NUMBER_TYPE.
     */
	virtual bool getInvalidIteratorState() const = 0;

    /**
     * Returns the number value of this result
     * @return numberValue 
     * The value of this number result. If the native double type of the DOM 
     * binding does not directly support the exact IEEE 754 result of the XPath 
     * expression, then it is up to the definition of the binding to specify how 
     * the XPath number is converted to the native binding number.
     * @exception XPathException
     * TYPE_ERR: raised if resultType is not NUMBER_TYPE.
     */
	virtual double getNumberValue() const = 0;

    /**
     * Returns the result type of this result
     * @return resultType 
     * A code representing the type of this result, as defined by the type constants.       
     * @exception XPathException
     * TYPE_ERR: raised if resultType is not ANY_UNORDERED_NODE_TYPE or FIRST_ORDERED_NODE_TYPE.
     */
	virtual short getResultType() const = 0;
    	
    /**
     * Returns the single node value of this result
     * @return singleNodeValue 
     * The value of this single node result, which may be null.
     * @exception XPathException
     * TYPE_ERR: raised if resultType is not ANY_UNORDERED_NODE_TYPE or FIRST_ORDERED_NODE_TYPE.
     */
	virtual DOMNode *getSingleNodeValue() const = 0;

    /**
     * Returns the snapshot length
     * @return snapshotLength 
     * The number of nodes in the result snapshot. Valid values for snapshotItem indices 
     * are 0 to snapshotLength-1 inclusive.
     * @exception XPathException
     * TYPE_ERR: raised if resultType is not UNORDERED_NODE_SNAPSHOT_TYPE or 
     * ORDERED_NODE_SNAPSHOT_TYPE.        
     */
	virtual unsigned long getSnapshotLength() const = 0;
        
    /**
     * Returns the string value of this result
     * @return stringValue 
     * The value of this string result.
     * @exception XPathException
     * TYPE_ERR: raised if resultType is not STRING_TYPE.
     */
	virtual const XMLCh* getStringValue() const = 0;
    
    /**
     * Iterates and returns the next node from the node set or nullif there are no more nodes.
     * @return the next node.
     * @exception XPathException
     * TYPE_ERR: raised if resultType is not UNORDERED_NODE_ITERATOR_TYPE or ORDERED_NODE_ITERATOR_TYPE.
     * @exception DOMException
     * INVALID_STATE_ERR: The document has been mutated since the result was returned.	
     */
	virtual DOMNode* iterateNext() const = 0;

    /**
     * Returns the indexth item in the snapshot collection. If index is greater than or
     * equal to the number of nodes in the list, this method returns null. Unlike the
     * iterator result, the snapshot does not become invalid, but may not correspond 
     * to the current document if it is mutated.
     * @param index of type unsigned long - Index into the snapshot collection.
     * @return The node at the indexth position in the NodeList, or null if that is not a valid index.
     * @exception XPathException
     * TYPE_ERR: raised if resultType is not UNORDERED_NODE_SNAPSHOT_TYPE or ORDERED_NODE_SNAPSHOT_TYPE.	
     */
	virtual DOMNode* snapshotItem(unsigned long index) const = 0;

    //@}
};

XERCES_CPP_NAMESPACE_END

#endif
