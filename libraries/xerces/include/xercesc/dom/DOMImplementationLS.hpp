#ifndef DOMImplementationLS_HEADER_GUARD_
#define DOMImplementationLS_HEADER_GUARD_

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
 * $Log: DOMImplementationLS.hpp,v $
 * Revision 1.10  2003/12/01 23:23:25  neilg
 * fix for bug 25118; thanks to Jeroen Witmond
 *
 * Revision 1.9  2003/06/20 18:47:21  peiyongz
 * Stateless Grammar Pool :: Part I
 *
 * Revision 1.8  2003/05/15 18:25:53  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.7  2003/03/07 19:59:05  tng
 * [Bug 11692] Unimplement the hidden constructors and assignment operator to remove warnings from gcc.
 *
 * Revision 1.6  2002/11/04 15:09:24  tng
 * C++ Namespace Support.
 *
 * Revision 1.5  2002/08/22 15:04:57  tng
 * Remove unused parameter variables in inline functions.
 *
 * Revision 1.4  2002/06/06 20:53:06  tng
 * Documentation Fix: Update the API Documentation for DOM headers
 *
 * Revision 1.3  2002/05/30 19:24:48  knoaman
 * documentation update
 *
 * Revision 1.2  2002/05/30 16:39:06  knoaman
 * DOM L3 LS.
 *
 * Revision 1.1  2002/05/29 21:28:02  knoaman
 * DOM L3 LS: DOMInputSource, DOMEntityResolver, DOMImplementationLS and DOMBuilder
 *
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
     * @return The newly created <code>DOMBuilder<code> object. This
     * <code>DOMBuilder</code> is either synchronous or asynchronous depending
     * on the value of the <code>mode<code> argument.
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
     * @return The newly created <code>DOMWriter<code> object.
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
     * @return The newly created <code>DOMInputSource<code> object.
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
