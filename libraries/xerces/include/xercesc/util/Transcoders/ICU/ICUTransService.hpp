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
 * $Log: ICUTransService.hpp,v $
 * Revision 1.9  2003/12/24 15:24:15  cargilld
 * More updates to memory management so that the static memory manager.
 *
 * Revision 1.8  2003/05/17 16:32:17  knoaman
 * Memory manager implementation : transcoder update.
 *
 * Revision 1.7  2003/05/15 18:47:03  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.6  2003/03/07 18:15:57  tng
 * Return a reference instead of void for operator=
 *
 * Revision 1.5  2002/11/22 14:56:47  tng
 * 390: Uniconv390 support.  Patch by Chris Larsson and Stephen Dulin.
 *
 * Revision 1.4  2002/11/11 14:08:01  tng
 * Fix: UConverter should be declared outside xerces-c++ namespace
 *
 * Revision 1.3  2002/11/04 15:14:33  tng
 * C++ Namespace Support.
 *
 * Revision 1.2  2002/04/09 15:44:00  knoaman
 * Add lower case string support.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:36  peiyongz
 * sane_include
 *
 * Revision 1.10  2000/03/18 00:00:03  roddey
 * Initial updates for two way transcoding support
 *
 * Revision 1.9  2000/03/02 19:55:34  roddey
 * This checkin includes many changes done while waiting for the
 * 1.1.0 code to be finished. I can't list them all here, but a list is
 * available elsewhere.
 *
 * Revision 1.8  2000/02/06 07:48:32  rahulj
 * Year 2K copyright swat.
 *
 * Revision 1.7  2000/01/25 22:49:56  roddey
 * Moved the supportsSrcOfs() method from the individual transcoder to the
 * transcoding service, where it should have been to begin with.
 *
 * Revision 1.6  2000/01/25 19:19:08  roddey
 * Simple addition of a getId() method to the xcode and netacess abstractions to
 * allow each impl to give back an id string.
 *
 * Revision 1.5  2000/01/19 23:21:11  abagchi
 * Made this file compatible with ICU 1.4
 *
 * Revision 1.4  2000/01/19 00:58:07  roddey
 * Update to support new ICU 1.4 release.
 *
 * Revision 1.3  1999/12/18 00:22:32  roddey
 * Changes to support the new, completely orthagonal, transcoder architecture.
 *
 * Revision 1.2  1999/12/15 19:43:45  roddey
 * Now implements the new transcoding abstractions, with separate interface
 * classes for XML transcoders and local code page transcoders.
 *
 * Revision 1.1.1.1  1999/11/09 01:06:08  twl
 * Initial checkin
 *
 * Revision 1.3  1999/11/08 20:45:34  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */

#ifndef ICUTRANSSERVICE_HPP
#define ICUTRANSSERVICE_HPP

#include <xercesc/util/Mutexes.hpp>
#include <xercesc/util/TransService.hpp>

struct UConverter;

XERCES_CPP_NAMESPACE_BEGIN

class XMLUTIL_EXPORT ICUTransService : public XMLTransService
{
public :
    friend class Uniconv390TransService;
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    ICUTransService();
    ~ICUTransService();


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
    //  Protected virtual methods
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
    ICUTransService(const ICUTransService&);
    ICUTransService& operator=(const ICUTransService&);
};



class XMLUTIL_EXPORT ICUTranscoder : public XMLTranscoder
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    ICUTranscoder
    (
        const   XMLCh* const        encodingName
        ,       UConverter* const   toAdopt
        , const unsigned int        blockSize
        , MemoryManager* const      manager = XMLPlatformUtils::fgMemoryManager
    );
    ~ICUTranscoder();


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
    ICUTranscoder();
    ICUTranscoder(const ICUTranscoder&);
    ICUTranscoder& operator=(const ICUTranscoder&);


    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fConverter
    //      This is a pointer to the ICU converter that this transcoder
    //      uses.
    //
    //  fFixed
    //      This is set to true if the encoding is a fixed size one. This
    //      can be used to optimize some operations.
    //
    //  fSrcOffsets
    //      This is an array of longs, which are allocated to the size of
    //      the trancoding block (if any) indicated in the ctor. It is used
    //      to get the character offsets from ICU, which are then translated
    //      into an array of char sizes for return.
    // -----------------------------------------------------------------------
    UConverter*     fConverter;
    bool            fFixed;
    XMLUInt32*      fSrcOffsets;
};


class XMLUTIL_EXPORT ICULCPTranscoder : public XMLLCPTranscoder
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    ICULCPTranscoder(UConverter* const toAdopt);
    ~ICULCPTranscoder();


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
    ICULCPTranscoder();
    ICULCPTranscoder(const ICULCPTranscoder&);
    ICULCPTranscoder& operator=(const ICULCPTranscoder&);


    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fConverter
    //      This is a pointer to the ICU converter that this transcoder
    //      uses.
    //
    //  fMutex
    //      We have to synchronize threaded calls to the converter.
    // -----------------------------------------------------------------------
    UConverter*     fConverter;
    XMLMutex        fMutex;
};

XERCES_CPP_NAMESPACE_END

#endif
