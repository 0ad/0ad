/*
 * Copyright 1999-2001,2004 The Apache Software Foundation.
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
 * $Id: TransService.hpp 191054 2005-06-17 02:56:35Z jberry $
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
      * @param srcCount number of bytes in the source buffer
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
