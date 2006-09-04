/*
 * Copyright 2001-2002,2004 The Apache Software Foundation.
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
 * $Id: RangeToken.hpp 176399 2005-05-05 01:12:24Z dbertoni $
 */

#if !defined(RANGETOKEN_HPP)
#define RANGETOKEN_HPP

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/regx/Token.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Forward Declaration
// ---------------------------------------------------------------------------
class TokenFactory;


class XMLUTIL_EXPORT RangeToken : public Token {
public:
    // -----------------------------------------------------------------------
    //  Public Constructors and Destructor
    // -----------------------------------------------------------------------
    RangeToken(const unsigned short tokType,
               MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    ~RangeToken();

    // -----------------------------------------------------------------------
    //  Public Constants
    // -----------------------------------------------------------------------
    static const int MAPSIZE;
    static const unsigned int INITIALSIZE;

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    RangeToken* getCaseInsensitiveToken(TokenFactory* const tokFactory);

    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    void setRangeValues(XMLInt32* const rangeValues, const unsigned int count);

    // -----------------------------------------------------------------------
    //  Range manipulation methods
    // -----------------------------------------------------------------------
    void addRange(const XMLInt32 start, const XMLInt32 end);
    void mergeRanges(const Token *const tok);
    void sortRanges();
    void compactRanges();
    void subtractRanges(RangeToken* const tok);
    void intersectRanges(RangeToken* const tok);
    static Token* complementRanges(RangeToken* const tok,
                                   TokenFactory* const tokFactory,
                                   MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    // -----------------------------------------------------------------------
    //  Match methods
    // -----------------------------------------------------------------------
    bool match(const XMLInt32 ch);

    // -----------------------------------------------------------------------
    //  Creates the map.  This will happen automatically,
    //  necessary.
    // -----------------------------------------------------------------------
    void
    createMap();

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    RangeToken(const RangeToken&);
    RangeToken& operator=(const RangeToken&);

    // -----------------------------------------------------------------------
    // Private Helper methods
    // -----------------------------------------------------------------------
    void expand(const unsigned int length);

    void doCreateMap();

    // -----------------------------------------------------------------------
    //  Private data members
    // -----------------------------------------------------------------------
    bool         fSorted;
    bool         fCompacted;
    int          fNonMapIndex;
    unsigned int fElemCount;
    unsigned int fMaxCount;
    int*         fMap;
    XMLInt32*    fRanges;
    RangeToken*  fCaseIToken;
    MemoryManager* fMemoryManager;
};


inline void RangeToken::createMap()
{
    if (!fMap)
    {
        doCreateMap();
    }
}


XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file RangeToken.hpp
  */

