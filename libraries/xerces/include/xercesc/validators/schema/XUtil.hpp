/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2001 The Apache Software Foundation.  All rights
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
 * $Log: XUtil.hpp,v $
 * Revision 1.5  2004/01/29 11:52:31  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.4  2002/11/04 14:49:42  tng
 * C++ Namespace Support.
 *
 * Revision 1.3  2002/05/21 19:31:45  tng
 * DOM Reorganization: modify to use the new DOM interface and remove obsolete code in XUtil.
 *
 * Revision 1.2  2002/02/06 22:21:49  knoaman
 * Use IDOM for schema processing.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:50  peiyongz
 * sane_include
 *
 * Revision 1.3  2001/11/02 14:13:45  knoaman
 * Add support for identity constraints.
 *
 * Revision 1.2  2001/05/11 13:27:39  tng
 * Copyright update.
 *
 * Revision 1.1  2001/03/30 16:06:00  tng
 * Schema: XUtil, added by Pei Yong Zhang
 *
 */

#if !defined(XUTIL_HPP)
#define XUTIL_HPP

#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>
#include <xercesc/dom/DOMNode.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class DOMNode;
class DOMElement;

/**
 * Some useful utility methods.
 */
class VALIDATORS_EXPORT XUtil
{
public:

    // Finds and returns the first child element node.
    static DOMElement* getFirstChildElement(const DOMNode* const parent);

    // Finds and returns the first child node with the given qualifiedname.
    static DOMElement* getFirstChildElementNS(const DOMNode* const parent
                                              , const XMLCh** const elemNames
                                              , const XMLCh* const uriStr
                                              , unsigned int       length);

    // Finds and returns the next sibling element node.
    static DOMElement* getNextSiblingElement(const DOMNode* const node);

    static DOMElement* getNextSiblingElementNS(const DOMNode* const node
                                               , const XMLCh** const elemNames
                                               , const XMLCh* const uriStr
                                               , unsigned int        length);

private:
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------

	// This class cannot be instantiated.
     XUtil() {};
	~XUtil() {};
};

XERCES_CPP_NAMESPACE_END

#endif
