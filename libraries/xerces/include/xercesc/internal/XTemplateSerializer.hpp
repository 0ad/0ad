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
 * $Log: XTemplateSerializer.hpp,v $
 * Revision 1.3  2003/11/11 22:48:13  knoaman
 * Serialization of XSAnnotation.
 *
 * Revision 1.2  2003/10/29 16:16:08  peiyongz
 * GrammarPool' serialization/deserialization support
 *
 * Revision 1.1  2003/10/17 21:07:49  peiyongz
 * To support Template object serialization/deserialization
 *
 * $Id: XTemplateSerializer.hpp,v 1.3 2003/11/11 22:48:13 knoaman Exp $
 *
 */

#if !defined(XTEMPLATE_SERIALIZER_HPP)
#define XTEMPLATE_SERIALIZER_HPP

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
#include <xercesc/framework/psvi/XSAnnotation.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLUTIL_EXPORT XTemplateSerializer
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
    static void           storeObject(ValueVectorOf<SchemaElementDecl*>* const tempObjToWrite
                                    , XSerializeEngine&                        serEng);

    static void           loadObject(ValueVectorOf<SchemaElementDecl*>**       tempObjToRead
                                   , int                                       initSize
                                   , bool                                      toCallDestructor
                                   , XSerializeEngine&                         serEng);

    static void           storeObject(ValueVectorOf<unsigned int>* const tempObjToWrite
                                    , XSerializeEngine&                  serEng);

    static void           loadObject(ValueVectorOf<unsigned int>**       tempObjToRead
                                   , int                                 initSize
                                   , bool                                toCallDestructor
                                   , XSerializeEngine&                   serEng);

    /**********************************************************
     *
     * RefArrayVectorOf
     *
     *   XMLCh
     *
     ***********************************************************/   
    static void           storeObject(RefArrayVectorOf<XMLCh>* const tempObjToWrite
                                    , XSerializeEngine&              serEng);

    static void           loadObject(RefArrayVectorOf<XMLCh>**       tempObjToRead
                                   , int                             initSize
                                   , bool                            toAdopt
                                   , XSerializeEngine&               serEng);

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
    static void           storeObject(RefVectorOf<SchemaAttDef>* const tempObjToWrite
                                    , XSerializeEngine&                serEng);

    static void           loadObject(RefVectorOf<SchemaAttDef>**       tempObjToRead
                                   , int                               initSize
                                   , bool                              toAdopt
                                   , XSerializeEngine&                 serEng);

    static void           storeObject(RefVectorOf<SchemaElementDecl>* const tempObjToWrite
                                    , XSerializeEngine&                     serEng);

    static void           loadObject(RefVectorOf<SchemaElementDecl>**       tempObjToRead
                                   , int                                    initSize
                                   , bool                                   toAdopt
                                   , XSerializeEngine&                      serEng);

    static void           storeObject(RefVectorOf<ContentSpecNode>* const tempObjToWrite
                                    , XSerializeEngine&                   serEng);

    static void           loadObject(RefVectorOf<ContentSpecNode>**       tempObjToRead
                                   , int                                  initSize
                                   , bool                                 toAdopt
                                   , XSerializeEngine&                    serEng);

    static void           storeObject(RefVectorOf<IC_Field>* const tempObjToWrite
                                    , XSerializeEngine&            serEng);

    static void           loadObject(RefVectorOf<IC_Field>**       tempObjToRead
                                   , int                           initSize
                                   , bool                          toAdopt
                                   , XSerializeEngine&             serEng);

    static void           storeObject(RefVectorOf<DatatypeValidator>* const tempObjToWrite
                                    , XSerializeEngine&                     serEng);

    static void           loadObject(RefVectorOf<DatatypeValidator>**       tempObjToRead
                                   , int                                    initSize
                                   , bool                                   toAdopt
                                   , XSerializeEngine&                      serEng);
 
    static void           storeObject(RefVectorOf<IdentityConstraint>* const tempObjToWrite
                                    , XSerializeEngine&                      serEng);

    static void           loadObject(RefVectorOf<IdentityConstraint>**       tempObjToRead
                                   , int                                     initSize
                                   , bool                                    toAdopt
                                   , XSerializeEngine&                       serEng);

    static void           storeObject(RefVectorOf<XMLNumber>* const tempObjToWrite
                                    , XSerializeEngine&             serEng);

    static void           loadObject(RefVectorOf<XMLNumber>**       tempObjToRead
                                   , int                            initSize
                                   , bool                           toAdopt
                                   , XMLNumber::NumberType          numType
                                   , XSerializeEngine&              serEng);

    static void           storeObject(RefVectorOf<XercesLocationPath>* const tempObjToWrite
                                    , XSerializeEngine&                      serEng);

    static void           loadObject(RefVectorOf<XercesLocationPath>**       tempObjToRead
                                   , int                                     initSize
                                   , bool                                    toAdopt
                                   , XSerializeEngine&                       serEng);

    static void           storeObject(RefVectorOf<XercesStep>* const tempObjToWrite
                                    , XSerializeEngine&              serEng);

    static void           loadObject(RefVectorOf<XercesStep>**       tempObjToRead
                                   , int                             initSize
                                   , bool                            toAdopt
                                   , XSerializeEngine&               serEng);

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
     *   XSAnnotation
     *
     ***********************************************************/
    static void           storeObject(RefHashTableOf<KVStringPair>* const tempObjToWrite
                                    , XSerializeEngine&                   serEng);

    static void           loadObject(RefHashTableOf<KVStringPair>**       tempObjToRead
                                   , int                                  initSize
                                   , bool                                 toAdopt
                                   , XSerializeEngine&                    serEng);

    static void           storeObject(RefHashTableOf<XMLAttDef>* const tempObjToWrite
                                    , XSerializeEngine&                serEng);

    static void           loadObject(RefHashTableOf<XMLAttDef>**       tempObjToRead
                                   , int                               initSize
                                   , bool                              toAdopt
                                   , XSerializeEngine&                 serEng);

    static void           storeObject(RefHashTableOf<DTDAttDef>* const tempObjToWrite
                                    , XSerializeEngine&                serEng);

    static void           loadObject(RefHashTableOf<DTDAttDef>**       tempObjToRead
                                   , int                               initSize
                                   , bool                              toAdopt
                                   , XSerializeEngine&                 serEng);

    static void           storeObject(RefHashTableOf<ComplexTypeInfo>* const tempObjToWrite
                                    , XSerializeEngine&                      serEng);

    static void           loadObject(RefHashTableOf<ComplexTypeInfo>**       tempObjToRead
                                   , int                                     initSize
                                   , bool                                    toAdopt
                                   , XSerializeEngine&                       serEng);

    static void           storeObject(RefHashTableOf<XercesGroupInfo>* const tempObjToWrite
                                    , XSerializeEngine&                      serEng);

    static void           loadObject(RefHashTableOf<XercesGroupInfo>**       tempObjToRead
                                   , int                                     initSize
                                   , bool                                    toAdopt
                                   , XSerializeEngine&                       serEng);

    static void           storeObject(RefHashTableOf<XercesAttGroupInfo>* const tempObjToWrite
                                    , XSerializeEngine&                         serEng);

    static void           loadObject(RefHashTableOf<XercesAttGroupInfo>**       tempObjToRead
                                   , int                                        initSize
                                   , bool                                       toAdopt
                                   , XSerializeEngine&                          serEng);

    static void           storeObject(RefHashTableOf<XMLRefInfo>* const tempObjToWrite
                                    , XSerializeEngine&                 serEng);

    static void           loadObject(RefHashTableOf<XMLRefInfo>**       tempObjToRead
                                   , int                                initSize
                                   , bool                               toAdopt
                                   , XSerializeEngine&                  serEng);

    static void           storeObject(RefHashTableOf<DatatypeValidator>* const tempObjToWrite
                                    , XSerializeEngine&                        serEng);

    static void           loadObject(RefHashTableOf<DatatypeValidator>**       tempObjToRead
                                   , int                                       initSize
                                   , bool                                      toAdopt
                                   , XSerializeEngine&                         serEng);

    static void           storeObject(RefHashTableOf<Grammar>* const tempObjToWrite
                                    , XSerializeEngine&              serEng);

    static void           loadObject(RefHashTableOf<Grammar>**       tempObjToRead
                                   , int                             initSize
                                   , bool                            toAdopt
                                   , XSerializeEngine&               serEng);

    static void           storeObject(RefHashTableOf<XSAnnotation>* const tempObjToWrite
                                    , XSerializeEngine&                   serEng);

    static void           loadObject(RefHashTableOf<XSAnnotation>**  tempObjToRead
                                   , int                             initSize
                                   , bool                            toAdopt
                                   , XSerializeEngine&               serEng);

    /**********************************************************
     *
     * RefHash2KeysTableOf
     *
     *   SchemaAttDef
     *   ElemVector
     *
     ***********************************************************/
    static void           storeObject(RefHash2KeysTableOf<SchemaAttDef>* const tempObjToWrite
                                    , XSerializeEngine&                        serEng);

    static void           loadObject(RefHash2KeysTableOf<SchemaAttDef>**       tempObjToRead
                                   , int                                       initSize
                                   , bool                                      toAdopt
                                   , XSerializeEngine&                         serEng);

    static void           storeObject(RefHash2KeysTableOf<ElemVector>* const tempObjToWrite
                                    , XSerializeEngine&                      serEng);

    static void           loadObject(RefHash2KeysTableOf<ElemVector>**       tempObjToRead
                                   , int                                     initSize
                                   , bool                                    toAdopt
                                   , XSerializeEngine&                       serEng);

    /**********************************************************
     *
     * RefHash3KeysIdPool
     *
     *   SchemaElementDecl
     *
     ***********************************************************/
    static void           storeObject(RefHash3KeysIdPool<SchemaElementDecl>* const tempObjToWrite
                                    , XSerializeEngine&                            serEng);

    static void           loadObject(RefHash3KeysIdPool<SchemaElementDecl>**       tempObjToRead
                                   , int                                           initSize
                                   , bool                                          toAdopt
                                   , int                                           initSize2
                                   , XSerializeEngine&                             serEng);

    /**********************************************************
     *
     * NameIdPool
     *
     *   DTDElementDecl
     *   DTDEntityDecl
     *   XMLNotationDecl
     *
     ***********************************************************/
    static void           storeObject(NameIdPool<DTDElementDecl>* const tempObjToWrite
                                    , XSerializeEngine&                 serEng);

    static void           loadObject(NameIdPool<DTDElementDecl>**       tempObjToRead
                                   , int                                initSize
                                   , int                                initSize2
                                   , XSerializeEngine&                  serEng);

    static void           storeObject(NameIdPool<DTDEntityDecl>* const tempObjToWrite
                                    , XSerializeEngine&                serEng);

    static void           loadObject(NameIdPool<DTDEntityDecl>**       tempObjToRead
                                   , int                               initSize
                                   , int                               initSize2
                                   , XSerializeEngine&                 serEng);

    static void           storeObject(NameIdPool<XMLNotationDecl>* const tempObjToWrite
                                    , XSerializeEngine&                  serEng);

    static void           loadObject(NameIdPool<XMLNotationDecl>**      tempObjToRead
                                   , int                                initSize
                                   , int                                initSize2
                                   , XSerializeEngine&                  serEng);

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
	~XTemplateSerializer();
    XTemplateSerializer();
    XTemplateSerializer(const XTemplateSerializer&);
	XTemplateSerializer& operator=(const XTemplateSerializer&);

};

XERCES_CPP_NAMESPACE_END

#endif
