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
 * $Id: ParenToken.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#if !defined(PARENTOKEN_HPP)
#define PARENTOKEN_HPP

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/regx/Token.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLUTIL_EXPORT ParenToken : public Token {
public:
	// -----------------------------------------------------------------------
    //  Public Constructors and Destructor
    // -----------------------------------------------------------------------
	ParenToken(const unsigned short tokType, Token* const tok,
               const int noParen, MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    ~ParenToken();

	// -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    int size() const;
	int getNoParen() const;
    Token* getChild(const int index) const;

private:
	// -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    ParenToken(const ParenToken&);
    ParenToken& operator=(const ParenToken&);

	// -----------------------------------------------------------------------
    //  Private data members
	// -----------------------------------------------------------------------
	int    fNoParen;
	Token* fChild;
};


// ---------------------------------------------------------------------------
//  ParenToken: getter methods
// ---------------------------------------------------------------------------
inline int ParenToken::size() const {

    return 1;
}

inline int ParenToken::getNoParen() const {

    return fNoParen;
}

inline Token* ParenToken::getChild(const int) const {

    return fChild;
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file ParenToken.hpp
  */
