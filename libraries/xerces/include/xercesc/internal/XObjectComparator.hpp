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
 * $Log: XObjectComparator.hpp,v $
 * Revision 1.2  2003/10/31 22:15:42  peiyongz
 * dumpContent
 *
 * Revision 1.1  2003/10/29 16:14:15  peiyongz
 * XObjectComparator/XTemplateComparator
 *
 * $Id: XObjectComparator.hpp,v 1.2 2003/10/31 22:15:42 peiyongz Exp $
 *
 */

#if !defined(XOBJECT_COMPARATOR_HPP)
#define XOBJECT_COMPARATOR_HPP

#include <xercesc/framework/XMLNotationDecl.hpp>
#include <xercesc/framework/XMLRefInfo.hpp>
#include <xercesc/framework/XMLDTDDescription.hpp>
#include <xercesc/framework/XMLSchemaDescription.hpp>

#include <xercesc/internal/XMLGrammarPoolImpl.hpp>

#include <xercesc/util/XMLNumber.hpp>
#include <xercesc/util/KVStringPair.hpp>

#include <xercesc/validators/common/ContentSpecNode.hpp>

#include <xercesc/validators/DTD/DTDAttDef.hpp>
#include <xercesc/validators/DTD/DTDAttDefList.hpp>
#include <xercesc/validators/DTD/DTDElementDecl.hpp>
#include <xercesc/validators/DTD/DTDEntityDecl.hpp>
#include <xercesc/validators/DTD/DTDGrammar.hpp>

#include <xercesc/validators/schema/SchemaAttDef.hpp>
#include <xercesc/validators/schema/SchemaAttDefList.hpp>
#include <xercesc/validators/schema/SchemaElementDecl.hpp>
#include <xercesc/validators/schema/XercesGroupInfo.hpp>
#include <xercesc/validators/schema/XercesAttGroupInfo.hpp>
#include <xercesc/validators/schema/SchemaGrammar.hpp>

#include <xercesc/validators/schema/identity/IC_Field.hpp>
#include <xercesc/validators/schema/identity/IC_Selector.hpp>
#include <xercesc/validators/schema/identity/IC_Key.hpp>
#include <xercesc/validators/schema/identity/IC_KeyRef.hpp>
#include <xercesc/validators/schema/identity/IC_Unique.hpp>
#include <xercesc/validators/schema/identity/IdentityConstraint.hpp>
#include <xercesc/validators/schema/identity/XercesXPath.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLUTIL_EXPORT XObjectComparator
{
public:

/**********************************************************
 *
 * XMLGrammarPoolImpl
 *
 * Grammar
 *
 *   SchemaGrammar
 *   DTDGrammar
 *
 ***********************************************************/   
    static void dumpContent
                (
                    XMLGrammarPoolImpl* const
                );

    static bool isEquivalent
                (
                    XMLGrammarPoolImpl* const
                  , XMLGrammarPoolImpl* const
                );

    static bool isEquivalent
                (
                    Grammar* const
                  , Grammar* const
                );

    static bool isBaseEquivalent
                (
                    Grammar* const
                  , Grammar* const
                );

    static bool isEquivalent
                (
                   SchemaGrammar* const
                 , SchemaGrammar* const
                );

/**********************************************************
 *
 * XMLGrammarDescription
 *
 *   XMLSchemaDescription
 *   XMLDTDDescription
 *
 ***********************************************************/   
    static bool isEquivalent
                (
                    XMLSchemaDescription* const
                  , XMLSchemaDescription* const
                );

    static bool isEquivalent
                (
                   XMLDTDDescription* const
                 , XMLDTDDescription* const
                );

/**********************************************************
 *
 * XMLElementDecl
 *   SchemaElementDecl
 *   DTDElementDecl
 *
 ***********************************************************/    
    static bool isBaseEquivalent
                (
                    XMLElementDecl* const
                  , XMLElementDecl* const
                );

    static bool isEquivalent
                (
                   SchemaElementDecl* const
                 , SchemaElementDecl* const
                );

    static bool isEquivalent
                (
                   DTDElementDecl* const
                 , DTDElementDecl* const
                );

/**********************************************************
 * XMLAttDef
 *   SchemaAttDef
 *   DTDAttDef
 *
***********************************************************/    
    static bool isBaseEquivalent
                (
                   XMLAttDef* const
                 , XMLAttDef* const
                );

    static bool isEquivalent
                (
                    SchemaAttDef* const
                  , SchemaAttDef* const
                );

    static bool isEquivalent
                (
                    DTDAttDef* const
                  , DTDAttDef* const
                );

/**********************************************************
 * XMLAttDefList
 *   SchemaAttDefList
 *   DTDAttDefList
 *
***********************************************************/   
    static bool isBaseEquivalent
                (
                   XMLAttDefList* const
                 , XMLAttDefList* const
                );
    
    static bool isEquivalent
                (
                    SchemaAttDefList* const
                  , SchemaAttDefList* const
                );

    static bool isEquivalent
                (
                    DTDAttDefList* const
                  , DTDAttDefList* const
                );

/**********************************************************
 * XMLEntityDecl
 *    DTDEntityDecl
 *
 ***********************************************************/   
    static bool isBaseEquivalent
                (
                    XMLEntityDecl* const
                  , XMLEntityDecl* const
                );

    static bool isEquivalent
                (
                    DTDEntityDecl* const
                  , DTDEntityDecl* const
                );

/**********************************************************
 * XMLNotationDecl
 *
 * DTDEntityDecl
 *
 * ComplexTypeInfo
 * XercesGroupInfo
 * XercesAttGroupInfo
 ***********************************************************/   
    static bool isEquivalent
                (
                    XMLNotationDecl* const
                  , XMLNotationDecl* const
                );
   
    static bool isEquivalent
                (
                    ComplexTypeInfo* const
                  , ComplexTypeInfo* const
                );

    static bool isEquivalent
                (
                    XercesGroupInfo* const
                  , XercesGroupInfo* const
                );

    static bool isEquivalent
                (
                    XercesAttGroupInfo* const
                  , XercesAttGroupInfo* const
                );

/**********************************************************
 *
 * DatatypeValidator
 *
 *
 * DatatypeValidatorFactory
 *
 ***********************************************************/   

    static bool isEquivalent
                (
                   DatatypeValidator* const
                 , DatatypeValidator* const
                );

    static bool isBaseEquivalent
                (
                   DatatypeValidator* const
                 , DatatypeValidator* const
                );

    static bool isEquivalent
                (
                   DatatypeValidatorFactory* const
                 , DatatypeValidatorFactory* const
                );

/**********************************************************
 *
 * ContentSpecNode
 * QName
 * KVStringPair
 * XMLRefInfo
 * XMLStringPool
 *
 ***********************************************************/   
    static bool isEquivalent
                (
                   ContentSpecNode* const
                 , ContentSpecNode* const
                );

    static bool isEquivalent
                (
                   QName* const
                 , QName* const
                );

    static bool isEquivalent
                (
                   KVStringPair* const
                 , KVStringPair* const
                );

    static bool isEquivalent
                (
                   XMLRefInfo* const
                 , XMLRefInfo* const
                );

    static bool isEquivalent
                (
                   XMLStringPool* const
                 , XMLStringPool* const
                );

/**********************************************************
 *
 * XercesNodeTest
 * XercesStep
 * XercesLocationPath
 * XercesXPath
 *
***********************************************************/   
    static bool isEquivalent
                (
                   XercesNodeTest* const
                 , XercesNodeTest* const
                );

    static bool isEquivalent
                (
                   XercesStep* const
                 , XercesStep* const
                );

    static bool isEquivalent
                (
                   XercesLocationPath* const
                 , XercesLocationPath* const
                );

    static bool isEquivalent
                (
                   XercesXPath* const
                 , XercesXPath* const
                );

/**********************************************************
 *
 * IC_Field
 * IC_Selector
 *
 * IdentityConstraint
 *   IC_Key
 *   IC_KeyRef
 *   IC_Unique
 *
 ***********************************************************/   
    static bool isEquivalent
                (
                   IC_Field* const
                 , IC_Field* const
                );

    static bool isEquivalent
                (
                   IC_Selector* const
                 , IC_Selector* const
                );

    static bool isEquivalent
                (
                   IdentityConstraint* const
                 , IdentityConstraint* const
                );

    static bool isBaseEquivalent
                (
                   IdentityConstraint* const
                 , IdentityConstraint* const
                );

    static bool isEquivalent
                (
                   IC_Key* const
                 , IC_Key* const
                );

    static bool isEquivalent
                (
                   IC_KeyRef* const
                 , IC_KeyRef* const
                );

    static bool isEquivalent
                (
                   IC_Unique* const
                 , IC_Unique* const
                );

/**********************************************************
 * XMLNumber
 *   XMLDouble
 *   XMLFloat
 *   XMLBigDecimal
 *   XMLDateTime
 *
 ***********************************************************/   
    static bool isEquivalent
                (
                   XMLNumber* const
                 , XMLNumber* const
                );

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
	~XObjectComparator();
    XObjectComparator();
    XObjectComparator(const XObjectComparator&);
	XObjectComparator& operator=(const XObjectComparator&);

};

XERCES_CPP_NAMESPACE_END

#endif
