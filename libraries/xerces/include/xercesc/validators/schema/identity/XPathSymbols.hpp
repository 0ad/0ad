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
 * $Id: XPathSymbols.hpp,v 1.4 2004/01/29 11:52:32 cargilld Exp $
 */

#if !defined(XPATHSYMBOLS_HPP)
#define XPATHSYMBOLS_HPP

#include <xercesc/util/XercesDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/*
 * Collection of symbols used to parse a Schema Grammar
 */

class VALIDATORS_EXPORT XPathSymbols
{
public :
    // -----------------------------------------------------------------------
    // Constant data
    // -----------------------------------------------------------------------
    static const XMLCh fgSYMBOL_AND[];
    static const XMLCh fgSYMBOL_OR[];
    static const XMLCh fgSYMBOL_MOD[];
    static const XMLCh fgSYMBOL_DIV[];
    static const XMLCh fgSYMBOL_COMMENT[];
    static const XMLCh fgSYMBOL_TEXT[];
    static const XMLCh fgSYMBOL_PI[];
    static const XMLCh fgSYMBOL_NODE[];
    static const XMLCh fgSYMBOL_ANCESTOR[];
    static const XMLCh fgSYMBOL_ANCESTOR_OR_SELF[];
    static const XMLCh fgSYMBOL_ATTRIBUTE[];
    static const XMLCh fgSYMBOL_CHILD[];
    static const XMLCh fgSYMBOL_DESCENDANT[];
    static const XMLCh fgSYMBOL_DESCENDANT_OR_SELF[];
    static const XMLCh fgSYMBOL_FOLLOWING[];
    static const XMLCh fgSYMBOL_FOLLOWING_SIBLING[];
    static const XMLCh fgSYMBOL_NAMESPACE[];
    static const XMLCh fgSYMBOL_PARENT[];
    static const XMLCh fgSYMBOL_PRECEDING[];
    static const XMLCh fgSYMBOL_PRECEDING_SIBLING[];
    static const XMLCh fgSYMBOL_SELF[];

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XPathSymbols();
};

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file XPathSymbols.hpp
  */

