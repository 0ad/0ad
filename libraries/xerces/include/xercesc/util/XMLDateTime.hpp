/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2001 The Apache Software Foundation.  All rights
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
 * $Id: XMLDateTime.hpp,v 1.17 2004/01/29 11:48:47 cargilld Exp $
 * $Log: XMLDateTime.hpp,v $
 * Revision 1.17  2004/01/29 11:48:47  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.16  2004/01/13 19:50:56  peiyongz
 * remove parseContent()
 *
 * Revision 1.14  2003/12/31 02:34:11  neilg
 * enable production of canonical representations for dates with negative years, or years >9999
 *
 * Revision 1.13  2003/12/17 20:00:49  cargilld
 * Update for memory management so that the static memory manager (one
 * used to call Initialize) is only for static data.
 *
 * Revision 1.12  2003/12/17 00:18:35  cargilld
 * Update to memory management so that the static memory manager (one used to call Initialize) is only for static data.
 *
 * Revision 1.11  2003/12/11 21:38:12  peiyongz
 * support for Canonical Representation for Datatype
 *
 * Revision 1.10  2003/09/23 18:16:07  peiyongz
 * Inplementation for Serialization/Deserialization
 *
 * Revision 1.9  2003/08/14 02:57:27  knoaman
 * Code refactoring to improve performance of validation.
 *
 * Revision 1.8  2003/05/18 14:02:05  knoaman
 * Memory manager implementation: pass per instance manager.
 *
 * Revision 1.7  2003/05/15 19:07:46  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.6  2003/05/09 15:13:46  peiyongz
 * Deprecated toString() in XMLNumber family
 *
 * Revision 1.5  2003/03/10 20:55:58  peiyongz
 * Schema Errata E2-40 double/float
 *
 * Revision 1.4  2003/02/02 23:54:43  peiyongz
 * getFormattedString() added to return original and converted value.
 *
 * Revision 1.3  2003/01/30 21:55:22  tng
 * Performance: create getRawData which is similar to toString but return the internal data directly, user is not required to delete the returned memory.
 *
 * Revision 1.2  2002/11/04 15:22:05  tng
 * C++ Namespace Support.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:14  peiyongz
 * sane_include
 *
 * Revision 1.4  2001/11/22 20:23:00  peiyongz
 * _declspec(dllimport) and inline warning C4273
 *
 * Revision 1.3  2001/11/12 20:36:54  peiyongz
 * SchemaDateTimeException defined
 *
 * Revision 1.2  2001/11/09 20:41:45  peiyongz
 * Fix: compilation error on Solaris and AIX.
 *
 * Revision 1.1  2001/11/07 19:16:03  peiyongz
 * DateTime Port
 *
 */

#ifndef XML_DATETIME_HPP
#define XML_DATETIME_HPP

#include <xercesc/util/XMLNumber.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/XMLUniDefs.hpp>
#include <xercesc/util/SchemaDateTimeException.hpp>

XERCES_CPP_NAMESPACE_BEGIN

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
        MiliSecond ,
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

    // -----------------------------------------------------------------------
    // validator and normalizer
    // -----------------------------------------------------------------------

    void                  validateDateTime()          const;

    void                  normalize();

    void                  fillString(XMLCh*& ptr, valueIndex ind, int expLen) const;

    int                   fillYearString(XMLCh*& ptr, valueIndex ind) const;

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
    XMLCh*       fBuffer;
    MemoryManager* fMemoryManager;
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

    fTimeZone[hh] = fTimeZone[mm] = 0;
    fStart = fEnd = 0;

    if (fBuffer)
        *fBuffer = 0;
}

inline void XMLDateTime::copy(const XMLDateTime& rhs)
{
    for ( int i = 0; i < TOTAL_SIZE; i++ )
        fValue[i] = rhs.fValue[i];

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
