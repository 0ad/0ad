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
 * $Id: RegularExpression.hpp 225473 2005-07-27 07:09:16Z dbertoni $
 */

#if !defined(REGULAREXPRESSION_HPP)
#define REGULAREXPRESSION_HPP

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/RefArrayVectorOf.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/Janitor.hpp>
#include <xercesc/util/regx/Op.hpp>
#include <xercesc/util/regx/TokenFactory.hpp>
#include <xercesc/util/regx/BMPattern.hpp>
#include <xercesc/util/regx/ModifierToken.hpp>
#include <xercesc/util/regx/ConditionToken.hpp>
#include <xercesc/util/regx/OpFactory.hpp>
#include <xercesc/util/regx/RegxUtil.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Forward Declaration
// ---------------------------------------------------------------------------
class RangeToken;
class Match;

class XMLUTIL_EXPORT RegularExpression : public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Public Constructors and Destructor
    // -----------------------------------------------------------------------
    RegularExpression
    (
        const char* const pattern
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    RegularExpression
    (
        const char* const pattern
        , const char* const options
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    RegularExpression
    (
        const XMLCh* const pattern
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    RegularExpression
    (
        const XMLCh* const pattern
        , const XMLCh* const options
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    ~RegularExpression();

    // -----------------------------------------------------------------------
    //  Public Constants
    // -----------------------------------------------------------------------
    static const unsigned int   MARK_PARENS;
    static const unsigned int   IGNORE_CASE;
    static const unsigned int   SINGLE_LINE;
    static const unsigned int   MULTIPLE_LINE;
    static const unsigned int   EXTENDED_COMMENT;
    static const unsigned int   USE_UNICODE_CATEGORY;
    static const unsigned int   UNICODE_WORD_BOUNDARY;
    static const unsigned int   PROHIBIT_HEAD_CHARACTER_OPTIMIZATION;
    static const unsigned int   PROHIBIT_FIXED_STRING_OPTIMIZATION;
    static const unsigned int   XMLSCHEMA_MODE;
    static const unsigned int   SPECIAL_COMMA;
    static const unsigned short WT_IGNORE;
    static const unsigned short WT_LETTER;
    static const unsigned short WT_OTHER;

    // -----------------------------------------------------------------------
    //  Public Helper methods
    // -----------------------------------------------------------------------
    static int getOptionValue(const XMLCh ch);

    // -----------------------------------------------------------------------
    //  Matching methods
    // -----------------------------------------------------------------------
    bool matches(const char* const matchString, MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    bool matches(const char* const matchString, const int start,
                 const int end, MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    bool matches(const char* const matchString, Match* const pMatch, MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    bool matches(const char* const matchString, const int start,
                 const int end, Match* const pMatch, MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    bool matches(const XMLCh* const matchString, MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    bool matches(const XMLCh* const matchString, const int start,
                 const int end, MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    bool matches(const XMLCh* const matchString, Match* const pMatch, MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    bool matches(const XMLCh* const matchString, const int start,
                 const int end, Match* const pMatch, MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    // -----------------------------------------------------------------------
    //  Tokenize methods
    // -----------------------------------------------------------------------
    // Note: The caller owns the string vector that is returned, and is responsible
    //       for deleting it.
    RefArrayVectorOf<XMLCh> *tokenize(const char* const matchString);
    RefArrayVectorOf<XMLCh> *tokenize(const char* const matchString, const int start,
                                      const int end);

    RefArrayVectorOf<XMLCh> *tokenize(const XMLCh* const matchString);
    RefArrayVectorOf<XMLCh> *tokenize(const XMLCh* const matchString,
                                      const int start, const int end);

    // -----------------------------------------------------------------------
    //  Replace methods
    // -----------------------------------------------------------------------
    // Note: The caller owns the XMLCh* that is returned, and is responsible for
    //       deleting it.
    XMLCh *replace(const char* const matchString, const char* const replaceString);
    XMLCh *replace(const char* const matchString, const char* const replaceString,
                   const int start, const int end);

    XMLCh *replace(const XMLCh* const matchString, const XMLCh* const replaceString);
    XMLCh *replace(const XMLCh* const matchString, const XMLCh* const replaceString,
                   const int start, const int end);

    // -----------------------------------------------------------------------
    //  Static initialize and cleanup methods
    // -----------------------------------------------------------------------
    static void
    staticInitialize(MemoryManager*  memoryManager);

    static void
    staticCleanup();

    static bool isSet(const int options, const int flag);

private:
    // -----------------------------------------------------------------------
    //  Private data types
    // -----------------------------------------------------------------------
    class XMLUTIL_EXPORT Context : public XMemory
    {
        public :
            Context(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
            ~Context();

            inline const XMLCh* getString() const { return fString; }
            void reset(const XMLCh* const string, const int stringLen,
                       const int start, const int limit, const int noClosures);
            bool nextCh(XMLInt32& ch, int& offset, const short direction);
            
            bool           fAdoptMatch;
            int            fStart;
            int            fLimit;
            int            fLength;    // fLimit - fStart
            int            fSize;
            int            fStringMaxLen;
            int*           fOffsets;
            Match*         fMatch;
            const XMLCh*   fString;
            MemoryManager* fMemoryManager;
    };

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    RegularExpression(const RegularExpression&);
    RegularExpression& operator=(const RegularExpression&);

    // -----------------------------------------------------------------------
    //  Cleanup methods
    // -----------------------------------------------------------------------
    void cleanUp();

    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    void setPattern(const XMLCh* const pattern, const XMLCh* const options=0);

    // -----------------------------------------------------------------------
    //  Private Helper methods
    // -----------------------------------------------------------------------
    void prepare();
    int parseOptions(const XMLCh* const options);
    unsigned short getWordType(const XMLCh* const target, const int begin,
                               const int end, const int offset);
    unsigned short getCharType(const XMLCh ch);
    unsigned short getPreviousWordType(const XMLCh* const target,
                                       const int start, const int end,
                                       int offset);

    /**
      *    Matching helpers
      */
    int match(Context* const context, const Op* const operations, int offset,
              const short direction);
    bool matchIgnoreCase(const XMLInt32 ch1, const XMLInt32 ch2);

    /**
      *    Helper methods used by match(Context* ...)
      */
    bool matchChar(Context* const context, const XMLInt32 ch, int& offset,
                   const short direction, const bool ignoreCase);
    bool matchDot(Context* const context, int& offset, const short direction);
    bool matchRange(Context* const context, const Op* const op,
                    int& offset, const short direction, const bool ignoreCase);
    bool matchAnchor(Context* const context, const XMLInt32 ch,
                     const int offset);
    bool matchBackReference(Context* const context, const XMLInt32 ch,
                            int& offset, const short direction,
                            const bool ignoreCase);
    bool matchString(Context* const context, const XMLCh* const literal,
                     int& offset, const short direction, const bool ignoreCase);
    int  matchUnion(Context* const context, const Op* const op, int offset,
                    const short direction);
    int matchCapture(Context* const context, const Op* const op, int offset,
                     const short direction);
    bool matchCondition(Context* const context, const Op* const op, int offset,
                        const short direction);
    int matchModifier(Context* const context, const Op* const op, int offset,
                      const short direction);

    /**
     *    Tokenize helper
     *
     *    This overloaded tokenize is for internal use only. It provides a way to
     *    keep track of the sub-expressions in each match of the pattern.
     *
     *    It is called by the other tokenize methods, and by the replace method.
     *    The caller is responsible for the deletion of the returned
     *    RefArrayVectorOf<XMLCh*>
     */
    RefArrayVectorOf<XMLCh> *tokenize(const XMLCh* const matchString,
                                      const int start, const int end,
                                      RefVectorOf<Match> *subEx);
    /**
     *    Replace helpers
     *
     *    Note: the caller owns the XMLCh* that is returned
     */
    const XMLCh *subInExp(const XMLCh* const repString,
                          const XMLCh* const origString,
                          const Match* subEx);
    /**
     *    Converts a token tree into an operation tree
     */
    void compile(const Token* const token);
    Op*  compile(const Token* const token, Op* const next,
                 const bool reverse);
    /**
      *    Helper methods used by compile
      */
    Op* compileSingle(const Token* const token, Op* const next,
                      const unsigned short tokType);
    Op* compileUnion(const Token* const token, Op* const next,
                     const bool reverse);
    Op* compileCondition(const Token* const token, Op* const next,
                         const bool reverse);
    Op* compileParenthesis(const Token* const token, Op* const next,
                           const bool reverse);
    Op* compileLook(const Token* const token, const Op* const next,
                    const bool reverse, const unsigned short tokType);
    Op* compileConcat(const Token* const token, Op* const next,
                      const bool reverse);
    Op* compileClosure(const Token* const token, Op* const next,
                       const bool reverse, const unsigned short tokType);

    // -----------------------------------------------------------------------
    //  Private data members
    // -----------------------------------------------------------------------
    bool               fHasBackReferences;
    bool               fFixedStringOnly;
    int                fNoGroups;
    int                fMinLength;
    int                fNoClosures;
    unsigned int       fOptions;
    BMPattern*         fBMPattern;
    XMLCh*             fPattern;
    XMLCh*             fFixedString;
    Op*                fOperations;
    Token*             fTokenTree;
    RangeToken*        fFirstChar;
    static RangeToken* fWordRange;
    OpFactory          fOpFactory;
    TokenFactory*      fTokenFactory;
    MemoryManager*     fMemoryManager;
};



  // -----------------------------------------------------------------------
  //  RegularExpression: Static initialize and cleanup methods
  // -----------------------------------------------------------------------
  inline void RegularExpression::staticCleanup()
  {
      fWordRange = 0;
  }

  // ---------------------------------------------------------------------------
  //  RegularExpression: Cleanup methods
  // ---------------------------------------------------------------------------
  inline void RegularExpression::cleanUp() {

      fMemoryManager->deallocate(fPattern);//delete [] fPattern;
      fMemoryManager->deallocate(fFixedString);//delete [] fFixedString;      
      delete fBMPattern;
      delete fTokenFactory;
  }

  // ---------------------------------------------------------------------------
  //  RegularExpression: Helper methods
  // ---------------------------------------------------------------------------
  inline bool RegularExpression::isSet(const int options, const int flag) {

      return (options & flag) == flag;
  }

  inline Op* RegularExpression::compileLook(const Token* const token,
                                            const Op* const next,
                                            const bool reverse,
                                            const unsigned short tokType) {

      Op*    ret = 0;
      Op*    result = compile(token->getChild(0), 0, reverse);

      switch(tokType) {
      case Token::T_LOOKAHEAD:
          ret = fOpFactory.createLookOp(Op::O_LOOKAHEAD, next, result);
          break;
      case Token::T_NEGATIVELOOKAHEAD:
          ret = fOpFactory.createLookOp(Op::O_NEGATIVELOOKAHEAD, next, result);
          break;
      case Token::T_LOOKBEHIND:
          ret = fOpFactory.createLookOp(Op::O_LOOKBEHIND, next, result);
          break;
      case Token::T_NEGATIVELOOKBEHIND:
          ret = fOpFactory.createLookOp(Op::O_NEGATIVELOOKBEHIND, next, result);
          break;
      case Token::T_INDEPENDENT:
          ret = fOpFactory.createIndependentOp(next, result);
          break;
      case Token::T_MODIFIERGROUP:
          ret = fOpFactory.createModifierOp(next, result,
                                     ((const ModifierToken *) token)->getOptions(),
                                     ((const ModifierToken *) token)->getOptionsMask());
          break;
      }


      return ret;
  }

  inline Op* RegularExpression::compileSingle(const Token* const token,
                                              Op* const next,
                                              const unsigned short tokType) {

      Op* ret = 0;

      switch (tokType) {
      case Token::T_DOT:
          ret = fOpFactory.createDotOp();
          break;
      case Token::T_CHAR:
          ret = fOpFactory.createCharOp(token->getChar());
          break;
      case Token::T_ANCHOR:
          ret = fOpFactory.createAnchorOp(token->getChar());
          break;
      case Token::T_RANGE:
      case Token::T_NRANGE:
          ret = fOpFactory.createRangeOp(token);
          break;
      case Token::T_EMPTY:
          ret = next;
          break;
      case Token::T_STRING:
          ret = fOpFactory.createStringOp(token->getString());
          break;
      case Token::T_BACKREFERENCE:
          ret = fOpFactory.createBackReferenceOp(token->getReferenceNo());
          break;
      }

      if (tokType != Token::T_EMPTY)
          ret->setNextOp(next);

      return ret;
  }


  inline Op* RegularExpression::compileUnion(const Token* const token,
                                             Op* const next,
                                             const bool reverse) {

      int tokSize = token->size();
      UnionOp* uniOp = fOpFactory.createUnionOp(tokSize);

      for (int i=0; i<tokSize; i++) {

          uniOp->addElement(compile(token->getChild(i), next, reverse));
      }

      return uniOp;
  }


  inline Op* RegularExpression::compileCondition(const Token* const token,
                                                 Op* const next,
                                                 const bool reverse) {

      Token* condTok = ((const ConditionToken*) token)->getConditionToken();
      Token* yesTok  = token->getChild(0);
      Token* noTok   = token->getChild(1);
      int    refNo   = token->getReferenceNo();
      Op*    condOp  = (condTok == 0) ? 0 : compile(condTok, 0, reverse);
      Op*    yesOp   = compile(yesTok, next, reverse);
      Op*    noOp    = (noTok == 0) ? 0 : compile(noTok, next, reverse);

      return fOpFactory.createConditionOp(next, refNo, condOp, yesOp, noOp);
  }


  inline Op* RegularExpression::compileParenthesis(const Token* const token,
                                                   Op* const next,
                                                   const bool reverse) {

      if (token->getNoParen() == 0)
          return compile(token->getChild(0), next, reverse);

      Op* captureOp    = 0;

      if (reverse) {

          captureOp = fOpFactory.createCaptureOp(token->getNoParen(), next);
          captureOp = compile(token->getChild(0), captureOp, reverse);

          return fOpFactory.createCaptureOp(-token->getNoParen(), captureOp);
      }

      captureOp = fOpFactory.createCaptureOp(-token->getNoParen(), next);
      captureOp = compile(token->getChild(0), captureOp, reverse);

      return fOpFactory.createCaptureOp(token->getNoParen(), captureOp);
  }

  inline Op* RegularExpression::compileConcat(const Token* const token,
                                              Op*  const next,
                                              const bool reverse) {

      Op* ret = next;
      int tokSize = token->size();

      if (!reverse) {

          for (int i= tokSize - 1; i>=0; i--) {
              ret = compile(token->getChild(i), ret, false);
          }
      }
      else {

          for (int i= 0; i< tokSize; i++) {
              ret = compile(token->getChild(i), ret, true);
          }
      }

      return ret;
  }

  inline Op* RegularExpression::compileClosure(const Token* const token,
                                               Op* const next,
                                               const bool reverse,
                                               const unsigned short tokType) {

      Op*    ret      = 0;
      Token* childTok = token->getChild(0);
      int    min      = token->getMin();
      int    max      = token->getMax();

      if (min >= 0 && min == max) {

          ret = next;
          for (int i=0; i< min; i++) {
              ret = compile(childTok, ret, reverse);
          }

          return ret;
      }

      if (min > 0 && max > 0)
          max -= min;

      if (max > 0) {

          ret = next;
          for (int i=0; i<max; i++) {

              ChildOp* childOp = fOpFactory.createQuestionOp(
                  tokType == Token::T_NONGREEDYCLOSURE);

              childOp->setNextOp(next);
              childOp->setChild(compile(childTok, ret, reverse));
              ret = childOp;
          }
      }
      else {

          ChildOp* childOp = 0;

          if (tokType == Token::T_NONGREEDYCLOSURE) {
              childOp = fOpFactory.createNonGreedyClosureOp();
          }
          else {

              if (childTok->getMinLength() == 0)
                  childOp = fOpFactory.createClosureOp(fNoClosures++);
              else
                  childOp = fOpFactory.createClosureOp(-1);
          }

          childOp->setNextOp(next);
          childOp->setChild(compile(childTok, childOp, reverse));
          ret = childOp;
      }

      if (min > 0) {

          for (int i=0; i< min; i++) {
              ret = compile(childTok, ret, reverse);
          }
      }

      return ret;
  }

  inline int RegularExpression::matchUnion(Context* const context,
                                           const Op* const op, int offset,
                                           const short direction)
  {
      unsigned int opSize = op->getSize();
      int ret = -1;

      for (unsigned int i=0; i < opSize; i++) {

          ret = match(context, op->elementAt(i), offset, direction);

          if (ret >= 0 && ret <= context->fLimit)
              return ret;
      }

      return -1;
  }

  inline int RegularExpression::matchModifier(Context* const context,
                                              const Op* const op, int offset,
                                              const short direction)
  {
      int saveOptions = fOptions;
      fOptions |= (int) op->getData();
      fOptions &= (int) ~op->getData2();

      int ret = match(context, op->getChild(), offset, direction);

      fOptions = saveOptions;

      return ret;
  }

  inline unsigned short RegularExpression::getWordType(const XMLCh* const target
                                                       , const int begin
                                                       , const int end
                                                       , const int offset)
  {
      if (offset < begin || offset >= end)
          return WT_OTHER;

      return getCharType(target[offset]);
  }

  inline
  unsigned short RegularExpression::getPreviousWordType(const XMLCh* const target
                                                        , const int start
                                                        , const int end
                                                        , int offset)
  {
      unsigned short ret = getWordType(target, start, end, --offset);

      while (ret == WT_IGNORE) {
          ret = getWordType(target, start, end, --offset);
      }

      return ret;
  }

XERCES_CPP_NAMESPACE_END

#endif
/**
  * End of file RegularExpression.hpp
  */

