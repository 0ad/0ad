/*
 * Copyright 2002,2004 The Apache Software Foundation.
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
 * $Id: Wrapper4InputSource.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */


#ifndef WRAPPER4INPUTSOURCE_HPP
#define WRAPPER4INPUTSOURCE_HPP

#include <xercesc/dom/DOMInputSource.hpp>
#include <xercesc/util/PlatformUtils.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class InputSource;


/**
  * Wrap a SAX InputSource object to a DOM InputSource.
  */
class XMLPARSER_EXPORT Wrapper4InputSource: public DOMInputSource
{
public:
    /** @name Constructors and Destructor */
    //@{

  /**
    * Constructor
    *
    * Wrap a SAX InputSource and pretend it to be a DOMInputSource.
    * By default, the wrapper will adopt the SAX InputSource that is wrapped.
    *
    * @param  inputSource  The SAX InputSource to be wrapped
    * @param  adoptFlag    Indicates if the wrapper should adopt the wrapped
    *                      SAX InputSource. Default is true.
    * @param manager The MemoryManager to use to allocate objects
    */
    Wrapper4InputSource(InputSource* const inputSource
                        , const bool adoptFlag = true
                        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

  /**
    * Destructor
    *
    */
    virtual ~Wrapper4InputSource();
    //@}


    // -----------------------------------------------------------------------
    /** @name Virtual input source interface */
    //@{
  /**
    * <p><b>"Experimental - subject to change"</b></p>
    *
    * Makes the byte stream for this input source.
    *
    * <p>The function will call the makeStream of the wrapped input source.
    * The returned stream becomes the parser's property.</p>
    *
    * @see BinInputStream
    */
    BinInputStream* makeStream() const;

    //@}

    // -----------------------------------------------------------------------
    /** @name Getter methods */
    //@{
  /**
    * <p><b>"Experimental - subject to change"</b></p>
    *
    * An input source can be set to force the parser to assume a particular
    * encoding for the data that input source reprsents, via the setEncoding()
    * method. This method will delegate to the wrapped input source to return
    * name of the encoding that is to be forced. If the encoding has never
    * been forced, it returns a null pointer.
    *
    * @return The forced encoding, or null if none was supplied.
    * @see #setEncoding
    */
    const XMLCh* getEncoding() const;


  /**
    * <p><b>"Experimental - subject to change"</b></p>
    *
    * Get the public identifier for this input source. Delegated to the
    * wrapped input source object.
    *
    * @return The public identifier, or null if none was supplied.
    * @see #setPublicId
    */
    const XMLCh* getPublicId() const;


  /**
    * <p><b>"Experimental - subject to change"</b></p>
    *
    * Get the system identifier for this input source. Delegated to the
    * wrapped input source object.
    *
    * <p>If the system ID is a URL, it will be fully resolved.</p>
    *
    * @return The system identifier.
    * @see #setSystemId
    */
    const XMLCh* getSystemId() const;

   /**
     * Get the base URI to be used for resolving relative URIs to absolute
     * URIs. If the baseURI is itself a relative URI, the behavior is
     * implementation dependent. Delegated to the wrapped intput source
     * object.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @return The base URI.
     * @see #setBaseURI
     * @since DOM Level 3
     */
    const XMLCh* getBaseURI() const;

 /**
    * <p><b>"Experimental - subject to change"</b></p>
    *
    * Get the flag that indicates if the parser should issue fatal error if this input source
    * is not found. Delegated to the wrapped input source object.
    *
    * @return True if the parser should issue fatal error if this input source is not found.
    *         False if the parser issue warning message instead.
    * @see #setIssueFatalErrorIfNotFound
    */
    bool getIssueFatalErrorIfNotFound() const;

    //@}


    // -----------------------------------------------------------------------
    /** @name Setter methods */
    //@{

  /**
    * <p><b>"Experimental - subject to change"</b></p>
    *
    * Set the encoding which will be required for use with the XML text read
    * via a stream opened by this input source. This will update the wrapped
    * input source object.
    *
    * <p>This is usually not set, allowing the encoding to be sensed in the
    * usual XML way. However, in some cases, the encoding in the file is known
    * to be incorrect because of intermediate transcoding, for instance
    * encapsulation within a MIME document.
    *
    * @param encodingStr The name of the encoding to force.
    */
    void setEncoding(const XMLCh* const encodingStr);


  /**
    * <p><b>"Experimental - subject to change"</b></p>
    *
    * Set the public identifier for this input source. This will update the
    * wrapped input source object.
    *
    * <p>The public identifier is always optional: if the application writer
    * includes one, it will be provided as part of the location information.</p>
    *
    * @param publicId The public identifier as a string.
    * @see Locator#getPublicId
    * @see SAXParseException#getPublicId
    * @see #getPublicId
    */
    void setPublicId(const XMLCh* const publicId);

  /**
    * <p><b>"Experimental - subject to change"</b></p>
    *
    * Set the system identifier for this input source. This will update the
    * wrapped input source object.
    *
    * <p>The system id is always required. The public id may be used to map
    * to another system id, but the system id must always be present as a fall
    * back.</p>
    *
    * <p>If the system ID is a URL, it must be fully resolved.</p>
    *
    * @param systemId The system identifier as a string.
    * @see #getSystemId
    * @see Locator#getSystemId
    * @see SAXParseException#getSystemId
    */
    void setSystemId(const XMLCh* const systemId);

   /**
    * Set the base URI to be used for resolving relative URIs to absolute
    * URIs. If the baseURI is itself a relative URI, the behavior is
    * implementation dependent. This will update the wrapped input source
    * object.
    *
    * <p><b>"Experimental - subject to change"</b></p>
    *
    * @param baseURI The base URI.
    * @see #getBaseURI
    * @since DOM Level 3
    */
    void setBaseURI(const XMLCh* const baseURI);

  /**
    * <p><b>"Experimental - subject to change"</b></p>
    *
    * Indicates if the parser should issue fatal error if this input source
    * is not found.  If set to false, the parser issue warning message
    * instead. This will update the wrapped input source object.
    *
    * @param flag  True if the parser should issue fatal error if this input
    *              source is not found.
    *              If set to false, the parser issue warning message instead.
    *              (Default: true)
    *
    * @see #getIssueFatalErrorIfNotFound
    */
    void setIssueFatalErrorIfNotFound(const bool flag);

   /**
    * Called to indicate that this DOMInputSource is no longer in use
    * and that the implementation may relinquish any resources associated with it.
    *
    * Access to a released object will lead to unexpected result.
    */
    void              release();

    //@}


private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    Wrapper4InputSource(const Wrapper4InputSource&);
    Wrapper4InputSource& operator=(const Wrapper4InputSource&);

    // -----------------------------------------------------------------------
    //  Private data members
    // -----------------------------------------------------------------------
    bool         fAdoptInputSource;
    InputSource* fInputSource;
};


// ---------------------------------------------------------------------------
//  Wrapper4InputSource: Getter methods
// ---------------------------------------------------------------------------
inline const XMLCh* Wrapper4InputSource::getBaseURI() const
{
    return 0; // REVISIT - should we return an empty string?
}

// ---------------------------------------------------------------------------
//  Wrapper4InputSource: Setter methods
// ---------------------------------------------------------------------------
inline void Wrapper4InputSource::setBaseURI(const XMLCh* const)
{
}

XERCES_CPP_NAMESPACE_END

#endif
