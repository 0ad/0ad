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
 * $Log: PSVIAttribute.hpp,v $
 * Revision 1.6  2003/12/30 05:58:56  neilg
 * allow schema normalized values to be associated with a PSVIAttribute after it is reset
 *
 * Revision 1.5  2003/11/28 20:20:54  neilg
 * make use of canonical representation in PSVIAttribute implementation
 *
 * Revision 1.4  2003/11/27 06:10:32  neilg
 * PSVIAttribute implementation
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

#if !defined(PSVIATTRIBUTE_HPP)
#define PSVIATTRIBUTE_HPP

#include <xercesc/framework/psvi/PSVIItem.hpp>
#include <xercesc/framework/psvi/XSSimpleTypeDefinition.hpp>
#include <xercesc/validators/datatype/DatatypeValidator.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
 * Represent the PSVI contributions for one attribute information item.
 * This is *always* owned by the scanner/parser object from which
 * it is obtained.  The validator will specify 
 * under what conditions it may be relied upon to have meaningful contents.
 */

// forward declarations
class XSAttributeDeclaration;

class XMLPARSER_EXPORT PSVIAttribute : public PSVIItem  
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
    PSVIAttribute( MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    //@};

    /** @name Destructor */
    //@{
    ~PSVIAttribute();
    //@}

    //---------------------
    /** @name PSVIAttribute methods */

    //@{

    /**
     * An item isomorphic to the attribute declaration used to validate
     * this attribute.
     * 
     * @return  an attribute declaration
     */
    XSAttributeDeclaration *getAttributeDeclaration();
    
    /**
     * An item isomorphic to the type definition used to validate this element.
     * 
     * @return  a type declaration
     */
    XSTypeDefinition *getTypeDefinition();
    
    /**
     * If and only if that type definition is a simple type definition
     * with {variety} union, or a complex type definition whose {content type}
     * is a simple thype definition with {variety} union, then an item isomorphic
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

    /**
      * reset this object.  Intended to be called by
      * the implementation.
      */
    void reset(
            const XMLCh * const         valContext
            , PSVIItem::VALIDITY_STATE  state
            , PSVIItem::ASSESSMENT_TYPE assessmentType
            , XSSimpleTypeDefinition *  validatingType
            , XSSimpleTypeDefinition *  memberType
            , const XMLCh * const       defaultValue
            , const bool                isSpecified
            , XSAttributeDeclaration *  attrDecl
            , DatatypeValidator *       dv
        );

    /**
      * set the schema normalized value (and
      * implicitly the canonical value) of this object; intended to be used
      * by the implementation.
      */
    void setValue(const XMLCh * const       normalizedValue);

    /**
      * set VALIDITY_STATE to specified value; intended to be
      * called by implementation.
      */
    void updateValidity(VALIDITY_STATE newValue);

    //@}

private:

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    PSVIAttribute(const PSVIAttribute&);
    PSVIAttribute & operator=(const PSVIAttribute &);


    // -----------------------------------------------------------------------
    //  data members
    // -----------------------------------------------------------------------
    // fAttributeDecl
    //      attribute declaration component that validated this attribute 
    // fDV
    //      implementation-specific datatype validator used to validate this attribute
    XSAttributeDeclaration *    fAttributeDecl;
    DatatypeValidator *         fDV;
};
inline PSVIAttribute::~PSVIAttribute() 
{
    fMemoryManager->deallocate((void *)fCanonicalValue);
}

inline XSAttributeDeclaration *PSVIAttribute::getAttributeDeclaration() 
{
    return fAttributeDecl;
}

inline XSTypeDefinition* PSVIAttribute::getTypeDefinition()
{
    return fType;
}

inline XSSimpleTypeDefinition* PSVIAttribute::getMemberTypeDefinition() 
{
    return fMemberType;
}

inline void PSVIAttribute::updateValidity(VALIDITY_STATE newValue)
{
    fValidityState = newValue;
}

XERCES_CPP_NAMESPACE_END

#endif
