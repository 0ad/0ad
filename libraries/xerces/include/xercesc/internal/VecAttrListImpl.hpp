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
 * $Log: VecAttrListImpl.hpp,v $
 * Revision 1.4  2003/05/15 18:26:29  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.3  2003/03/07 18:08:58  tng
 * Return a reference instead of void for operator=
 *
 * Revision 1.2  2002/11/04 14:58:18  tng
 * C++ Namespace Support.
 *
 * Revision 1.1.1.1  2002/02/01 22:21:58  peiyongz
 * sane_include
 *
 * Revision 1.4  2000/02/24 20:18:07  abagchi
 * Swat for removing Log from API docs
 *
 * Revision 1.3  2000/02/06 07:47:53  rahulj
 * Year 2K copyright swat.
 *
 * Revision 1.2  1999/12/15 19:49:37  roddey
 * Added second getValue() method which takes a short name for the attribute
 * to get the value for. Just a convenience method.
 *
 * Revision 1.1.1.1  1999/11/09 01:08:19  twl
 * Initial checkin
 *
 * Revision 1.2  1999/11/08 20:44:45  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */


#if !defined(VECATTRLISTIMPL_HPP)
#define VECATTRLISTIMPL_HPP

#include <xercesc/sax/AttributeList.hpp>
#include <xercesc/framework/XMLAttr.hpp>
#include <xercesc/util/RefVectorOf.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLPARSER_EXPORT VecAttrListImpl : public XMemory, public AttributeList
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    VecAttrListImpl();
    ~VecAttrListImpl();


    // -----------------------------------------------------------------------
    //  Implementation of the attribute list interface
    // -----------------------------------------------------------------------
    virtual unsigned int getLength() const;
    virtual const XMLCh* getName(const unsigned int index) const;
    virtual const XMLCh* getType(const unsigned int index) const;
    virtual const XMLCh* getValue(const unsigned int index) const;
    virtual const XMLCh* getType(const XMLCh* const name) const;
    virtual const XMLCh* getValue(const XMLCh* const name) const;
    virtual const XMLCh* getValue(const char* const name) const;


    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    void setVector
    (
        const   RefVectorOf<XMLAttr>* const srcVec
        , const unsigned int                count
        , const bool                        adopt = false
    );


private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    VecAttrListImpl(const VecAttrListImpl&);
    VecAttrListImpl& operator=(const VecAttrListImpl&);


    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fAdopt
    //      Indicates whether the passed vector is to be adopted or not. If
    //      so, we destroy it when we are destroyed (and when a new vector is
    //      set!)
    //
    //  fCount
    //      The count of elements in the vector that should be considered
    //      valid. This is an optimization to allow vector elements to be
    //      reused over and over but a different count of them be valid for
    //      each use.
    //
    //  fVector
    //      The vector that provides the backing for the list.
    // -----------------------------------------------------------------------
    bool                        fAdopt;
    unsigned int                fCount;
    const RefVectorOf<XMLAttr>* fVector;
};

XERCES_CPP_NAMESPACE_END

#endif
