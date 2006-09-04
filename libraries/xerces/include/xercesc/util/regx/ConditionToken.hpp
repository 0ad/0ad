/*
 * Copyright 2001,2004 The Apache Software Foundation.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * $Id: ConditionToken.hpp 176026 2004-09-08 13:57:07Z peiyongz $
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
