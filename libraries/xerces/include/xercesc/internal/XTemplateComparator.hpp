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
 * $Log: XTemplateComparator.hpp,v $
 * Revision 1.1  2003/10/29 16:14:15  peiyongz
 * XObjectComparator/XTemplateComparator
 *
 * $Id: XTemplateComparator.hpp,v 1.1 2003/10/29 16:14:15 peiyongz Exp $
 *
 */

#if !defined(XTEMPLATE_COMPARATOR_HPP)
#define XTEMPLATE_COMPARATOR_HPP

#include <xercesc/util/ValueVectorOf.hpp>
#include <xercesc/util/RefArrayVectorOf.hpp>
#include <xercesc/util/RefVectorOf.hpp>
#include <xercesc/util/RefHashTableOf.hpp>
#include <xercesc/util/RefHash2KeysTableOf.hpp>
#include <xercesc/util/RefHash3KeysIdPool.hpp>
#include <xercesc/util/NameIdPool.hpp>

#include <xercesc/framework/XMLNotationDecl.hpp>
#include <xercesc/framework/XMLRefInfo.hpp>
#include <xercesc/util/XMLNumber.hpp>
#include <xercesc/validators/common/ContentSpecNode.hpp>
#include <xercesc/validators/DTD/DTDAttDef.hpp>
#include <xercesc/validators/DTD/DTDElementDecl.hpp>
#include <xercesc/validators/DTD/DTDEntityDecl.hpp>
#include <xercesc/validators/schema/SchemaAttDef.hpp>
#include <xercesc/validators/schema/SchemaElementDecl.hpp>
#include <xercesc/validators/schema/XercesGroupInfo.hpp>
#include <xercesc/validators/schema/XercesAttGroupInfo.hpp>
#include <xercesc/validators/schema/SchemaGrammar.hpp>
#include <xercesc/validators/schema/identity/IC_Field.hpp>
#include <xercesc/validators/schema/identity/IdentityConstraint.hpp>
#include <xercesc/validators/schema/identity/XercesXPath.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLUTIL_EXPORT XTemplateComparator
{
public:

    /**********************************************************
     *
     * ValueVectorOf
     *
     *   SchemaElementDecl*
     *   unsigned int
     *
     ***********************************************************/   
    static bool           isEquivalent(
                                          ValueVectorOf<SchemaElementDecl*>* const lValue
                                        , ValueVectorOf<SchemaElementDecl*>* const rValue
                                       );

    static bool           isEquivalent(
                                          ValueVectorOf<unsigned int>* const lValue
                                        , ValueVectorOf<unsigned int>* const rValue
                                       );

    /**********************************************************
     *
     * RefArrayVectorOf
     *
     *   XMLCh
     *
     ***********************************************************/   
    static bool           isEquivalent(
                                          RefArrayVectorOf<XMLCh>* const lValue
                                        , RefArrayVectorOf<XMLCh>* const rValue
                                       );

    /**********************************************************
     *
     * RefVectorOf
     *
     *   SchemaAttDef
     *   SchemaElementDecl
     *   ContentSpecNode
     *   IC_Field
     *   DatatypeValidator
     *   IdentityConstraint
     *   XMLNumber
     *   XercesLocationPath
     *   XercesStep
     *
     ***********************************************************/
    static bool           isEquivalent(
                                          RefVectorOf<SchemaAttDef>* const lValue
                                        , RefVectorOf<SchemaAttDef>* const rValue
                                       );

    static bool           isEquivalent(
                                          RefVectorOf<SchemaElementDecl>* const lValue
                                        , RefVectorOf<SchemaElementDecl>* const rValue
                                       );

    static bool           isEquivalent(
                                          RefVectorOf<ContentSpecNode>* const lValue
                                        , RefVectorOf<ContentSpecNode>* const rValue
                                       );

    static bool           isEquivalent(
                                          RefVectorOf<IC_Field>* const lValue
                                        , RefVectorOf<IC_Field>* const rValue
                                       );

    static bool           isEquivalent(
                                          RefVectorOf<DatatypeValidator>* const lValue
                                        , RefVectorOf<DatatypeValidator>* const rValue
                                       );

    static bool           isEquivalent(
                                          RefVectorOf<IdentityConstraint>* const lValue
                                        , RefVectorOf<IdentityConstraint>* const rValue
                                       );

    static bool           isEquivalent(
                                          RefVectorOf<XMLNumber>* const lValue
                                        , RefVectorOf<XMLNumber>* const rValue
                                       );

    static bool           isEquivalent(
                                          RefVectorOf<XercesLocationPath>* const lValue
                                        , RefVectorOf<XercesLocationPath>* const rValue
                                       );

    static bool           isEquivalent(
                                          RefVectorOf<XercesStep>* const lValue
                                        , RefVectorOf<XercesStep>* const rValue
                                       );

    /**********************************************************
     *
     * RefHashTableOf
     *
     *   KVStringPair
     *   XMLAttDef
     *   DTDAttDef
     *   ComplexTypeInfo
     *   XercesGroupInfo
     *   XercesAttGroupInfo
     *   XMLRefInfo
     *   DatatypeValidator
     *   Grammar
     *
     ***********************************************************/
    static bool           isEquivalent(
                                          RefHashTableOf<KVStringPair>* const lValue
                                        , RefHashTableOf<KVStringPair>* const rValue
                                       );

    static bool           isEquivalent(
                                          RefHashTableOf<XMLAttDef>* const lValue
                                        , RefHashTableOf<XMLAttDef>* const rValue
                                       );

    static bool           isEquivalent(
                                          RefHashTableOf<DTDAttDef>* const lValue
                                        , RefHashTableOf<DTDAttDef>* const rValue
                                       );

    static bool           isEquivalent(
                                          RefHashTableOf<ComplexTypeInfo>* const lValue
                                        , RefHashTableOf<ComplexTypeInfo>* const rValue
                                       );

    static bool           isEquivalent(
                                          RefHashTableOf<XercesGroupInfo>* const lValue
                                        , RefHashTableOf<XercesGroupInfo>* const rValue
                                       );

    static bool           isEquivalent(
                                          RefHashTableOf<XercesAttGroupInfo>* const lValue
                                        , RefHashTableOf<XercesAttGroupInfo>* const rValue
                                       );

    static bool           isEquivalent(
                                          RefHashTableOf<XMLRefInfo>* const lValue
                                        , RefHashTableOf<XMLRefInfo>* const rValue
                                       );

    static bool           isEquivalent(
                                          RefHashTableOf<DatatypeValidator>* const lValue
                                        , RefHashTableOf<DatatypeValidator>* const rValue
                                       );

    static bool           isEquivalent(
                                          RefHashTableOf<Grammar>* const lValue
                                        , RefHashTableOf<Grammar>* const rValue
                                       );

    /**********************************************************
     *
     * RefHash2KeysTableOf
     *
     *   SchemaAttDef
     *   ElemVector
     *
     ***********************************************************/
    static bool           isEquivalent(
                                          RefHash2KeysTableOf<SchemaAttDef>* const lValue
                                        , RefHash2KeysTableOf<SchemaAttDef>* const rValue
                                       );

    static bool           isEquivalent(
                                          RefHash2KeysTableOf<ElemVector>* const lValue
                                        , RefHash2KeysTableOf<ElemVector>* const rValue
                                       );

    /**********************************************************
     *
     * RefHash3KeysIdPool
     *
     *   SchemaElementDecl
     *
     ***********************************************************/
    static bool           isEquivalent(
                                         RefHash3KeysIdPool<SchemaElementDecl>* const lop
                                       , RefHash3KeysIdPool<SchemaElementDecl>* const rop
                                      );


    /**********************************************************
     *
     * NameIdPool
     *
     *   DTDElementDecl
     *   DTDEntityDecl
     *   XMLNotationDecl
     *
     ***********************************************************/
    static bool           isEquivalent(
                                          NameIdPool<DTDElementDecl>* const lValue
                                        , NameIdPool<DTDElementDecl>* const rValue
                                       );

    static bool           isEquivalent(
                                          NameIdPool<DTDEntityDecl>* const lValue
                                        , NameIdPool<DTDEntityDecl>* const rValue
                                       );

    static bool           isEquivalent(
                                          NameIdPool<XMLNotationDecl>* const lValue
                                        , NameIdPool<XMLNotationDecl>* const rValue
                                       );

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
	~XTemplateComparator();
    XTemplateComparator();
    XTemplateComparator(const XTemplateComparator&);
	XTemplateComparator& operator=(const XTemplateComparator&);

};

XERCES_CPP_NAMESPACE_END

#endif
