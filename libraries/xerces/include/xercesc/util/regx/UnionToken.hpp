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
 * $Id: UnionToken.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#if !defined(UNIONTOKEN_HPP)
#define UNIONTOKEN_HPP

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/regx/Token.hpp>
#include <xercesc/util/RefVectorOf.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLUTIL_EXPORT UnionToken : public Token {
public:
	// -----------------------------------------------------------------------
    //  Public Constructors and Destructor
    // -----------------------------------------------------------------------
	UnionToken(const unsigned short tokType
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    ~UnionToken();

	// -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    int size() const;
    Token* getChild(const int index) const;

	// -----------------------------------------------------------------------
    //  Children manipulation methods
    // -----------------------------------------------------------------------
    void addChild(Token* const child, TokenFactory* const tokFactory);

private:
	// -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    UnionToken(const UnionToken&);
    UnionToken& operator=(const UnionToken&);

	// -----------------------------------------------------------------------
    //  Private Constants
    // -----------------------------------------------------------------------
	static const unsigned short INITIALSIZE;

	// -----------------------------------------------------------------------
    //  Private data members
	// -----------------------------------------------------------------------
	RefVectorOf<Token>* fChildren;
};


// ---------------------------------------------------------------------------
//  UnionToken: getter methods
// ---------------------------------------------------------------------------
inline int UnionToken::size() const {

    return fChildren == 0 ? 0 : fChildren->size();
}

inline Token* UnionToken::getChild(const int index) const {

    return fChildren->elementAt(index);
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file UnionToken.hpp
  */
