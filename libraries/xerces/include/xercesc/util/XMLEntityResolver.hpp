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
 * $Log: XMLEntityResolver.hpp,v $
 * Revision 1.1  2003/10/30 21:37:32  knoaman
 * Enhanced Entity Resolver Support. Thanks to David Cargill.
 *
 *
 * Revision 1.1    1999/11/09 01:07:44  twl
 * Initial checkin
 *
 */

#ifndef XMLENTITYRESOLVER_HPP
#define XMLENTITYRESOLVER_HPP

#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/util/XMemory.hpp>
#include <xercesc/util/XMLResourceIdentifier.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class InputSource;

/**
  * Revised interface for resolving entities.
  *
  * <p>If an application needs to implement customized handling
  * for external entities, it can implement this interface and
  * register an instance with the parser using the parser's
  * setXMLEntityResolver method or it can use the basic SAX interface 
  * (EntityResolver).  The difference between the two interfaces is
  * the arguments to the resolveEntity() method.  With the SAX
  * EntityResolve the arguments are systemId and publicId.  With this
  * interface the argument is a XMLResourceIdentifier object.  <i>Only
  * one EntityResolver can be set using setEntityResolver() or 
  * setXMLEntityResolver, if both are set the last one set is 
  * used.</i></p>
  *
  * <p>The parser will then allow the application to intercept any
  * external entities (including the external DTD subset and external
  * parameter entities, if any) before including them.</p>
  *
  * <p>Many applications will not need to implement this interface,
  * but it will be especially useful for applications that build
  * XML documents from databases or other specialised input sources,
  * or for applications that use URI types other than URLs.</p>
  *
  * <p>The following resolver would provide the application
  * with a special character stream for the entity with the system
  * identifier "http://www.myhost.com/today":</p>
  *
  *<pre>
  * #include <xercesc/util/XMLEntityResolver.hpp>
  * #include <xercesc/sax/InputSource.hpp>
  *
  * class MyResolver : public XMLEntityResolver {
  *  public:
  *    InputSource resolveEntity (XMLResourceIdentifier* xmlri);
  *   ...
  *   };
  *
  *  MyResolver::resolveEntity(XMLResourceIdentifier* xmlri) {
  *    switch(xmlri->getResourceIdentifierType()) {
  *      case XMLResourceIdentifier::SystemId: 
  *        if (XMLString::compareString(xmlri->getSystemId(), "http://www.myhost.com/today")) {
  *          MyReader* reader = new MyReader();
  *          return new InputSource(reader);
  *        } else {
  *          return null;
  *        }
  *        break;
  *      default:
  *        return null;
  *    }
  *  }</pre>
  *
  * <p>The application can also use this interface to redirect system
  * identifiers to local URIs or to look up replacements in a catalog
  * (possibly by using the public identifier).</p>
  *
  * <p>The HandlerBase class implements the default behaviour for
  * this interface, which is simply always to return null (to request
  * that the parser use the default system identifier).</p>
  *
  * @see XMLResourceIdentifier
  * @see Parser#setXMLEntityResolver
  * @see InputSource#InputSource
  * @see HandlerBase#HandlerBase
  */
class XMLUTIL_EXPORT XMLEntityResolver
{
public:
    /** @name Constructors and Destructor */
    //@{


    /** Destructor */
    virtual ~XMLEntityResolver()
    {
    }

    //@}

    /** @name The XMLEntityResolver interface */
    //@{

  /**
    * Allow the application to resolve external entities.
    *
    * <p>The Parser will call this method before opening any external
    * entity except the top-level document entity (including the
    * external DTD subset, external entities referenced within the
    * DTD, and external entities referenced within the document
    * element): the application may request that the parser resolve
    * the entity itself, that it use an alternative URI, or that it
    * use an entirely different input source.</p>
    *
    * <p>Application writers can use this method to redirect external
    * system identifiers to secure and/or local URIs, to look up
    * public identifiers in a catalogue, or to read an entity from a
    * database or other input source (including, for example, a dialog
    * box).</p>
    *
    * <p>If the system identifier is a URL, the SAX parser must
    * resolve it fully before reporting it to the application.</p>
    *
    * @param resourceIdentifier An object containing the type of
    *        resource to be resolved and the associated data members
    *        corresponding to this type.
    * @return An InputSource object describing the new input source,
    *         or null to request that the parser open a regular
    *         URI connection to the system identifier.
    *         The returned InputSource is owned by the parser which is
    *         responsible to clean up the memory.
    * @exception SAXException Any SAX exception, possibly
    *            wrapping another exception.
    * @exception IOException An IO exception,
    *            possibly the result of creating a new InputStream
    *            or Reader for the InputSource.
    *
    * @see InputSource#InputSource
    * @see XMLResourceIdentifier
    */
    virtual InputSource* resolveEntity
    (
        XMLResourceIdentifier* resourceIdentifier
    ) = 0;

    //@}
protected: 
    /** Default Constructor */
    XMLEntityResolver()
    {
    }

private :
    /* Unimplemented constructors and operators */

    /* Copy constructor */
    XMLEntityResolver(const XMLEntityResolver&);

    /* Assignment operator */
    XMLEntityResolver& operator=(const XMLEntityResolver&);

};

XERCES_CPP_NAMESPACE_END

#endif
