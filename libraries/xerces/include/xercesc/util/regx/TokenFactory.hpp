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
 * $Id: TokenFactory.hpp,v 1.8 2003/10/17 16:44:34 knoaman Exp $
 */

#if !defined(TOKENFACTORY_HPP)
#define TOKENFACTORY_HPP

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/RefVectorOf.hpp>
#include <xercesc/util/regx/Token.hpp>
#include <xercesc/util/Mutexes.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Forward Declaration
// ---------------------------------------------------------------------------
class RangeToken;
class CharToken;
class ClosureToken;
class ConditionToken;
class ConcatToken;
class ModifierToken;
class ParenToken;
class StringToken;
class UnionToken;

class XMLUTIL_EXPORT TokenFactory : public XMemory
{

public:
	// -----------------------------------------------------------------------
    //  Constructors and destructors
    // -----------------------------------------------------------------------
    TokenFactory(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    ~TokenFactory();

    // -----------------------------------------------------------------------
    //  Factory methods
    // -----------------------------------------------------------------------
    Token* createToken(const unsigned short tokType);

    ParenToken* createLook(const unsigned short tokType, Token* const token);
    ParenToken* createParenthesis(Token* const token, const int noGroups);
    ClosureToken* createClosure(Token* const token, bool isNonGreedy = false);
    ConcatToken* createConcat(Token* const token1, Token* const token2);
    UnionToken* createUnion(const bool isConcat = false);
    RangeToken* createRange(const bool isNegRange = false);
    CharToken* createChar(const XMLUInt32 ch, const bool isAnchor = false);
    StringToken* createBackReference(const int refNo);
    StringToken* createString(const XMLCh* const literal);
    ModifierToken* createModifierGroup(Token* const child,
                                       const int add, const int mask);
    ConditionToken* createCondition(const int refNo, Token* const condition,
                                    Token* const yesFlow, Token* const noFlow);


	//static void printUnicode();

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    /*
     *  Gets a commonly used RangeToken from the token registry based on the
     *  range name.
     */
    RangeToken* getRange(const XMLCh* const name,const bool complement=false);
    Token* getLineBegin();
	Token* getLineBegin2();
    Token* getLineEnd();
    Token* getStringBegin();
    Token* getStringEnd();
    Token* getStringEnd2();
    Token* getWordEdge();
    Token* getNotWordEdge();
    Token* getWordBegin();
    Token* getWordEnd();
    Token* getDot();
	Token* getCombiningCharacterSequence();
	Token* getGraphemePattern();
    MemoryManager* getMemoryManager() const;

    // -----------------------------------------------------------------------
    //  Notification that lazy data has been deleted
    // -----------------------------------------------------------------------
	static void reinitTokenFactoryMutex();

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    TokenFactory(const TokenFactory&);
    TokenFactory& operator=(const TokenFactory&);

    // -----------------------------------------------------------------------
    //  Private Helpers methods
    // -----------------------------------------------------------------------
    /*
     *  Initializes the registry with a set of commonly used RangeToken
     *  objects.
     */
    void initializeRegistry();
    friend class RangeTokenMap;

    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fRangeInitialized
    //      Indicates whether we have initialized the RangeFactory instance or
    //      not
	//		
    //  fToken
    //      Contains user created Token objects. Used for memory cleanup.
    // -----------------------------------------------------------------------
    static bool         fRangeInitialized;
    RefVectorOf<Token>* fTokens;
    Token*              fEmpty;
    Token*              fLineBegin;
    Token*              fLineBegin2;
    Token*              fLineEnd;
    Token*              fStringBegin;
    Token*              fStringEnd;
    Token*              fStringEnd2;
    Token*              fWordEdge;
    Token*              fNotWordEdge;
    Token*              fWordEnd;
    Token*              fWordBegin;
    Token*              fDot;
    Token*              fCombiningChar;
    Token*              fGrapheme;
    MemoryManager*      fMemoryManager;
};

inline MemoryManager* TokenFactory::getMemoryManager() const
{
    return fMemoryManager;
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  *	End file TokenFactory
  */

