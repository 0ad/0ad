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
 * $Id: ClosureToken.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#if !defined(CLOSURETOKEN_HPP)
#define CLOSURETOKEN_HPP

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/regx/Token.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLUTIL_EXPORT ClosureToken : public Token {
public:
	// -----------------------------------------------------------------------
    //  Public Constructors and Destructor
    // -----------------------------------------------------------------------
	ClosureToken(const unsigned short tokType, Token* const tok
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
	~ClosureToken();

	// -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
	int size() const;
	int getMin() const;
	int getMax() const;
	Token* getChild(const int index) const;

	// -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
	void setMin(const int minVal);
	void setMax(const int maxVal);

private:
	// -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    ClosureToken(const ClosureToken&);
    ClosureToken& operator=(const ClosureToken&);

	// -----------------------------------------------------------------------
    //  Private data members
	// -----------------------------------------------------------------------
	int    fMin;
	int    fMax;
	Token* fChild;
};


// ---------------------------------------------------------------------------
//  ClosureToken: getter methods
// ---------------------------------------------------------------------------
inline int ClosureToken::size() const {

	return 1;
}


inline int ClosureToken::getMax() const {

	return fMax;
}

inline int ClosureToken::getMin() const {

	return fMin;
}

inline Token* ClosureToken::getChild(const int) const {

	return fChild;
}

// ---------------------------------------------------------------------------
//  ClosureToken: setter methods
// ---------------------------------------------------------------------------
inline void ClosureToken::setMax(const int maxVal) {

	fMax = maxVal;
}

inline void ClosureToken::setMin(const int minVal) {

	fMin = minVal;
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file ClosureToken.hpp
  */
