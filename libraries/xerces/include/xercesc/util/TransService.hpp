/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2001 The Apache Software Foundation.  All rights
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
 * $Log: TransService.hpp,v $
 * Revision 1.12  2004/01/29 11:48:46  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.11  2003/12/24 15:24:13  cargilld
 * More updates to memory management so that the static memory manager.
 *
 * Revision 1.10  2003/11/24 19:52:06  neilg
 * allow classes derived from XMLTransService to tailor the intrinsic maps to their taste.
 *
 * Revision 1.9  2003/06/03 18:12:29  knoaman
 * Add default value for memory manager argument.
 *
 * Revision 1.8  2003/05/15 19:07:45  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.7  2003/03/07 18:11:55  tng
 * Return a reference instead of void for operator=
 *
 * Revision 1.6  2003/02/04 22:11:52  peiyongz
 * bug#16784: Obsolete documentation on XMLTranscoder -- reported by
 * Colin Paul Adams, Preston Lancashire
 *
 * Revision 1.5  2002/11/25 21:27:52  tng
 * Performance: use XMLRecognizer::Encodings enum to make new transcode, faster than comparing the encoding string every time.
 *
 * Revision 1.4  2002/11/04 15:22:04  tng
 * C++ Namespace Support.
 *
 * Revision 1.3  2002/07/18 20:05:31  knoaman
 * Add a new feature to control strict IANA encoding name.
 *
 * Revision 1.2  2002/04/09 15:44:00  knoaman
 * Add lower case string support.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:13  peiyongz
 * sane_include
 *
 * Revision 1.14  2001/11/01 23:37:07  jasons
 * 2001-11-01  Jason E. Stewart  <jason@openinformatics.com>
 *
 * 	* src/util/TransService.hpp (Repository):
 * 	Updated Doxygen documentation for XMLTranscoder class
 *
 * Revision 1.13  2001/05/11 13:26:30  tng
 * Copyright update.
 *
 * Revision 1.12  2001/01/25 19:19:32  tng
 * Let user add their encoding to the intrinsic mapping table.  Added by Khaled Noaman.
 *
 * Revision 1.11  2000/04/12 22:57:45  roddey
 * A couple of fixes to comments and parameter names to make them
 * more correct.
 *
 * Revision 1.10  2000/03/28 19:43:19  roddey
 * Fixes for signed/unsigned warnings. New work for two way transcoding
 * stuff.
 *
 * Revision 1.9  2000/03/17 23:59:54  roddey
 * Initial updates for two way transcoding support
 *
 * Revision 1.8  2000/03/02 19:54:46  roddey
 * This checkin includes many changes done while waiting for the
 * 1.1.0 code to be finished. I can't list them all here, but a list is
 * available elsewhere.
 *
 * Revision 1.7  2000/02/24 20:05:25  abagchi
 * Swat for removing Log from API docs
 *
 * Revision 1.6  2000/02/06 07:48:04  rahulj
 * Year 2K copyright swat.
 *
 * Revision 1.5  2000/01/25 22:49:55  roddey
 * Moved the supportsSrcOfs() method from the individual transcoder to the
 * transcoding service, where it should have been to begin with.
 *
 * Revision 1.4  2000/01/25 19:19:07  roddey
 * Simple addition of a getId() method to the xcode and netacess abstractions to
 * allow each impl to give back an id string.
 *
 * Revision 1.3  1999/12/18 00:18:10  roddey
 * More changes to support the new, completely orthagonal support for
 * intrinsic encodings.
 *
 * Revision 1.2  1999/12/15 19:41:28  roddey
 * Support for the new transcoder system, where even intrinsic encodings are
 * done via the same transcoder abstraction as external ones.
 *
 * Revision 1.1.1.1  1999/11/09 01:05:16  twl
 * Initial checkin
 *
 * Revision 1.2  1999/11/08 20:45:16  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */

#ifndef TRANSSERVICE_HPP
#define TRANSSERVICE_HPP

#include <xercesc/util/XMemory.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/XMLRecognizer.hpp>
#include <xercesc/util/RefHashTableOf.hpp>
#include <xercesc/util/RefVectorOf.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// Forward references
//class XMLPlatformUtils;
class XMLLCPTranscoder;
class XMLTranscoder;
class ENameMap;


//
//  This class is an abstract base class which are used to abstract the
//  transcoding services that Xerces uses. The parser's actual transcoding
//  needs are small so it is desirable to allow different implementations
//  to be provided.
//
//  The transcoding service has to provide a couple of required string
//  and character operations, but its most important service is the creation
//  of transcoder objects. There are two types of transcoders, which are
//  discussed below in the XMLTranscoder class' description.
//
class XMLUTIL_EXPORT XMLTransService : public XMemory
{
public :
    // -----------------------------------------------------------------------
    //  Class specific types
    // -----------------------------------------------------------------------
    enum Codes
    {
        Ok
        , UnsupportedEncoding
        , InternalFailure
        , SupportFilesNotFound
    };

    struct TransRec
    {
        XMLCh       intCh;
        XMLByte     extCh;
    };


    // -----------------------------------------------------------------------
    //  Public constructors and destructor
    // -----------------------------------------------------------------------
    virtual ~XMLTransService();


    // -----------------------------------------------------------------------
    //  Non-virtual API
    // -----------------------------------------------------------------------
    XMLTranscoder* makeNewTranscoderFor
    (
        const   XMLCh* const            encodingName
        ,       XMLTransService::Codes& resValue
        , const unsigned int            blockSize
        , MemoryManager* const          manager = XMLPlatformUtils::fgMemoryManager
    );

    XMLTranscoder* makeNewTranscoderFor
    (
        const   char* const             encodingName
        ,       XMLTransService::Codes& resValue
        , const unsigned int            blockSize
        , MemoryManager* const          manager = XMLPlatformUtils::fgMemoryManager
    );

    XMLTranscoder* makeNewTranscoderFor
    (
        XMLRecognizer::Encodings        encodingEnum
        ,       XMLTransService::Codes& resValue
        , const unsigned int            blockSize
        , MemoryManager* const          manager = XMLPlatformUtils::fgMemoryManager
    );


    // -----------------------------------------------------------------------
    //  The virtual transcoding service API
    // -----------------------------------------------------------------------
    virtual int compareIString
    (
        const   XMLCh* const    comp1
        , const XMLCh* const    comp2
    ) = 0;

    virtual int compareNIString
    (
        const   XMLCh* const    comp1
        , const XMLCh* const    comp2
        , const unsigned int    maxChars
    ) = 0;

    virtual const XMLCh* getId() const = 0;

    virtual bool isSpace(const XMLCh toCheck) const = 0;

    virtual XMLLCPTranscoder* makeNewLCPTranscoder() = 0;

    virtual bool supportsSrcOfs() const = 0;

    virtual void upperCase(XMLCh* const toUpperCase) const = 0;
    virtual void lowerCase(XMLCh* const toLowerCase) const = 0;

	// -----------------------------------------------------------------------
    //	Allow users to add their own encodings to the intrinsinc mapping
	//	table
	//	Usage:
	//		XMLTransService::addEncoding (
	//			gMyEncodingNameString
    //			, new ENameMapFor<MyTransClassType>(gMyEncodingNameString)
	//		);
    // -----------------------------------------------------------------------
	static void addEncoding(const XMLCh* const encoding, ENameMap* const ownMapping);


protected :
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    XMLTransService();


    // -----------------------------------------------------------------------
    //  Protected virtual methods.
    // -----------------------------------------------------------------------
    virtual XMLTranscoder* makeNewXMLTranscoder
    (
        const   XMLCh* const            encodingName
        ,       XMLTransService::Codes& resValue
        , const unsigned int            blockSize
        , MemoryManager* const          manager
    ) = 0;

    // -----------------------------------------------------------------------
    //  Protected init method for platform utils to call
    // -----------------------------------------------------------------------
    friend class XMLPlatformUtils;
    virtual void initTransService();

    // -----------------------------------------------------------------------
    // protected static members
    //  gMappings
    //      This is a hash table of ENameMap objects. It is created and filled
    //      in when the platform init calls our initTransService() method.
    //
    //  gMappingsRecognizer
    //      This is an array of ENameMap objects, predefined for those
    //      already recognized by XMLRecognizer::Encodings.
    //

    static RefHashTableOf<ENameMap>*    gMappings;
    static RefVectorOf<ENameMap>*       gMappingsRecognizer;

private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XMLTransService(const XMLTransService&);
    XMLTransService& operator=(const XMLTransService&);

    // -----------------------------------------------------------------------
    //  Hidden method to enable/disable strict IANA encoding check
    //  Caller: XMLPlatformUtils
    // -----------------------------------------------------------------------
    void strictIANAEncoding(const bool newState);
    bool isStrictIANAEncoding();
    static void reinitMappings();
    static void reinitMappingsRecognizer();

};



/**
  * <code>DOMString</code> is the generic string class that stores all strings
  * used in the DOM C++ API.
  *
  * Though this class supports most of the common string operations to manipulate
  * strings, it is not meant to be a comphrehensive string class.
  */

/**
  *   <code>XMLTranscoder</code> is for transcoding non-local code
  *   page encodings, i.e.  named encodings. These are used internally
  *   by the scanner to internalize raw XML into the internal Unicode
  *   format, and by writer classes to convert that internal Unicode
  *   format (which comes out of the parser) back out to a format that
  *   the receiving client code wants to use.
  */
class XMLUTIL_EXPORT XMLTranscoder : public XMemory
{
public :

	/**
	 * This enum is used by the <code>transcodeTo()</code> method
	 * to indicate how to react to unrepresentable characters. The
	 * <code>transcodeFrom()</code> method always works the
	 * same. It will consider any invalid data to be an error and
	 * throw.
	 */
    enum UnRepOpts
    {
        UnRep_Throw		/**< Throw an exception */
        , UnRep_RepChar		/**< Use the replacement char */
    };


	/** @name Destructor. */
	//@{

	 /**
	  * Destructor for XMLTranscoder
	  *
	  */
    virtual ~XMLTranscoder();
	//@}



    /** @name The virtual transcoding interface */
    //@{

    /** Converts from the encoding of the service to the internal XMLCh* encoding
      *
      * @param srcData the source buffer to be transcoded
      * @param srcCount number of characters in the source buffer
      * @param toFill the destination buffer
      * @param maxChars the max number of characters in the destination buffer
      * @param bytesEaten after transcoding, this will hold the number of bytes
      *    that were processed from the source buffer
      * @param charSizes an array which must be at least as big as maxChars
      *    into which will be inserted values that indicate how many
      *    bytes from the input went into each XMLCh that was created
      *    into toFill. Since many encodings use variable numbers of
      *    byte per character, this provides a means to find out what
      *    bytes in the input went into making a particular output
      *    UTF-16 character.
      * @return Returns the number of chars put into the target buffer
      */


    virtual unsigned int transcodeFrom
    (
        const   XMLByte* const          srcData
        , const unsigned int            srcCount
        ,       XMLCh* const            toFill
        , const unsigned int            maxChars
        ,       unsigned int&           bytesEaten
        ,       unsigned char* const    charSizes
    ) = 0;

    /** Converts from the internal XMLCh* encoding to the encoding of the service
      *
      * @param srcData    the source buffer to be transcoded
      * @param srcCount   number of characters in the source buffer
      * @param toFill     the destination buffer
      * @param maxBytes   the max number of bytes in the destination buffer
      * @param charsEaten after transcoding, this will hold the number of chars
      *    that were processed from the source buffer
      * @param options    options to pass to the transcoder that explain how to
      *    respond to an unrepresentable character
      * @return Returns the number of chars put into the target buffer
      */

    virtual unsigned int transcodeTo
    (
        const   XMLCh* const    srcData
        , const unsigned int    srcCount
        ,       XMLByte* const  toFill
        , const unsigned int    maxBytes
        ,       unsigned int&   charsEaten
        , const UnRepOpts       options
    ) = 0;

    /** Query whether the transcoder can handle a given character
      *
      * @param toCheck   the character code point to check
      */

    virtual bool canTranscodeTo
    (
        const   unsigned int    toCheck
    )   const = 0;

    //@}

    /** @name Getter methods */
    //@{

    /** Get the internal block size
     *
       * @return The block size indicated in the constructor.
       */
    unsigned int getBlockSize() const;

    /** Get the encoding name
      *
      * @return the name of the encoding that this
      *    <code>XMLTranscoder</code> object is for
      */
    const XMLCh* getEncodingName() const;
	//@}

    /** @name Getter methods*/
    //@{

    /** Get the plugged-in memory manager
      *
      * This method returns the plugged-in memory manager user for dynamic
      * memory allocation/deallocation.
      *
      * @return the plugged-in memory manager
      */
    MemoryManager* getMemoryManager() const;

	//@}

protected :
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    XMLTranscoder
    (
        const   XMLCh* const    encodingName
        , const unsigned int    blockSize
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    );


    // -----------------------------------------------------------------------
    //  Protected helper methods
    // -----------------------------------------------------------------------
    // As the body of this function is commented out it could be removed.
    // However, currently all calls to it are guarded by #if defined(XERCES_DEBUG)
    // so will leave it for now.
    void checkBlockSize(const unsigned int toCheck);


private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XMLTranscoder(const XMLTranscoder&);
    XMLTranscoder& operator=(const XMLTranscoder&);

    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fBlockSize
    //      This is the block size indicated in the constructor.
    //
    //  fEncodingName
    //      This is the name of the encoding this encoder is for. All basic
    //      XML transcoder's are for named encodings.
    // -----------------------------------------------------------------------
    unsigned int    fBlockSize;
    XMLCh*          fEncodingName;
    MemoryManager*  fMemoryManager;
};


//
//  This class is a specialized transcoder that only transcodes between
//  the internal XMLCh format and the local code page. It is specialized
//  for the very common job of translating data from the client app's
//  native code page to the internal format and vice versa.
//
class XMLUTIL_EXPORT XMLLCPTranscoder : public XMemory
{
public :
    // -----------------------------------------------------------------------
    //  Public constructors and destructor
    // -----------------------------------------------------------------------
    virtual ~XMLLCPTranscoder();


    // -----------------------------------------------------------------------
    //  The virtual transcoder API
    //
    //  NOTE:   All these APIs don't include null terminator characters in
    //          their parameters. So calcRequiredSize() returns the number
    //          of actual chars, not including the null. maxBytes and maxChars
    //          parameters refer to actual chars, not including the null so
    //          its assumed that the buffer is physically one char or byte
    //          larger.
    // -----------------------------------------------------------------------
    virtual unsigned int calcRequiredSize(const char* const srcText
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager) = 0;

    virtual unsigned int calcRequiredSize(const XMLCh* const srcText
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager) = 0;

    virtual char* transcode(const XMLCh* const toTranscode) = 0;
    virtual char* transcode(const XMLCh* const toTranscode,
                            MemoryManager* const manager) = 0;

    virtual XMLCh* transcode(const char* const toTranscode) = 0;
    virtual XMLCh* transcode(const char* const toTranscode,
                             MemoryManager* const manager) = 0;

    virtual bool transcode
    (
        const   char* const     toTranscode
        ,       XMLCh* const    toFill
        , const unsigned int    maxChars
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    ) = 0;

    virtual bool transcode
    (
        const   XMLCh* const    toTranscode
        ,       char* const     toFill
        , const unsigned int    maxBytes
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    ) = 0;


protected :
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    XMLLCPTranscoder();


private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XMLLCPTranscoder(const XMLLCPTranscoder&);
    XMLLCPTranscoder& operator=(const XMLLCPTranscoder&);
};


// ---------------------------------------------------------------------------
//  XMLTranscoder: Getter methods
// ---------------------------------------------------------------------------
inline MemoryManager* XMLTranscoder::getMemoryManager() const
{
    return fMemoryManager;
}

// ---------------------------------------------------------------------------
//  XMLTranscoder: Protected helper methods
// ---------------------------------------------------------------------------
inline unsigned int XMLTranscoder::getBlockSize() const
{
    return fBlockSize;
}

inline const XMLCh* XMLTranscoder::getEncodingName() const
{
    return fEncodingName;
}

XERCES_CPP_NAMESPACE_END

#endif
