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
 * $Log: Win32TransService.hpp,v $
 * Revision 1.7  2003/12/24 15:24:15  cargilld
 * More updates to memory management so that the static memory manager.
 *
 * Revision 1.6  2003/05/17 16:32:18  knoaman
 * Memory manager implementation : transcoder update.
 *
 * Revision 1.5  2003/05/15 18:47:07  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.4  2003/03/07 18:15:58  tng
 * Return a reference instead of void for operator=
 *
 * Revision 1.3  2002/11/04 15:14:34  tng
 * C++ Namespace Support.
 *
 * Revision 1.2  2002/04/09 15:44:00  knoaman
 * Add lower case string support.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:37  peiyongz
 * sane_include
 *
 * Revision 1.10  2000/05/09 00:22:45  andyh
 * Memory Cleanup.  XMLPlatformUtils::Terminate() deletes all lazily
 * allocated memory; memory leak checking tools will no longer report
 * that leaks exist.  (DOM GetElementsByTagID temporarily removed
 * as part of this.)
 *
 * Revision 1.9  2000/03/18 00:00:04  roddey
 * Initial updates for two way transcoding support
 *
 * Revision 1.8  2000/03/07 23:45:36  roddey
 * First cut for additions to Win32 xcode. Based very loosely on a
 * prototype from Eric Ulevik.
 *
 * Revision 1.7  2000/03/02 19:55:36  roddey
 * This checkin includes many changes done while waiting for the
 * 1.1.0 code to be finished. I can't list them all here, but a list is
 * available elsewhere.
 *
 * Revision 1.6  2000/02/06 07:48:34  rahulj
 * Year 2K copyright swat.
 *
 * Revision 1.5  2000/01/25 22:49:58  roddey
 * Moved the supportsSrcOfs() method from the individual transcoder to the
 * transcoding service, where it should have been to begin with.
 *
 * Revision 1.4  2000/01/25 19:19:09  roddey
 * Simple addition of a getId() method to the xcode and netacess abstractions to
 * allow each impl to give back an id string.
 *
 * Revision 1.3  1999/12/18 00:22:33  roddey
 * Changes to support the new, completely orthagonal, transcoder architecture.
 *
 * Revision 1.2  1999/12/15 19:44:02  roddey
 * Now implements the new transcoding abstractions, with separate interface
 * classes for XML transcoders and local code page transcoders.
 *
 * Revision 1.1.1.1  1999/11/09 01:06:06  twl
 * Initial checkin
 *
 * Revision 1.2  1999/11/08 20:45:35  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */

#ifndef WIN32TRANSSERVICE_HPP
#define WIN32TRANSSERVICE_HPP

#include <xercesc/util/TransService.hpp>
#include <xercesc/util/RefHashTableOf.hpp>
#include <windows.h>

XERCES_CPP_NAMESPACE_BEGIN

class CPMapEntry;



//---------------------------------------------------------------------------
//
//  class Win32TransService
//
//---------------------------------------------------------------------------
class XMLUTIL_EXPORT Win32TransService : public XMLTransService
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    Win32TransService();
    virtual ~Win32TransService();


    // -----------------------------------------------------------------------
    //  Implementation of the virtual transcoding service API
    // -----------------------------------------------------------------------
    virtual int compareIString
    (
        const   XMLCh* const    comp1
        , const XMLCh* const    comp2
    );

    virtual int compareNIString
    (
        const   XMLCh* const    comp1
        , const XMLCh* const    comp2
        , const unsigned int    maxChars
    );

    virtual const XMLCh* getId() const;

    virtual bool isSpace(const XMLCh toCheck) const;

    virtual XMLLCPTranscoder* makeNewLCPTranscoder();

    virtual bool supportsSrcOfs() const;

    virtual void upperCase(XMLCh* const toUpperCase) const;
    virtual void lowerCase(XMLCh* const toLowerCase) const;


protected :
    // -----------------------------------------------------------------------
    //  Protected virtual methods, implemented in Win32TransService2.cpp
    // -----------------------------------------------------------------------
    virtual XMLTranscoder* makeNewXMLTranscoder
    (
        const   XMLCh* const            encodingName
        ,       XMLTransService::Codes& resValue
        , const unsigned int            blockSize
        ,       MemoryManager* const    manager
    );


private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    Win32TransService(const Win32TransService&);
    Win32TransService& operator=(const Win32TransService&);

    //      This is a hash table of entries which map encoding names to their
    //      Windows specific code pages. The code page allows us to create
    //      transcoders for those encodings. The encoding names come from XML
    //      files.
    //
    //      This map is shared unsynchronized among all threads of the process,
    //      which is cool since it will be read only once its initialized.



    static bool isAlias(const   HKEY            encodingKey
                    ,       char* const     aliasBuf = 0
                    , const unsigned int    nameBufSz = 0);


    RefHashTableOf<CPMapEntry>    *fCPMap;
};







//---------------------------------------------------------------------------
//
//  class Win32Transcoder
//
//---------------------------------------------------------------------------

class XMLUTIL_EXPORT Win32Transcoder : public XMLTranscoder
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    Win32Transcoder
    (
        const   XMLCh* const    encodingName
        , const unsigned int    winCP
        , const unsigned int    ieCP
        , const unsigned int    blockSize
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager);
    ~Win32Transcoder();


    // -----------------------------------------------------------------------
    //  Implementation of the virtual transcoder interface
    // -----------------------------------------------------------------------
    virtual unsigned int transcodeFrom
    (
        const   XMLByte* const          srcData
        , const unsigned int            srcCount
        ,       XMLCh* const            toFill
        , const unsigned int            maxChars
        ,       unsigned int&           bytesEaten
        ,       unsigned char* const    charSizes
    );

    virtual unsigned int transcodeTo
    (
        const   XMLCh* const    srcData
        , const unsigned int    srcCount
        ,       XMLByte* const  toFill
        , const unsigned int    maxBytes
        ,       unsigned int&   charsEaten
        , const UnRepOpts       options
    );

    virtual bool canTranscodeTo
    (
        const   unsigned int    toCheck
    )   const;


private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    Win32Transcoder(const Win32Transcoder&);
    Win32Transcoder& operator=(const Win32Transcoder&);


    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fIECP
    //      This is the internet explorer code page for this encoding.
    //
    //  fWinCP
    //      This is the windows code page for this encoding.
    // -----------------------------------------------------------------------
    unsigned int    fIECP;
    unsigned int    fWinCP;
};





//---------------------------------------------------------------------------
//
//  class Win32LCPTranscoder
//
//---------------------------------------------------------------------------
class XMLUTIL_EXPORT Win32LCPTranscoder : public XMLLCPTranscoder
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    Win32LCPTranscoder();
    ~Win32LCPTranscoder();


    // -----------------------------------------------------------------------
    //  Implementation of the virtual transcoder interface
    // -----------------------------------------------------------------------
    virtual unsigned int calcRequiredSize(const char* const srcText
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    virtual unsigned int calcRequiredSize(const XMLCh* const srcText
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    virtual char* transcode(const XMLCh* const toTranscode);
    virtual char* transcode(const XMLCh* const toTranscode,
                            MemoryManager* const manager);

    virtual XMLCh* transcode(const char* const toTranscode);
    virtual XMLCh* transcode(const char* const toTranscode,
                             MemoryManager* const manager);

    virtual bool transcode
    (
        const   char* const     toTranscode
        ,       XMLCh* const    toFill
        , const unsigned int    maxChars
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    );

    virtual bool transcode
    (
        const   XMLCh* const    toTranscode
        ,       char* const     toFill
        , const unsigned int    maxChars
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    );


private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    Win32LCPTranscoder(const Win32LCPTranscoder&);
    Win32LCPTranscoder& operator=(const Win32LCPTranscoder&);
};

XERCES_CPP_NAMESPACE_END

#endif
