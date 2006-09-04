/*
 * Copyright 2001-2004 The Apache Software Foundation.
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
 * $Id: Token.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#if !defined(TOKEN_HPP)
#define TOKEN_HPP

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/RuntimeException.hpp>
#include <xercesc/util/PlatformUtils.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Forward Declaration
// ---------------------------------------------------------------------------
class RangeToken;
class TokenFactory;


class XMLUTIL_EXPORT Token : public XMemory
{
public:
	// -----------------------------------------------------------------------
    //  Public Constructors and Destructor
    // -----------------------------------------------------------------------
	Token(const unsigned short tokType
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
        );
	virtual ~Token();

	// -----------------------------------------------------------------------
    //  Public Constants
    // -----------------------------------------------------------------------
	// Token types
	enum {
		T_CHAR = 0,
		T_CONCAT = 1,
		T_UNION = 2,
		T_CLOSURE = 3,
		T_RANGE = 4,
		T_NRANGE = 5,
		T_PAREN = 6,
		T_EMPTY = 7,
		T_ANCHOR = 8,
		T_NONGREEDYCLOSURE = 9,
		T_STRING = 10,
		T_DOT = 11,
		T_BACKREFERENCE = 12,
		T_LOOKAHEAD = 20,
		T_NEGATIVELOOKAHEAD = 21,
		T_LOOKBEHIND = 22,
		T_NEGATIVELOOKBEHIND = 23,
		T_INDEPENDENT = 24,
		T_MODIFIERGROUP = 25,
		T_CONDITION = 26
	};

	static const XMLInt32		UTF16_MAX;
	static const unsigned short FC_CONTINUE;
	static const unsigned short FC_TERMINAL;
	static const unsigned short FC_ANY;

	// -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
	unsigned short       getTokenType() const;
	int                  getMinLength() const;
    int                  getMaxLength() const;
	virtual Token*       getChild(const int index) const;
	virtual int          size() const;
    virtual int          getMin() const;
    virtual int          getMax() const;
    virtual int          getNoParen() const;
	virtual int          getReferenceNo() const;
    virtual const XMLCh* getString() const;
	virtual XMLInt32     getChar() const;

	// -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
	void setTokenType(const unsigned short tokType);
	virtual void setMin(const int minVal);
	virtual void setMax(const int maxVal);

    // -----------------------------------------------------------------------
    //  Range manipulation methods
    // -----------------------------------------------------------------------
	virtual void addRange(const XMLInt32 start, const XMLInt32 end);
	virtual void mergeRanges(const Token *const tok);
	virtual void sortRanges();
	virtual void compactRanges();
	virtual void subtractRanges(RangeToken* const tok);
	virtual void intersectRanges(RangeToken* const tok);

	// -----------------------------------------------------------------------
    //  Putter methods
    // -----------------------------------------------------------------------
	virtual void addChild(Token* const child, TokenFactory* const tokFactory);

	// -----------------------------------------------------------------------
    //  Helper methods
    // -----------------------------------------------------------------------
	int analyzeFirstCharacter(RangeToken* const rangeTok, const int options,
                              TokenFactory* const tokFactory);
    Token* findFixedString(int options, int& outOptions);

private:
	// -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    Token(const Token&);
    Token& operator=(const Token&);

    // -----------------------------------------------------------------------
    //  Private Helper methods
    // -----------------------------------------------------------------------
	bool isSet(const int options, const unsigned int flag);
    bool isShorterThan(Token* const tok);

	// -----------------------------------------------------------------------
    //  Private data members
	// -----------------------------------------------------------------------
	unsigned short fTokenType;
protected:
    MemoryManager* const    fMemoryManager;
};


// ---------------------------------------------------------------------------
//  Token: getter methods
// ---------------------------------------------------------------------------
inline unsigned short Token::getTokenType() const {

	return fTokenType;
}

inline int Token::size() const {

	return 0;
}

inline Token* Token::getChild(const int) const {

	return 0;
}

inline int Token::getMin() const {

    return -1;
}

inline int Token::getMax() const {

    return -1;
}

inline int Token::getReferenceNo() const {

    return 0;
}

inline int Token::getNoParen() const {

    return 0;
}

inline const XMLCh* Token::getString() const {

    return 0;
}

inline XMLInt32 Token::getChar() const {

    return -1;
}

// ---------------------------------------------------------------------------
//  Token: setter methods
// ---------------------------------------------------------------------------
inline void Token::setTokenType(const unsigned short tokType) {
	
	fTokenType = tokType;
}

inline void Token::setMax(const int) {
	// ClosureToken
}

inline void Token::setMin(const int) {
	// ClosureToken
}

inline bool Token::isSet(const int options, const unsigned int flag) {

	return (options & flag) == flag;
}

// ---------------------------------------------------------------------------
//  Token: setter methods
// ---------------------------------------------------------------------------
inline void Token::addChild(Token* const, TokenFactory* const) {

    ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::Regex_NotSupported, fMemoryManager);
}

// ---------------------------------------------------------------------------
//  Token: Range manipulation methods
// ---------------------------------------------------------------------------
inline void Token::addRange(const XMLInt32, const XMLInt32) {

    ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::Regex_NotSupported, fMemoryManager);
}

inline void Token::mergeRanges(const Token *const) {

    ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::Regex_NotSupported, fMemoryManager);
}

inline void Token::sortRanges() {

    ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::Regex_NotSupported, fMemoryManager);
}

inline void Token::compactRanges() {

    ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::Regex_NotSupported, fMemoryManager);
}

inline void Token::subtractRanges(RangeToken* const) {

    ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::Regex_NotSupported, fMemoryManager);
}

inline void Token::intersectRanges(RangeToken* const) {

    ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::Regex_NotSupported, fMemoryManager);
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file Token.hpp
  */

