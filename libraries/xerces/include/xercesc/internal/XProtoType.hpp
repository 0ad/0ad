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
 * $Id: XProtoType.hpp,v 1.4 2004/01/29 11:46:30 cargilld Exp $
 * $Log: XProtoType.hpp,v $
 * Revision 1.4  2004/01/29 11:46:30  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.3  2003/12/17 00:18:34  cargilld
 * Update to memory management so that the static memory manager (one used to call Initialize) is only for static data.
 *
 * Revision 1.2  2003/09/23 18:12:19  peiyongz
 * Macro re-organized: provide create/nocreate macros for abstract and
 * nonabstract classes
 *
 * Revision 1.1  2003/09/18 18:31:24  peiyongz
 * OSU: Object Serialization Utilities
 *
 */


#if !defined(XPROTOTYPE_HPP)
#define XPROTOTYPE_HPP

#include <xercesc/util/PlatformUtils.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XSerializeEngine;
class XSerializable;

class XMLUTIL_EXPORT XProtoType
{
public:

           void       store(XSerializeEngine& serEng) const;

    static void        load(XSerializeEngine&          serEng
                          , XMLByte*          const    name
                          , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
                          );

    // -------------------------------------------------------------------------------
    //  data
    //
    //  fClassName: 
    //            name of the XSerializable derivatives
    //
    //  fCreateObject:
    //            pointer to the factory method (createObject()) 
    //            of the XSerializable derivatives
    //
    // -------------------------------------------------------------------------------

    XMLByte*          fClassName;

    XSerializable*    (*fCreateObject)(MemoryManager*);

};

#define DECL_XPROTOTYPE(class_name) \
static  XProtoType        class##class_name;                   \
static  XSerializable*    createObject(MemoryManager* manager);

/***
 * For non-abstract class
 ***/
#define IMPL_XPROTOTYPE_TOCREATE(class_name) \
IMPL_XPROTOTYPE_INSTANCE(class_name) \
XSerializable* class_name::createObject(MemoryManager* manager) \
{return new (manager) class_name(manager);}

/***
* For abstract class
 ***/
#define IMPL_XPROTOTYPE_NOCREATE(class_name) \
IMPL_XPROTOTYPE_INSTANCE(class_name) \
XSerializable* class_name::createObject(MemoryManager*) \
{return 0;}


/***
 * Helper Macro 
 ***/
#define XPROTOTYPE_CLASS(class_name) ((XProtoType*)(&class_name::class##class_name))

#define IMPL_XPROTOTYPE_INSTANCE(class_name) \
XProtoType class_name::class##class_name = \
{(XMLByte*) #class_name, class_name::createObject };

XERCES_CPP_NAMESPACE_END

#endif
