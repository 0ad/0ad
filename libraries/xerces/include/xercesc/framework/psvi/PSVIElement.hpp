/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2003 The Apache Software Foundation.  All rights
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
 * $Log: PSVIElement.hpp,v $
 * Revision 1.6  2003/11/28 21:18:31  knoaman
 * Make use of canonical representation in PSVIElement
 *
 * Revision 1.5  2003/11/27 22:52:37  knoaman
 * PSVIElement implementation
 *
 * Revision 1.4  2003/11/21 22:34:45  neilg
 * More schema component model implementation, thanks to David Cargill.
 * In particular, this cleans up and completes the XSModel, XSNamespaceItem,
 * XSAttributeDeclaration and XSAttributeGroup implementations.
 *
 * Revision 1.3  2003/11/06 21:50:33  neilg
 * fix compilation errors under gcc 3.3.
 *
 * Revision 1.2  2003/11/06 15:30:04  neilg
 * first part of PSVI/schema component model implementation, thanks to David Cargill.  This covers setting the PSVIHandler on parser objects, as well as implementing XSNotation, XSSimpleTypeDefinition, XSIDCDefinition, and most of XSWildcard, XSComplexTypeDefinition, XSElementDeclaration, XSAttributeDeclaration and XSAttributeUse.
 *
 * Revision 1.1  2003/09/16 14:33:36  neilg
 * PSVI/schema component model classes, with Makefile/configuration changes necessary to build them
 *
 */

#if !defined(PSVIELEMENT_HPP)
#define PSVIELEMENT_HPP

#include <xercesc/framework/psvi/PSVIItem.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
 * Represent the PSVI contributions for one element information item.
 * This is *always* owned by the scanner/parser object from which
 * it is obtained.  The validator will specify 
 * under what conditions it may be relied upon to have meaningful contents.
 */

// forward declarations
class XSElementDeclaration;
class XSNotationDeclaration;
class XSModel;

class XMLPARSER_EXPORT PSVIElement : public PSVIItem  
{
public:

    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    /** @name Constructors */
    //@{

    /**
      * The default constructor 
      *
      * @param  manager     The configurable memory manager
      */
    PSVIElement( MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    //@};

    /** @name Destructor */
    //@{
    ~PSVIElement();
    //@}

    //---------------------
    /** @name PSVIElement methods */

    //@{

    /**
     * An item isomorphic to the element declaration used to validate
     * this element.
     * 
     * @return  an element declaration
     */
    XSElementDeclaration *getElementDeclaration();
    
    /**
     * [notation] 
     * @see <a href="http://www.w3.org/TR/xmlschema-1/#e-notation">XML Schema Part 1: Structures [notation]</a>
     * @return The notation declaration. 
     */
    XSNotationDeclaration *getNotationDeclaration();

    /**
     * [schema information]
     * @see <a href="http://www.w3.org/TR/xmlschema-1/#e-schema_information">XML Schema Part 1: Structures [schema information]</a>
     * @return The schema information property if it's the validation root,
     *         null otherwise.
     */
    XSModel *getSchemaInformation();
    
    /**
     * An item isomorphic to the type definition used to validate this element.
     * 
     * @return  a type declaration
     */
    XSTypeDefinition *getTypeDefinition();
    
    /**
     * If and only if that type definition is a simple type definition
     * with {variety} union, or a complex type definition whose {content type}
     * is a simple type definition with {variety} union, then an item isomorphic
     * to that member of the union's {member type definitions} which actually
     * validated the element item's normalized value.
     * 
     * @return  a simple type declaration
     */
    XSSimpleTypeDefinition *getMemberTypeDefinition();
    
    //@}

    //----------------------------------
    /** methods needed by implementation */

    //@{
    void reset
    (
        const VALIDITY_STATE            validityState
        , const ASSESSMENT_TYPE         assessmentType
        , const XMLCh* const            validationContext
        , bool                          isSpecified
        , XSElementDeclaration* const   elemDecl
        , XSTypeDefinition* const       typeDef
        , XSSimpleTypeDefinition* const memberType
        , XSModel* const                schemaInfo
        , const XMLCh* const            defaultValue
        , const XMLCh* const            normalizedValue = 0
        , XMLCh* const                  canonicalValue = 0
        , XSNotationDeclaration* const  notationDecl = 0
    );

    //@}

private:

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    PSVIElement(const PSVIElement&);
    PSVIElement & operator=(const PSVIElement &);


    // -----------------------------------------------------------------------
    //  data members
    // -----------------------------------------------------------------------
    // fElementDecl
    //  element declaration component that validated this element
    // fNotationDecl
    //  (optional) notation decl associated with this element
    // fSchemaInfo
    //  Schema Information Item with which this validation episode is associated
    XSElementDeclaration *fElementDecl;
    XSNotationDeclaration *fNotationDecl;
    XSModel *fSchemaInfo;
};

inline XSElementDeclaration *PSVIElement::getElementDeclaration() 
{
    return fElementDecl;
}

inline XSNotationDeclaration* PSVIElement::getNotationDeclaration() 
{
    return fNotationDecl;
}

inline XSModel* PSVIElement::getSchemaInformation() 
{
    return fSchemaInfo;
}

XERCES_CPP_NAMESPACE_END

#endif
