/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2003 The Apache Software Foundation.  All rights
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
 * $Id: XSerializable.hpp,v 1.3 2003/10/29 16:16:08 peiyongz Exp $
 * $Log: XSerializable.hpp,v $
 * Revision 1.3  2003/10/29 16:16:08  peiyongz
 * GrammarPool' serialization/deserialization support
 *
 * Revision 1.2  2003/09/23 18:12:19  peiyongz
 * Macro re-organized: provide create/nocreate macros for abstract and
 * nonabstract classes
 *
 * Revision 1.1  2003/09/18 18:31:24  peiyongz
 * OSU: Object Serialization Utilities
 *
 *
 */

#if !defined(XSERIALIZABLE_HPP)
#define XSERIALIZABLE_HPP

#include <xercesc/internal/XSerializeEngine.hpp>
#include <xercesc/internal/XProtoType.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLUTIL_EXPORT XSerializable
{
public :

    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    virtual ~XSerializable() {} ;

    // -----------------------------------------------------------------------
    //  Serialization Interface
    // -----------------------------------------------------------------------   
    virtual bool        isSerializable()               const = 0;

    virtual void        serialize(XSerializeEngine& )        = 0;

    virtual XProtoType* getProtoType()                 const = 0;

protected:
    XSerializable(){} ;

private:
    // -----------------------------------------------------------------------
    //  Unimplemented copy ctor and assignment operator
    // -----------------------------------------------------------------------
	XSerializable(const XSerializable& );              
	XSerializable& operator=(const XSerializable&);

};

inline void XSerializable::serialize(XSerializeEngine& )
{
}

/***
 * Macro to be included in XSerializable derivatives'
 * declaration's public section
 ***/
#define DECL_XSERIALIZABLE(class_name) \
public: \
\
DECL_XPROTOTYPE(class_name) \
\
virtual bool                    isSerializable()                  const ;  \
virtual XProtoType*             getProtoType()                    const;   \
virtual void                    serialize(XSerializeEngine&); \
friend  class XObjectComparator;   \
friend  class XTemplateComparator; \
\
inline friend XSerializeEngine& operator>>(XSerializeEngine& serEng  \
                                         , class_name*&      objPtr) \
{objPtr = (class_name*) serEng.read(XPROTOTYPE_CLASS(class_name));   \
 return serEng; \
};
	
/***
 * Macro to be included in the implementation file
 * of XSerializable derivatives' which is instantiable
 ***/
#define IMPL_XSERIALIZABLE_TOCREATE(class_name) \
IMPL_XPROTOTYPE_TOCREATE(class_name) \
IMPL_XSERIAL(class_name)

/***
 * Macro to be included in the implementation file
 * of XSerializable derivatives' which is UN-instantiable
 ***/
#define IMPL_XSERIALIZABLE_NOCREATE(class_name) \
IMPL_XPROTOTYPE_NOCREATE(class_name) \
IMPL_XSERIAL(class_name)

/***
 * Helper Macro 
 ***/
#define IMPL_XSERIAL(class_name) \
bool        class_name::isSerializable() const \
{return true; } \
XProtoType* class_name::getProtoType()   const \
{return XPROTOTYPE_CLASS(class_name); } 

#define IS_EQUIVALENT(lptr, rptr) \
    if (lptr == rptr)             \
        return true;              \
    if (( lptr && !rptr) || (!lptr &&  rptr))  \
        return false;

XERCES_CPP_NAMESPACE_END

#endif

