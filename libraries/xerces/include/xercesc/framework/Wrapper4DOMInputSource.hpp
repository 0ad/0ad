/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2002 The Apache Software Foundation.  All rights
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
 * $Id: Wrapper4DOMInputSource.hpp,v 1.7 2003/12/01 23:23:25 neilg Exp $
 */


#ifndef WRAPPER4DOMINPUTSOURCE_HPP
#define WRAPPER4DOMINPUTSOURCE_HPP

#include <xercesc/sax/InputSource.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class DOMInputSource;


/**
  * Wrap a DOMInputSource object to a SAX InputSource.
  */
class XMLPARSER_EXPORT Wrapper4DOMInputSource: public InputSource
{
public:
    /** @name Constructors and Destructor */
    //@{

  /**
    * Constructor
    *
    * Wrap a DOMInputSource and pretend it to be a SAX InputSource.
    * By default, the wrapper will adopt the DOMInputSource that is wrapped.
    *
    * @param  inputSource  The DOMInputSource to be wrapped
    * @param  adoptFlag    Indicates if the wrapper should adopt the wrapped
    *                      DOMInputSource. Default is true.
    * @param  manager      Pointer to the memory manager to be used to
    *                      allocate objects.
    */
    Wrapper4DOMInputSource
    (
        DOMInputSource* const inputSource
        , const bool adoptFlag = true
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    );

  /**
    * Destructor
    *
    */
    virtual ~Wrapper4DOMInputSource();
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
    * <p><b>"Experimental - subject to change"</b></p>
    *
    * Get the flag that indicates if the parser should issue fatal error if
    * this input source is not found. Delegated to the wrapped input source
    * object.
    *
    * @return True  if the parser should issue fatal error if this input source
    *               is not found.
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
    * <p><b>"Experimental - subject to change"</b></p>
    *
    * Indicates if the parser should issue fatal error if this input source
    * is not found.  If set to false, the parser issue warning message instead.
    * This will update the wrapped input source object.
    *
    * @param  flag True if the parser should issue fatal error if this input source is not found.
    *               If set to false, the parser issue warning message instead.  (Default: true)
    *
    * @see #getIssueFatalErrorIfNotFound
    */
    void setIssueFatalErrorIfNotFound(const bool flag);

    //@}


private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    Wrapper4DOMInputSource(const Wrapper4DOMInputSource&);
    Wrapper4DOMInputSource& operator=(const Wrapper4DOMInputSource&);

    // -----------------------------------------------------------------------
    //  Private data members
    // -----------------------------------------------------------------------
    bool            fAdoptInputSource;
    DOMInputSource* fInputSource;
};

XERCES_CPP_NAMESPACE_END


#endif
