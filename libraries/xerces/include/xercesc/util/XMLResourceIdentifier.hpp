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
 * $Log: XMLResourceIdentifier.hpp,v $
 * Revision 1.5  2004/02/15 19:37:16  amassari
 * Removed cause for warnings in VC 7.1
 *
 * Revision 1.4  2004/02/13 14:28:30  cargilld
 * Fix for bug 26900 (remove virtual on destructor)
 *
 * Revision 1.3  2004/01/29 11:48:47  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.2  2003/11/25 18:16:38  knoaman
 * Documentation update. Thanks to David Cargill.
 *
 * Revision 1.1  2003/10/30 21:37:32  knoaman
 * Enhanced Entity Resolver Support. Thanks to David Cargill.
 *
 *
 * Revision 1.1    1999/11/09 01:07:44  twl
 * Initial checkin
 *
 */

#ifndef XMLRESOURCEIDENTIFIER_HPP
#define XMLRESOURCEIDENTIFIER_HPP

XERCES_CPP_NAMESPACE_BEGIN

/**
  * <p>This class is used along with XMLEntityResolver to resolve entities.
  * Instead of passing publicId and systemId on the resolveEntity call, 
  * as is done with the SAX entity resolver, an object of type XMLResourceIdentifier
  * is passed.  By calling the getResourceIdentifierType() method the user can
  * determine which data members are available for inspection:</p>
  *
  * <table border='1'>
  * <tr>
  *  <td>ResourceIdentifierType</td>
  *  <td>Available Data Members</td>
  * </tr>
  * <tr>
  *  <td>SchemaGrammar</td>
  *  <td>schemaLocation, nameSpace & baseURI (current document)</td>
  * </tr>
  * <tr>
  *  <td>SchemaImport</td>
  *  <td>schemaLocation, nameSpace & baseURI (current document)</td>
  * </tr>
  * <tr>
  *  <td>SchemaInclude</td>
  *  <td>schemaLocation & baseURI (current document)</td>
  * </tr>
  * <tr>
  *  <td>SchemaRedefine</td>
  *  <td>schemaLocation & baseURI (current document)</td>
  * </tr>
  * <tr>
  *  <td>ExternalEntity</td>
  *  <td>systemId, publicId & baseURI (some items may be NULL)</td>
  * </tr>
  * </table>
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
  * @see SAXParser#setXMLEntityResolver
  * @see InputSource#InputSource
  */
class XMLUTIL_EXPORT XMLResourceIdentifier
{
public:

    enum ResourceIdentifierType {
        SchemaGrammar = 0,
        SchemaImport,
        SchemaInclude,
        SchemaRedefine ,
        ExternalEntity,
        UnKnown = 255
    //@{
    };

    /** @name Constructors and Destructor */

    /** Constructor */

    XMLResourceIdentifier(const ResourceIdentifierType resourceIdentitiferType
                            , const XMLCh* const  systemId
                            , const XMLCh* const  nameSpace = 0
                            , const XMLCh* const  publicId = 0
                            , const XMLCh* const  baseURI = 0);

    /** Destructor */
    ~XMLResourceIdentifier()
    {
    }

    //@}

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    ResourceIdentifierType getResourceIdentifierType() const;
    const XMLCh* getPublicId()          const;
    const XMLCh* getSystemId()          const;
    const XMLCh* getSchemaLocation()    const;
    const XMLCh* getBaseURI()           const;
    const XMLCh* getNameSpace()         const;

private :

    const ResourceIdentifierType    fResourceIdentifierType;
    const XMLCh*                    fPublicId;
    const XMLCh*                    fSystemId;
    const XMLCh*                    fBaseURI;
    const XMLCh*                    fNameSpace;


    /* Unimplemented constructors and operators */

    /* Copy constructor */
    XMLResourceIdentifier(const XMLResourceIdentifier&);

    /* Assignment operator */
    XMLResourceIdentifier& operator=(const XMLResourceIdentifier&);

};

inline XMLResourceIdentifier::ResourceIdentifierType XMLResourceIdentifier::getResourceIdentifierType() const 
{
    return fResourceIdentifierType;
}

inline const XMLCh* XMLResourceIdentifier::getPublicId() const
{
    return fPublicId;
}

inline const XMLCh* XMLResourceIdentifier::getSystemId() const
{
    return fSystemId;
}

inline const XMLCh* XMLResourceIdentifier::getSchemaLocation() const
{
    return fSystemId;
}

inline const XMLCh* XMLResourceIdentifier::getBaseURI() const
{
    return fBaseURI;
}

inline const XMLCh* XMLResourceIdentifier::getNameSpace() const
{
    return fNameSpace;
}

inline XMLResourceIdentifier::XMLResourceIdentifier(const ResourceIdentifierType resourceIdentifierType
                            , const XMLCh* const  systemId
                            , const XMLCh* const  nameSpace
                            , const XMLCh* const  publicId
                            , const XMLCh* const  baseURI )
    : fResourceIdentifierType(resourceIdentifierType)
    , fPublicId(publicId)
    , fSystemId(systemId)
    , fBaseURI(baseURI)
    , fNameSpace(nameSpace)    
{
}

XERCES_CPP_NAMESPACE_END

#endif
