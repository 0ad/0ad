#ifndef DOMSPtr_HEADER_GUARD_
#define DOMSPtr_HEADER_GUARD_

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

/*
 * $Id: StDOMNode.hpp,v 1.2 2003/05/06 13:48:34 neilg Exp $
 */

#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMAttr.hpp>
#include <xercesc/dom/DOMElement.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/* This class is a smart pointer implementation over DOMNode interface and
** classes derived from it. It takes care of reference counting automatically.
** Reference counting is optional so use of this class is experimental.
*/
template <class T> class StDOMNode {
	T* m_node;

	static inline void INCREFCOUNT(T *x) { if (x != (T*)0) x->incRefCount(); }
	static inline void DECREFCOUNT(T *x) { if (x != (T*)0) x->decRefCount(); }

public:
	inline StDOMNode(T* node = (T*)0) : m_node(node) { INCREFCOUNT(m_node); }
	inline StDOMNode(const StDOMNode& stNode) : m_node(stNode.m_node) { INCREFCOUNT(m_node); }
	inline ~StDOMNode() { DECREFCOUNT(m_node); }

	inline T* operator= (T *node)
	{
		if (m_node != node) {
			DECREFCOUNT(m_node);
			m_node = node;
			INCREFCOUNT(m_node);
		}
		return (m_node);
	}

	inline bool operator!= (T* node) const { return (m_node != node); }
	inline bool operator== (T* node) const { return (m_node == node); }

	inline T& operator* () { return (*m_node); }
	inline const T& operator* () const { return (*m_node); }
	inline T* operator-> () const { return (m_node); }
	inline operator T*() const { return (m_node); }
	inline void ClearNode() { operator=((T*)(0)); }
};

#if defined(XML_DOMREFCOUNT_EXPERIMENTAL)
    typedef StDOMNode<DOMNode> DOMNodeSPtr;
#else
    typedef DOMNode* DOMNodeSPtr;
#endif

/* StDOMNode is a smart pointer implementation over DOMNode interface and
** classes derived from it. It takes care of reference counting automatically.
** Reference counting is optional so use of this class is experimental.
*/
#if defined(XML_DOMREFCOUNT_EXPERIMENTAL)
    typedef StDOMNode<DOMAttr> DOMAttrSPtr;
#else
    typedef DOMAttr* DOMAttrSPtr;
#endif

/* StDOMNode is a smart pointer implementation over DOMNode interface and
** classes derived from it. It takes care of reference counting automatically.
** Reference counting is optional so use of this class is experimental.
*/
#if defined(XML_DOMREFCOUNT_EXPERIMENTAL)
    typedef StDOMNode<DOMElement> DOMElementSPtr;
#else
    typedef DOMElement* DOMElementSPtr;
#endif

XERCES_CPP_NAMESPACE_END

#endif

