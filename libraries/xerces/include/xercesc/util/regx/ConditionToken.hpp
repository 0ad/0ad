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
 * $Id: ConditionToken.hpp,v 1.4 2003/12/17 00:18:37 cargilld Exp $
 */

#if !defined(CONDITIONTOKEN_HPP)
#define CONDITIONTOKEN_HPP

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/regx/Token.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLUTIL_EXPORT ConditionToken : public Token {
public:
	// -----------------------------------------------------------------------
    //  Public Constructors and Destructor
    // -----------------------------------------------------------------------
	ConditionToken(const unsigned int refNo, Token* const condTok,
                   Token* const yesTok, Token* const noTok
                   , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    ~ConditionToken();

	// -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    int size() const;
	int getReferenceNo() const;
    Token* getChild(const int index) const;
    Token* getConditionToken() const;

private:
	// -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    ConditionToken(const ConditionToken&);
    ConditionToken& operator=(const ConditionToken&);

	// -----------------------------------------------------------------------
    //  Private data members
	// -----------------------------------------------------------------------
	int    fRefNo;
	Token* fConditionToken;
	Token* fYesToken;
	Token* fNoToken;
};


// ---------------------------------------------------------------------------
//  ConditionToken: getter methods
// ---------------------------------------------------------------------------
inline int ConditionToken::size() const {

    return fNoToken == 0 ? 1 : 2;
}

inline int ConditionToken::getReferenceNo() const {

    return fRefNo;
}

inline Token* ConditionToken::getChild(const int index) const {

    if (index < 0 || index > 1)
        ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::Regex_InvalidChildIndex, fMemoryManager);

    if (index == 0)
        return fYesToken;
	
    return fNoToken;
}

inline Token* ConditionToken::getConditionToken() const {

	return fConditionToken;
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file ConditionToken.hpp
  */
