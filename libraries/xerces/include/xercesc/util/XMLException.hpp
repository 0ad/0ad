/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2000 The Apache Software Foundation.  All rights
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
 * originally based on software copyright (c) 1999, International
 * Business Machines, Inc., http://www.ibm.com .  For more information
 * on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * $Id: XMLException.hpp,v 1.7 2003/12/19 23:02:25 cargilld Exp $
 */

#if !defined(EXCEPTION_HPP)
#define EXCEPTION_HPP

#include <xercesc/util/XMemory.hpp>
#include <xercesc/util/XMLExceptMsgs.hpp>
#include <xercesc/util/XMLUni.hpp>
#include <xercesc/framework/XMLErrorReporter.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  This is the base class from which all the XML parser exceptions are
//  derived. The virtual interface is very simple and most of the functionality
//  is in this class.
//
//  Because all derivatives are EXACTLY the same except for the static
//  string that is used to hold the name of the class, a macro is provided
//  below via which they are all created.
// ---------------------------------------------------------------------------
class XMLUTIL_EXPORT XMLException : public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Virtual Destructor
    // -----------------------------------------------------------------------
    virtual ~XMLException();


    // -----------------------------------------------------------------------
    //  The XML exception virtual interface
    // -----------------------------------------------------------------------
    virtual const XMLCh* getType() const = 0;


    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    XMLExcepts::Codes getCode() const;
    const XMLCh* getMessage() const;
    const char* getSrcFile() const;
    unsigned int getSrcLine() const;
    XMLErrorReporter::ErrTypes getErrorType() const;


    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    void setPosition(const char* const file, const unsigned int line);


    // -----------------------------------------------------------------------
    //  Hidden constructors and operators
    //
    //  NOTE:   Technically, these should be protected, since this is a
    //          base class that is never used directly. However, VC++ 6.0 will
    //          fail to catch via a reference to base class if the ctors are
    //          not public!! This seems to have been caused by the install
    //          of IE 5.0.
    // -----------------------------------------------------------------------
    XMLException();
    XMLException(const char* const srcFile, const unsigned int srcLine, MemoryManager* const memoryManager = 0);
    XMLException(const XMLException& toCopy);
    XMLException& operator=(const XMLException& toAssign);

    // -----------------------------------------------------------------------
    //  Notification that lazy data has been deleted
    // -----------------------------------------------------------------------
	static void reinitMsgMutex();

	static void reinitMsgLoader();

protected :
    // -----------------------------------------------------------------------
    //  Protected methods
    // -----------------------------------------------------------------------
    void loadExceptText
    (
        const   XMLExcepts::Codes toLoad
    );
    void loadExceptText
    (
        const   XMLExcepts::Codes toLoad
        , const XMLCh* const        text1
        , const XMLCh* const        text2 = 0
        , const XMLCh* const        text3 = 0
        , const XMLCh* const        text4 = 0
    );
    void loadExceptText
    (
        const   XMLExcepts::Codes toLoad
        , const char* const         text1
        , const char* const         text2 = 0
        , const char* const         text3 = 0
        , const char* const         text4 = 0
    );


private :
    // -----------------------------------------------------------------------
    //  Data members
    //
    //  fCode
    //      The error code that this exception represents.
    //
    //  fSrcFile
    //  fSrcLine
    //      These are the file and line information from the source where the
    //      exception was thrown from.
    //
    //  fMsg
    //      The loaded message text for this exception.
    // -----------------------------------------------------------------------
    XMLExcepts::Codes       fCode;
    char*                   fSrcFile;
    unsigned int            fSrcLine;
    XMLCh*                  fMsg;

protected:
    MemoryManager*          fMemoryManager;
};

// ---------------------------------------------------------------------------
//  XMLException: Getter methods
// ---------------------------------------------------------------------------
inline XMLExcepts::Codes XMLException::getCode() const
{
    return fCode;
}

inline const XMLCh* XMLException::getMessage() const
{
    return fMsg;
}

inline const char* XMLException::getSrcFile() const
{
    if (!fSrcFile)
        return "";
    return fSrcFile;
}

inline unsigned int XMLException::getSrcLine() const
{
    return fSrcLine;
}

inline XMLErrorReporter::ErrTypes XMLException::getErrorType() const
{
   if ((fCode >= XMLExcepts::W_LowBounds) && (fCode <= XMLExcepts::W_HighBounds))
       return XMLErrorReporter::ErrType_Warning;
   else if ((fCode >= XMLExcepts::F_LowBounds) && (fCode <= XMLExcepts::F_HighBounds))
        return XMLErrorReporter::ErrType_Fatal;
   else if ((fCode >= XMLExcepts::E_LowBounds) && (fCode <= XMLExcepts::E_HighBounds))
        return XMLErrorReporter::ErrType_Error;
   return XMLErrorReporter::ErrTypes_Unknown;
}

// ---------------------------------------------------------------------------
//  This macro is used to create derived classes. They are all identical
//  except the name of the exception, so it crazy to type them in over and
//  over.
// ---------------------------------------------------------------------------
#define MakeXMLException(theType, expKeyword) \
class expKeyword theType : public XMLException \
{ \
public: \
 \
    theType(const   char* const         srcFile \
            , const unsigned int        srcLine \
            , const XMLExcepts::Codes toThrow \
            , MemoryManager*            memoryManager = 0) : \
        XMLException(srcFile, srcLine, memoryManager) \
    { \
        loadExceptText(toThrow); \
    } \
 \
    theType(const theType& toCopy) : \
 \
        XMLException(toCopy) \
    { \
    } \
  \
    theType(const   char* const         srcFile \
            , const unsigned int        srcLine \
            , const XMLExcepts::Codes   toThrow \
            , const XMLCh* const        text1 \
            , const XMLCh* const        text2 = 0 \
            , const XMLCh* const        text3 = 0 \
            , const XMLCh* const        text4 = 0 \
            , MemoryManager*            memoryManager = 0) : \
        XMLException(srcFile, srcLine, memoryManager) \
    { \
        loadExceptText(toThrow, text1, text2, text3, text4); \
    } \
 \
    theType(const   char* const         srcFile \
            , const unsigned int        srcLine \
            , const XMLExcepts::Codes   toThrow \
            , const char* const         text1 \
            , const char* const         text2 = 0 \
            , const char* const         text3 = 0 \
            , const char* const         text4 = 0 \
            , MemoryManager*            memoryManager = 0) : \
        XMLException(srcFile, srcLine, memoryManager) \
    { \
        loadExceptText(toThrow, text1, text2, text3, text4); \
    } \
 \
    virtual ~theType() {} \
 \
    theType& operator=(const theType& toAssign) \
    { \
        XMLException::operator=(toAssign); \
        return *this; \
    } \
 \
    virtual XMLException* duplicate() const \
    { \
        return new (fMemoryManager) theType(*this); \
    } \
 \
    virtual const XMLCh* getType() const \
    { \
        return XMLUni::fg##theType##_Name; \
    } \
 \
private : \
    theType(); \
};



// ---------------------------------------------------------------------------
//  This macros is used to actually throw an exception. It is used in order
//  to make sure that source code line/col info is stored correctly, and to
//  give flexibility for other stuff in the future.
// ---------------------------------------------------------------------------

#define ThrowXML(type,code) throw type(__FILE__, __LINE__, code)

#define ThrowXML1(type,code,p1) throw type(__FILE__, __LINE__, code, p1)

#define ThrowXML2(type,code,p1,p2) throw type(__FILE__, __LINE__, code, p1, p2)

#define ThrowXML3(type,code,p1,p2,p3) throw type(__FILE__, __LINE__, code, p1, p2, p3)

#define ThrowXML4(type,code,p1,p2,p3,p4) throw type(__FILE__, __LINE__, code, p1, p2, p3, p4)

#define ThrowXMLwithMemMgr(type,code,memMgr) throw type(__FILE__, __LINE__, code, memMgr)

#define ThrowXMLwithMemMgr1(type,code,p1,memMgr) throw type(__FILE__, __LINE__, code, p1, 0, 0, 0, memMgr)

#define ThrowXMLwithMemMgr2(type,code,p1,p2,memMgr) throw type(__FILE__, __LINE__, code, p1, p2, 0, 0, memMgr)

#define ThrowXMLwithMemMgr3(type,code,p1,p2,p3,memMgr) throw type(__FILE__, __LINE__, code, p1, p2, p3, 0, memMgr)

#define ThrowXMLwithMemMgr4(type,code,p1,p2,p3,p4,memMgr) throw type(__FILE__, __LINE__, code, p1, p2, p3, p4, memMgr)

XERCES_CPP_NAMESPACE_END

#endif
