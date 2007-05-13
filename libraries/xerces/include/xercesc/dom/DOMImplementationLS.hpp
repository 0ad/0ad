#ifndef DOMImplementationLS_HEADER_GUARD_
#define DOMImplementationLS_HEADER_GUARD_

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
 * $Id: DOMImplementationLS.hpp 191054 2005-06-17 02:56:35Z jberry $
 */


#include <xercesc/util/PlatformUtils.hpp>

XERCES_CPP_NAMESPACE_BEGIN


class DOMBuilder;
class DOMWriter;
class DOMInputSource;
class MemoryManager;
class XMLGrammarPool;

/**
  * <p><code>DOMImplementationLS</code> contains the factory methods for
  * creating objects that implement the <code>DOMBuilder</code> (parser) and
  * <code>DOMWriter</code> (serializer) interfaces.</p>
  *
  * <p>An object that implements DOMImplementationLS is obtained by doing a
  * binding specific cast from DOMImplementation to DOMImplementationLS.
  * Implementations supporting the Load and Save feature must implement the
  * DOMImplementationLS interface on whatever object implements the
  * DOMImplementation interface.</p>
  *
  * @since DOM Level 3
  */
class CDOM_EXPORT DOMImplementationLS
{
protected:
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    /** @name Hidden constructors */
    //@{    
    DOMImplementationLS() {};
    //@}

private:
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    /** @name Unimplemented constructors and operators */
    //@{
    DOMImplementationLS(const DOMImplementationLS &);
    DOMImplementationLS & operator = (const DOMImplementationLS &);
    //@}

public:
    // -----------------------------------------------------------------------
    //  All constructors are hidden, just the destructor is available
    // -----------------------------------------------------------------------
    /** @name Destructor */
    //@{
    /**
     * Destructor
     *
     */
    virtual ~DOMImplementationLS() {};
    //@}

    // -----------------------------------------------------------------------
    //  Public constants
    // -----------------------------------------------------------------------
    /** @name Public constants */
    //@{
    /**
     * Create a synchronous or an asynchronous <code>DOMBuilder</code>.
     * @see createDOMBuilder(const short mode, const XMLCh* const schemaType)
     * @since DOM Level 3
     *
     */
    enum
    {
        MODE_SYNCHRONOUS = 1,
        MODE_ASYNCHRONOUS = 2
    };
    //@}

    // -----------------------------------------------------------------------
    // Virtual DOMImplementation LS interface
    // -----------------------------------------------------------------------
    /** @name Functions introduced in DOM Level 3 */
    //@{
    // -----------------------------------------------------------------------
    //  Factory create methods
    // -----------------------------------------------------------------------
    /**
     * Create a new DOMBuilder. The newly constructed parser may then be
     * configured by means of its setFeature method, and used to parse
     * documents by means of its parse method.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @param mode The mode argument is either <code>MODE_SYNCHRONOUS</code> or
     * <code>MODE_ASYNCHRONOUS</code>, if mode is <code>MODE_SYNCHRONOUS</code>
     * then the <code>DOMBuilder</code> that is created will operate in
     * synchronous mode, if it's <code>MODE_ASYNCHRONOUS</code> then the
     * <code>DOMBuilder</code> that is created will operate in asynchronous
     * mode.
     * @param schemaType An absolute URI representing the type of the schema
     * language used during the load of a DOMDocument using the newly created
     * <code>DOMBuilder</code>. Note that no lexical checking is done on the
     * absolute URI. In order to create a DOMBuilder for any kind of schema
     * types (i.e. the DOMBuilder will be free to use any schema found), use
     * the value <code>null</code>.
     * @param manager    Pointer to the memory manager to be used to
     *                   allocate objects.
     * @param gramPool   The collection of cached grammers.
     * @return The newly created <code>DOMBuilder</code> object. This
     * <code>DOMBuilder</code> is either synchronous or asynchronous depending
     * on the value of the <code>mode</code> argument.
     * @exception DOMException NOT_SUPPORTED_ERR: Raised if the requested mode
     * or schema type is not supported.
     *
     * @see DOMBuilder
     * @since DOM Level 3
     */
    virtual DOMBuilder* createDOMBuilder(const short            mode,
                                         const XMLCh* const     schemaType,
                                         MemoryManager* const   manager = XMLPlatformUtils::fgMemoryManager,
                                         XMLGrammarPool*  const gramPool = 0) = 0;


    /**
     * Create a new DOMWriter. DOMWriters are used to serialize a DOM tree
     * back into an XML document.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @return The newly created <code>DOMWriter</code> object.
     *
     * @see DOMWriter
     * @since DOM Level 3
     */
    virtual DOMWriter* createDOMWriter(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager) = 0;

    /**
     * Create a new "empty" DOMInputSource.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @return The newly created <code>DOMInputSource</code> object.
     * @exception DOMException NOT_SUPPORTED_ERR: Raised if this function is not
     * supported by implementation
     *
     * @see DOMInputSource
     * @since DOM Level 3
     */
    virtual DOMInputSource* createDOMInputSource() = 0;

    //@}
};


XERCES_CPP_NAMESPACE_END

#endif
