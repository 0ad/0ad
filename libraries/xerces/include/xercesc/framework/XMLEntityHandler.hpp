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
  * $Log: XMLEntityHandler.hpp,v $
  * Revision 1.7  2003/10/30 21:37:31  knoaman
  * Enhanced Entity Resolver Support. Thanks to David Cargill.
  *
  * Revision 1.6  2003/03/07 18:08:10  tng
  * Return a reference instead of void for operator=
  *
  * Revision 1.5  2002/11/04 15:00:21  tng
  * C++ Namespace Support.
  *
  * Revision 1.4  2002/06/06 20:41:51  tng
  * Regression fix: should assign the returned InputSource from resolveEntity to srcToFill.
  *
  * Revision 1.3  2002/05/31 15:14:07  tng
  * Fix doxygen documentation.
  *
  * Revision 1.2  2002/05/29 21:47:08  knoaman
  * DOM L3 LS: DOMInputSource, DOMEntityResolver, DOMImplementationLS and DOMBuilder
  *
  * Revision 1.1.1.1  2002/02/01 22:21:51  peiyongz
  * sane_include
  *
  * Revision 1.7  2000/03/02 19:54:25  roddey
  * This checkin includes many changes done while waiting for the
  * 1.1.0 code to be finished. I can't list them all here, but a list is
  * available elsewhere.
  *
  * Revision 1.6  2000/02/24 20:00:23  abagchi
  * Swat for removing Log from API docs
  *
  * Revision 1.5  2000/02/16 21:42:58  aruna1
  * API Doc++ summary changes in
  *
  * Revision 1.4  2000/02/15 01:21:31  roddey
  * Some initial documentation improvements. More to come...
  *
  * Revision 1.3  2000/02/06 07:47:48  rahulj
  * Year 2K copyright swat.
  *
  * Revision 1.2  1999/12/15 19:46:28  roddey
  * Got rid of redundant 'const' on bool return value. Some compilers choke on this
  * and its not useful anyway.
  *
  * Revision 1.1.1.1  1999/11/09 01:08:33  twl
  * Initial checkin
  *
  * Revision 1.2  1999/11/08 20:44:38  rahul
  * Swat for adding in Product name and CVS comment log variable.
  *
  */


#if !defined(XMLENTITYHANDLER_HPP)
#define XMLENTITYHANDLER_HPP

#include <xercesc/util/XercesDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class InputSource;
class XMLBuffer;
class XMLResourceIdentifier;

/**
 *  This abstract class is a callback mechanism for the scanner. By creating
 *  a derivative of this class and plugging into the scanner, the scanner
 *  will call back on the object's methods to entity events.
 *
 *  This class is primarily for use by those writing their own parser classes.
 *  If you use the standard parser classes, DOMParser and SAXParser, you won't
 *  use this API. You will instead use a similar mechanism defined by the SAX
 *  API, called EntityResolver.
 */
class XMLPARSER_EXPORT XMLEntityHandler
{
public:
    // -----------------------------------------------------------------------
    //  Constructors are hidden, only the virtual destructor is exposed
    // -----------------------------------------------------------------------

    /** @name Destructor */
    //@{

    /**
      * Default destructor
      */
    virtual ~XMLEntityHandler()
    {
    }
    //@}


    // -----------------------------------------------------------------------
    //  The virtual entity handler interface
    // -----------------------------------------------------------------------
    /** @name The pure virtual methods in this interface. */
    //@{

    /**
      * This method get called after the scanner has finished reading from
      * the given input source while processing external entity references.
      *
      * @param inputSource The input source for the entity
      */
    virtual void endInputSource(const InputSource& inputSource) = 0;

    /**
      * This method allows the passes the scanned systemId to the entity
      * handler, thereby giving it a chance to provide any customized
      * handling like resolving relative path names. The scanner first
      * calls this method before calling <code>resolveEntity</code>.
      *
      * @param systemId The system id extracted by the scanner from the
      *                 input source.
      * @param toFill The buffer in which the fully expanded system id needs
      *               to be stored.
      */
    virtual bool expandSystemId
    (
        const   XMLCh* const    systemId
        ,       XMLBuffer&      toFill
    ) = 0;

    /**
      * This method allows the entity handler to reset itself, so that
      * it can be used again. It is called prior to a new document parse
      * operation.
      */
    virtual void resetEntities() = 0;

    /**
      * This method allows the entity handler to provide customized
      * application specific entity resolution. This method is defined
      * by SAX 1.0 API.
      *
      * <i>Only one resolveEntity method will be used.  If both setEntityResolver and 
      * setXMLEntityResolver are called, then the last one is used.</i>
      *
      * @param publicId A const pointer to a Unicode string representing the
      *                 public id of the entity just parsed.
      * @param systemId A const pointer to a Unicode string representing the
      *                 system id of the entity just parsed.
      * @param baseURI  A const pointer to a Unicode string representing the
      *                 base URI of the entity just parsed,
      *                 or <code>null</code> if there is no base URI.
      * @return The value returned by the SAX resolveEntity method or
      *         NULL otherwise to indicate no processing was done.
      *         The returned InputSource is owned by the parser which is
      *         responsible to clean up the memory.
      */
    virtual InputSource* resolveEntity
    (
        const   XMLCh* const    publicId
        , const XMLCh* const    systemId
        , const XMLCh* const    baseURI = 0
    ) = 0;

    /**
      * This method allows the entity handler to provide customized
      * application specific entity resolution. 
      *
      * <i>Only one resolveEntity method will be used.  If both setEntityResolver and 
      * setXMLEntityResolver are called, then the last one is used.</i>
      *
      * @param resourceIdentifier An object containing the type of
      *        resource to be resolved and the associated data members
      *        corresponding to this type.
      * @return The value returned by the resolveEntity method or
      *         NULL otherwise to indicate no processing was done.
      *         The returned InputSource is owned by the parser which is
      *         responsible to clean up the memory.
      */
    virtual InputSource* resolveEntity
    (
        XMLResourceIdentifier* resourceIdentifier
    ) = 0;

    /**
      * This method will be called before the scanner starts reading
      * from an input source while processing external entity references.
      *
      * @param inputSource The external input source.
      */
    virtual void startInputSource(const InputSource& inputSource) = 0;
    //@}


protected :
    // -----------------------------------------------------------------------
    //  Hidden Constructors
    // -----------------------------------------------------------------------
    /** @name Constructor */
    //@{

    /**
      * Protected default constructor
      */
    XMLEntityHandler()
    {
    }
    //@}



private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and destructor
    // -----------------------------------------------------------------------
    XMLEntityHandler(const XMLEntityHandler&);
    XMLEntityHandler& operator=(const XMLEntityHandler&);
};

XERCES_CPP_NAMESPACE_END

#endif
