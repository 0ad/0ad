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
 * $Id: RegxParser.hpp 180016 2005-06-04 19:49:30Z jberry $
 */

/*
 *	A regular expression parser
 */
#if !defined(REGXPARSER_HPP)
#define REGXPARSER_HPP

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/RefVectorOf.hpp>
#include <xercesc/util/XMLUniDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Forward Declaration
// ---------------------------------------------------------------------------
class Token;
class RangeToken;
class TokenFactory;

class XMLUTIL_EXPORT RegxParser : public XMemory
{
public:

	// -----------------------------------------------------------------------
    //  Public constant data
    // -----------------------------------------------------------------------
    // Parse tokens
	enum {
		REGX_T_CHAR                     = 0,
		REGX_T_EOF                      = 1,
		REGX_T_OR                       = 2,
		REGX_T_STAR                     = 3,
		REGX_T_PLUS                     = 4,
		REGX_T_QUESTION                 = 5,
		REGX_T_LPAREN                   = 6,
		REGX_T_RPAREN                   = 7,
		REGX_T_DOT                      = 8,
		REGX_T_LBRACKET                 = 9,
		REGX_T_BACKSOLIDUS              = 10,
		REGX_T_CARET                    = 11,
		REGX_T_DOLLAR                   = 12,
		REGX_T_LPAREN2                  = 13,
		REGX_T_LOOKAHEAD                = 14,
		REGX_T_NEGATIVELOOKAHEAD        = 15,
		REGX_T_LOOKBEHIND               = 16,
		REGX_T_NEGATIVELOOKBEHIND       = 17,
		REGX_T_INDEPENDENT              = 18,
		REGX_T_SET_OPERATIONS           = 19,
		REGX_T_POSIX_CHARCLASS_START    = 20,
		REGX_T_COMMENT                  = 21,
		REGX_T_MODIFIERS                = 22,
		REGX_T_CONDITION                = 23,
		REGX_T_XMLSCHEMA_CC_SUBTRACTION	= 24
	};

	static const unsigned short S_NORMAL;
	static const unsigned short S_INBRACKETS;
	static const unsigned short S_INXBRACKETS;

	// -----------------------------------------------------------------------
    //  Public Constructors and Destructor
    // -----------------------------------------------------------------------
	RegxParser(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
	virtual ~RegxParser();

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    unsigned short getParseContext() const;
    unsigned short getState() const;
    XMLInt32       getCharData() const;
    int            getNoParen() const;
	int            getOffset() const;
	bool           hasBackReferences() const;
    TokenFactory*  getTokenFactory() const;

	// -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
	void setParseContext(const unsigned short value);
    void setTokenFactory(TokenFactory* const tokFactory);

	// -----------------------------------------------------------------------
    //  Public Parsing methods
    // -----------------------------------------------------------------------
	Token* parse(const XMLCh* const regxStr, const int options);

protected:
    // -----------------------------------------------------------------------
    //  Protected Helper methods
    // -----------------------------------------------------------------------
    virtual bool        checkQuestion(const int off);
	virtual XMLInt32    decodeEscaped();
    MemoryManager*      getMemoryManager() const;
    // -----------------------------------------------------------------------
    //  Protected Parsing/Processing methods
    // -----------------------------------------------------------------------
	void                processNext();
	Token*              parseRegx(const bool matchingRParen = false);
	virtual Token*      processCaret();
    virtual Token*      processDollar();
	virtual Token*      processLook(const unsigned short tokType);
    virtual Token*      processBacksolidus_A();
    virtual Token*      processBacksolidus_z();
    virtual Token*      processBacksolidus_Z();
    virtual Token*      processBacksolidus_b();
    virtual Token*      processBacksolidus_B();
    virtual Token*      processBacksolidus_lt();
    virtual Token*      processBacksolidus_gt();
    virtual Token*      processBacksolidus_c();
    virtual Token*      processBacksolidus_C();
    virtual Token*      processBacksolidus_i();
    virtual Token*      processBacksolidus_I();
    virtual Token*      processBacksolidus_g();
    virtual Token*      processBacksolidus_X();
    virtual Token*      processBackReference();
	virtual Token*      processStar(Token* const tok);
	virtual Token*      processPlus(Token* const tok);
	virtual Token*      processQuestion(Token* const tok);
    virtual Token*      processParen();
    virtual Token*      processParen2();
    virtual Token*      processCondition();
    virtual Token*      processModifiers();
    virtual Token*      processIndependent();
    virtual RangeToken* parseCharacterClass(const bool useNRange);
    virtual RangeToken* parseSetOperations();
	virtual XMLInt32    processCInCharacterClass(RangeToken* const tok,
                                                 const XMLInt32 ch);
    RangeToken*         processBacksolidus_pP(const XMLInt32 ch);

    // -----------------------------------------------------------------------
    //  Protected PreCreated RangeToken access methods
    // -----------------------------------------------------------------------
	virtual Token*      getTokenForShorthand(const XMLInt32 ch);

private:
    // -----------------------------------------------------------------------
    //  Private parsing/processing methods
    // -----------------------------------------------------------------------
    Token* parseTerm(const bool matchingRParen = false);
	Token* parseFactor();
	Token* parseAtom();

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    RegxParser(const RegxParser&);
    RegxParser& operator=(const RegxParser&);

	// -----------------------------------------------------------------------
    //  Private data types
    // -----------------------------------------------------------------------
    class ReferencePosition : public XMemory
    {
        public :
            ReferencePosition(const int refNo, const int position);

            int	fReferenceNo;
			int	fPosition;
    };

    // -----------------------------------------------------------------------
    //  Private Helper methods
    // -----------------------------------------------------------------------
    bool isSet(const int flag);
	int hexChar(const XMLInt32 ch);

	// -----------------------------------------------------------------------
    //  Private data members
	// -----------------------------------------------------------------------
    MemoryManager*                  fMemoryManager;
	bool                            fHasBackReferences;
	int                             fOptions;
	int                             fOffset;
	int                             fNoGroups;
	unsigned short                  fParseContext;
	int                             fStringLen;
	unsigned short                  fState;
	XMLInt32                        fCharData;
	XMLCh*                          fString;
	RefVectorOf<ReferencePosition>* fReferences;
    TokenFactory*                   fTokenFactory;
};


// ---------------------------------------------------------------------------
//  RegxParser: Getter Methods
// ---------------------------------------------------------------------------
inline unsigned short RegxParser::getParseContext() const {

    return fParseContext;
}

inline unsigned short RegxParser::getState() const {

	return fState;
}

inline XMLInt32 RegxParser::getCharData() const {

    return fCharData;
}

inline int RegxParser::getNoParen() const {

    return fNoGroups;
}

inline int RegxParser::getOffset() const {

	return fOffset;
}

inline bool RegxParser::hasBackReferences() const {

	return fHasBackReferences;
}

inline TokenFactory* RegxParser::getTokenFactory() const {

    return fTokenFactory;
}

inline MemoryManager* RegxParser::getMemoryManager() const {
    return fMemoryManager;
}
// ---------------------------------------------------------------------------
//  RegxParser: Setter Methods
// ---------------------------------------------------------------------------
inline void RegxParser::setParseContext(const unsigned short value) {

	fParseContext = value;
}

inline void RegxParser::setTokenFactory(TokenFactory* const tokFactory) {

    fTokenFactory = tokFactory;
}

// ---------------------------------------------------------------------------
//  RegxParser: Helper Methods
// ---------------------------------------------------------------------------
inline bool RegxParser::isSet(const int flag) {

    return (fOptions & flag) == flag;
}


inline int RegxParser::hexChar(const XMLInt32 ch) {

	if (ch < chDigit_0 || ch > chLatin_f)
		return -1;

	if (ch <= chDigit_9)
		return ch - chDigit_0;

	if (ch < chLatin_A)
		return -1;

	if (ch <= chLatin_F)
		return ch - chLatin_A + 10;

	if (ch < chLatin_a)
		return -1;

	return ch - chLatin_a + 10;
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  *	End file RegxParser.hpp
  */

