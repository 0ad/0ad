/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2001-2002 The Apache Software Foundation.  All rights
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
 * originally based on software copyright (c) 2001, International
 * Business Machines, Inc., http://www.ibm.com .  For more information
 * on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * $Id: GeneralAttributeCheck.hpp,v 1.12 2003/12/17 00:18:40 cargilld Exp $
 */

#if !defined(GENERALATTRIBUTECHECK_HPP)
#define GENERALATTRIBUTECHECK_HPP

/**
  * A general purpose class to check for valid values of attributes, as well
  * as check for proper association with corresponding schema elements.
  */

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/RefHashTableOf.hpp>
#include <xercesc/util/ValueHashTableOf.hpp>
#include <xercesc/validators/datatype/IDDatatypeValidator.hpp>
#include <xercesc/framework/ValidationContext.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Forward declaration
// ---------------------------------------------------------------------------
class TraverseSchema;
class DOMElement;
class DOMNode;

class VALIDATORS_EXPORT GeneralAttributeCheck : public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constants
    // -----------------------------------------------------------------------
    //Elements
    enum
    {
        E_All,
        E_Annotation,
        E_Any,
        E_AnyAttribute,
        E_Appinfo,
        E_AttributeGlobal,
        E_AttributeLocal,
        E_AttributeRef,
        E_AttributeGroupGlobal,
        E_AttributeGroupRef,
        E_Choice,
        E_ComplexContent,
        E_ComplexTypeGlobal,
        E_ComplexTypeLocal,
        E_Documentation,
        E_ElementGlobal,
        E_ElementLocal,
        E_ElementRef,
        E_Enumeration,
        E_Extension,
        E_Field,
        E_FractionDigits,
        E_GroupGlobal,
        E_GroupRef,
        E_Import,
        E_Include,
        E_Key,
        E_KeyRef,
        E_Length,
        E_List,
        E_MaxExclusive,
        E_MaxInclusive,
        E_MaxLength,
        E_MinExclusive,
        E_MinInclusive,
        E_MinLength,
        E_Notation,
        E_Pattern,
        E_Redefine,
        E_Restriction,
        E_Schema,
        E_Selector,
        E_Sequence,
        E_SimpleContent,
        E_SimpleTypeGlobal,
        E_SimpleTypeLocal,
        E_TotalDigits,
        E_Union,
        E_Unique,
        E_WhiteSpace,

        E_Count,
        E_Invalid = -1
    };

    //Attributes
    enum
    {
        A_Abstract,
        A_AttributeFormDefault,
        A_Base,
        A_Block,
        A_BlockDefault,
        A_Default,
        A_ElementFormDefault,
        A_Final,
        A_FinalDefault,
        A_Fixed,
        A_Form,
        A_ID,
        A_ItemType,
        A_MaxOccurs,
        A_MemberTypes,
        A_MinOccurs,
        A_Mixed,
        A_Name,
        A_Namespace,
        A_Nillable,
        A_ProcessContents,
        A_Public,
        A_Ref,
        A_Refer,
        A_SchemaLocation,
        A_Source,
        A_SubstitutionGroup,
        A_System,
        A_TargetNamespace,
        A_Type,
        A_Use,
        A_Value,
        A_Version,
        A_XPath,

        A_Count,
        A_Invalid = -1
    };

    //Validators
    enum {

        DV_String = 0,
        DV_AnyURI = 4,
        DV_NonNegInt = 8,
        DV_Boolean = 16,
        DV_ID = 32,
        DV_Form = 64,
        DV_MaxOccurs = 128,
        DV_MaxOccurs1 = 256,
        DV_MinOccurs1 = 512,
        DV_ProcessContents = 1024,
        DV_Use = 2048,
        DV_WhiteSpace = 4096,

        DV_Mask = (DV_AnyURI | DV_NonNegInt | DV_Boolean | DV_ID | DV_Form |
                   DV_MaxOccurs | DV_MaxOccurs1 | DV_MinOccurs1 |
                   DV_ProcessContents | DV_Use | DV_WhiteSpace)
    };

    // generate element-attributes map table
#if defined(NEED_TO_GEN_ELEM_ATT_MAP_TABLE)
    static void initCharFlagTable();
#endif

    // -----------------------------------------------------------------------
    //  Constructor/Destructor
    // -----------------------------------------------------------------------
    GeneralAttributeCheck(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    ~GeneralAttributeCheck();

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    unsigned short getFacetId(const XMLCh* const facetName, MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------

    //deprecated
    void setIDRefList(RefHashTableOf<XMLRefInfo>* const refList);

    inline void setValidationContext(ValidationContext* const);

    // -----------------------------------------------------------------------
    //  Validation methods
    // -----------------------------------------------------------------------
    void checkAttributes(const DOMElement* const elem,
                         const unsigned short elemContext,
                         TraverseSchema* const schema,
                         const bool isTopLevel = false,
                         ValueVectorOf<DOMNode*>* const nonXSAttList = 0);

    // -----------------------------------------------------------------------
    //  Notification that lazy data has been deleted
    // -----------------------------------------------------------------------
	static void reinitGeneralAttCheck();

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    GeneralAttributeCheck(const GeneralAttributeCheck&);
    GeneralAttributeCheck& operator=(const GeneralAttributeCheck&);

    // -----------------------------------------------------------------------
    //  Setup methods
    // -----------------------------------------------------------------------
    void setUpValidators();
    void mapElements();
    void mapAttributes();

    // -----------------------------------------------------------------------
    //  Validation methods
    // -----------------------------------------------------------------------
    void validate(const DOMElement* const elem, const XMLCh* const attName, const XMLCh* const attValue,
                  const short dvIndex, TraverseSchema* const schema);

    // -----------------------------------------------------------------------
    //  Private Constants
    // -----------------------------------------------------------------------
    // optional vs. required attribute
    enum {
        Att_Required = 1,
        Att_Optional = 2,
        Att_Mask = 3
    };

    // -----------------------------------------------------------------------
    //  Private data members
    // -----------------------------------------------------------------------
    static ValueHashTableOf<unsigned short>* fAttMap;
    static ValueHashTableOf<unsigned short>* fFacetsMap;
    static DatatypeValidator*                fNonNegIntDV;
    static DatatypeValidator*                fBooleanDV;
    static DatatypeValidator*                fAnyURIDV;
    static unsigned short                    fgElemAttTable[E_Count][A_Count];
    static const XMLCh*                      fAttNames[A_Count];
    MemoryManager*                           fMemoryManager;
    ValidationContext*                       fValidationContext;
    IDDatatypeValidator                      fIDValidator;
};


// ---------------------------------------------------------------------------
//  GeneralAttributeCheck: Getter methods
// ---------------------------------------------------------------------------
inline unsigned short
GeneralAttributeCheck::getFacetId(const XMLCh* const facetName, MemoryManager* const manager) {

    return fFacetsMap->get(facetName, manager);
}

// ---------------------------------------------------------------------------
//  GeneralAttributeCheck: Setter methods
// ---------------------------------------------------------------------------
inline void GeneralAttributeCheck::setValidationContext(ValidationContext* const newValidationContext)
{
    fValidationContext = newValidationContext;
}

inline void
GeneralAttributeCheck::setIDRefList(RefHashTableOf<XMLRefInfo>* const refList) {

    fValidationContext->setIdRefList(refList);
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file GeneralAttributeCheck.hpp
  */

