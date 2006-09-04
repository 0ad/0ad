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
 * $Id: ParserForXMLSchema.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#if !defined(PARSERFORXMLSCHEMA_HPP)
#define PARSERFORXMLSCHEMA_HPP

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/regx/RegxParser.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Forward Declaration
// ---------------------------------------------------------------------------
class Token;
class RangeToken;

class XMLUTIL_EXPORT ParserForXMLSchema : public RegxParser {
public:
    // -----------------------------------------------------------------------
    //  Public Constructors and Destructor
    // -----------------------------------------------------------------------
    ParserForXMLSchema(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    ~ParserForXMLSchema();

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------

protected:
    // -----------------------------------------------------------------------
    //  Parsing/Processing methods
    // -----------------------------------------------------------------------
    XMLInt32    processCInCharacterClass(RangeToken* const tok,
                                         const XMLInt32 ch);
    Token*      processCaret();
    Token*      processDollar();
	Token*		processLook(const unsigned short tokType);
    Token*      processBacksolidus_A();
    Token*      processBacksolidus_Z();
    Token*      processBacksolidus_z();
    Token*      processBacksolidus_b();
    Token*      processBacksolidus_B();
    Token*      processBacksolidus_c();
    Token*      processBacksolidus_C();
    Token*      processBacksolidus_i();
    Token*      processBacksolidus_I();
    Token*      processBacksolidus_g();
    Token*      processBacksolidus_X();
    Token*      processBacksolidus_lt();
    Token*      processBacksolidus_gt();
    Token*      processStar(Token* const tok);
    Token*      processPlus(Token* const tok);
	Token*      processQuestion(Token* const tok);
	Token*      processParen();
	Token*      processParen2();
	Token*      processCondition();
	Token*      processModifiers();
	Token*      processIndependent();
	Token*      processBackReference();
	RangeToken* parseCharacterClass(const bool useNRange);
	RangeToken* parseSetOperations();

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
	Token* getTokenForShorthand(const XMLInt32 ch);

    // -----------------------------------------------------------------------
    //  Helper methods
    // -----------------------------------------------------------------------
    bool checkQuestion(const int off);
	XMLInt32 decodeEscaped();

private:
	// -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    ParserForXMLSchema(const ParserForXMLSchema&);
    ParserForXMLSchema& operator=(const ParserForXMLSchema&);

    // -----------------------------------------------------------------------
    //  Private data members
	// -----------------------------------------------------------------------
};

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file ParserForXMLSchema.hpp
  */
