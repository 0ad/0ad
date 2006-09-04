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
 * $Id: XMLDateTime.hpp 225121 2005-07-25 13:41:47Z cargilld $
 */

#ifndef XML_DATETIME_HPP
#define XML_DATETIME_HPP

#include <xercesc/util/XMLNumber.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/XMLUniDefs.hpp>
#include <xercesc/util/SchemaDateTimeException.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XSValue;

class XMLUTIL_EXPORT XMLDateTime : public XMLNumber
{
public:

	enum valueIndex
    {
        CentYear   = 0,
        Month      ,
        Day        ,
        Hour       ,
        Minute     ,
        Second     ,
        MiliSecond ,  //not to be used directly
        utc        ,
        TOTAL_SIZE
    };

    enum utcType
    {
        UTC_UNKNOWN = 0,
        UTC_STD        ,          // set in parse() or normalize()
        UTC_POS        ,          // set in parse()
        UTC_NEG                   // set in parse()
    };

    // -----------------------------------------------------------------------
    // ctors and dtor
    // -----------------------------------------------------------------------

    XMLDateTime(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    XMLDateTime(const XMLCh* const,
                MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    ~XMLDateTime();

    inline void           setBuffer(const XMLCh* const);

    // -----------------------------------------------------------------------
    // Copy ctor and Assignment operators
    // -----------------------------------------------------------------------

    XMLDateTime(const XMLDateTime&);

    XMLDateTime&          operator=(const XMLDateTime&);

    // -----------------------------------------------------------------------
    // Implementation of Abstract Interface
    // -----------------------------------------------------------------------

    /**
     *
     *  Deprecated: please use getRawData
     *
     */
    virtual XMLCh*        toString() const;
    
    virtual XMLCh*        getRawData() const;

    virtual const XMLCh*  getFormattedString() const;

    virtual int           getSign() const;

    // -----------------------------------------------------------------------
    // Canonical Representation
    // -----------------------------------------------------------------------

    XMLCh*                getDateTimeCanonicalRepresentation(MemoryManager* const memMgr) const;

    XMLCh*                getTimeCanonicalRepresentation(MemoryManager* const memMgr)     const;

    XMLCh*                getDateCanonicalRepresentation(MemoryManager* const memMgr)     const;

    // -----------------------------------------------------------------------
    // parsers
    // -----------------------------------------------------------------------

    void                  parseDateTime();       //DateTime

    void                  parseDate();           //Date

    void                  parseTime();           //Time

    void                  parseDay();            //gDay

    void                  parseMonth();          //gMonth

    void                  parseYear();           //gYear

    void                  parseMonthDay();       //gMonthDay

    void                  parseYearMonth();      //gYearMonth

    void                  parseDuration();       //duration

    // -----------------------------------------------------------------------
    // Comparison
    // -----------------------------------------------------------------------
    static int            compare(const XMLDateTime* const
                                , const XMLDateTime* const);

    static int            compare(const XMLDateTime* const
                                , const XMLDateTime* const
                                , bool                    );

    static int            compareOrder(const XMLDateTime* const
                                     , const XMLDateTime* const);                                    

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(XMLDateTime)

private:

    // -----------------------------------------------------------------------
    // Constant data
    // -----------------------------------------------------------------------
	//

    enum timezoneIndex
    {
        hh = 0,
        mm ,
        TIMEZONE_ARRAYSIZE
    };

    // -----------------------------------------------------------------------
    // Comparison
    // -----------------------------------------------------------------------
    static int            compareResult(int
                                      , int
                                      , bool);

    static void           addDuration(XMLDateTime*             pDuration
                                    , const XMLDateTime* const pBaseDate
                                    , int                      index);


    static int            compareResult(const XMLDateTime* const
                                      , const XMLDateTime* const
                                      , bool
                                      , int);

    static inline int     getRetVal(int, int);

    // -----------------------------------------------------------------------
    // helper
    // -----------------------------------------------------------------------

    inline  void          reset();

    inline  void          assertBuffer()               const;

    inline  void          copy(const XMLDateTime&);

    // allow multiple parsing
    inline  void          initParser();

    inline  bool          isNormalized()               const;

    // -----------------------------------------------------------------------
    // scaners
    // -----------------------------------------------------------------------

    void                  getDate();

    void                  getTime();

    void                  getYearMonth();

    void                  getTimeZone(const int);

    void                  parseTimeZone();

    // -----------------------------------------------------------------------
    // locator and converter
    // -----------------------------------------------------------------------

    int                   findUTCSign(const int start);

    int                   indexOf(const int start
                                , const int end
                                , const XMLCh ch)     const;

    int                   parseInt(const int start
                                 , const int end)     const;

    int                   parseIntYear(const int end) const;

    double                parseMiliSecond(const int start
                                        , const int end) const;

    // -----------------------------------------------------------------------
    // validator and normalizer
    // -----------------------------------------------------------------------

    void                  validateDateTime()          const;

    void                  normalize();

    void                  fillString(XMLCh*& ptr, int value, int expLen) const;

    int                   fillYearString(XMLCh*& ptr, int value) const;

    void                  searchMiliSeconds(XMLCh*& miliStartPtr, XMLCh*& miliEndPtr) const;

    // -----------------------------------------------------------------------
    // Unimplemented operator ==
    // -----------------------------------------------------------------------
	bool operator==(const XMLDateTime& toCompare) const;


    // -----------------------------------------------------------------------
    //  Private data members
    //
    //     fValue[]
    //          object representation of date time.
    //
    //     fTimeZone[]
    //          temporary storage for normalization
    //
    //     fStart, fEnd
    //          pointers to the portion of fBuffer being parsed
    //
    //     fBuffer
    //          raw data to be parsed, own it.
    //
    // -----------------------------------------------------------------------

    int          fValue[TOTAL_SIZE];
    int          fTimeZone[TIMEZONE_ARRAYSIZE];
    int          fStart;
    int          fEnd;
    int          fBufferMaxLen;

    double       fMiliSecond;
    bool         fHasTime;

    XMLCh*       fBuffer;
    MemoryManager* fMemoryManager;

    friend class XSValue;
};

inline void XMLDateTime::setBuffer(const XMLCh* const aString)
{
    reset();

    fEnd = XMLString::stringLen(aString);
    if (fEnd > 0) {
    
        if (fEnd > fBufferMaxLen)
        {
            fMemoryManager->deallocate(fBuffer);
            fBufferMaxLen = fEnd + 8;
            fBuffer = (XMLCh*) fMemoryManager->allocate((fBufferMaxLen+1) * sizeof(XMLCh));
        }

        memcpy(fBuffer, aString, (fEnd+1) * sizeof(XMLCh));
    }
}

inline void XMLDateTime::reset()
{
    for ( int i=0; i < TOTAL_SIZE; i++ )
        fValue[i] = 0;

    fMiliSecond   = 0;
    fHasTime      = false;
    fTimeZone[hh] = fTimeZone[mm] = 0;
    fStart = fEnd = 0;

    if (fBuffer)
        *fBuffer = 0;
}

inline void XMLDateTime::copy(const XMLDateTime& rhs)
{
    for ( int i = 0; i < TOTAL_SIZE; i++ )
        fValue[i] = rhs.fValue[i];

    fMiliSecond   = rhs.fMiliSecond;
    fHasTime      = rhs.fHasTime;
    fTimeZone[hh] = rhs.fTimeZone[hh];
    fTimeZone[mm] = rhs.fTimeZone[mm];
    fStart = rhs.fStart;
    fEnd   = rhs.fEnd;

    if (fEnd > 0)
    {
        if (fEnd > fBufferMaxLen)
        {
            fMemoryManager->deallocate(fBuffer);//delete[] fBuffer;
            fBufferMaxLen = rhs.fBufferMaxLen;
            fBuffer = (XMLCh*) fMemoryManager->allocate((fBufferMaxLen+1) * sizeof(XMLCh));
        }

        memcpy(fBuffer, rhs.fBuffer, (fEnd+1) * sizeof(XMLCh));
    }
}

inline void XMLDateTime::assertBuffer() const
{
    if ( ( !fBuffer )            ||
         ( fBuffer[0] == chNull ) )
    {
        ThrowXMLwithMemMgr(SchemaDateTimeException
               , XMLExcepts::DateTime_Assert_Buffer_Fail
               , fMemoryManager);
    }

}

inline void XMLDateTime::initParser()
{
    assertBuffer();
    fStart = 0;   // to ensure scan from the very first beginning
                  // in case the pointer is updated accidentally by someone else.
}

inline bool XMLDateTime::isNormalized() const
{
    return ( fValue[utc] == UTC_STD ? true : false );
}

inline int XMLDateTime::getRetVal(int c1, int c2)
{
    if ((c1 == LESS_THAN    && c2 == GREATER_THAN) ||
        (c1 == GREATER_THAN && c2 == LESS_THAN)      )
    {
        return INDETERMINATE;
    }

    return ( c1 != INDETERMINATE ) ? c1 : c2;
}

XERCES_CPP_NAMESPACE_END

#endif
