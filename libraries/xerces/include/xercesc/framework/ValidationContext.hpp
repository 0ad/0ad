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
 * $Log: ValidationContext.hpp,v $
 * Revision 1.2  2003/11/24 05:10:26  neilg
 * implement method for determining member type of union that validated some value
 *
 * Revision 1.1  2003/11/12 20:28:16  peiyongz
 * Stateless Grammar: ValidationContext
 *
 * $Id: ValidationContext.hpp,v 1.2 2003/11/24 05:10:26 neilg Exp $
 *
 */

#if !defined(VALIDATION_CONTEXT_HPP)
#define VALIDATION_CONTEXT_HPP

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/RefHashTableOf.hpp>
#include <xercesc/util/NameIdPool.hpp>
#include <xercesc/util/XMemory.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLRefInfo;
class DTDEntityDecl;
class DatatypeValidator;

class XMLPARSER_EXPORT ValidationContext : public XMemory
{
public :
    // -----------------------------------------------------------------------
    /** @name Virtual destructor for derived classes */
    // -----------------------------------------------------------------------
    //@{

    /**
      * virtual destructor
      *
      */
    virtual ~ValidationContext(){};
    //@}

    // -----------------------------------------------------------------------
    /** @name The ValidationContext Interface */
    // -----------------------------------------------------------------------
    //@{

    /**
      * IDRefList
      *
      */
    virtual RefHashTableOf<XMLRefInfo>*  getIdRefList() const = 0;

    virtual void                         setIdRefList(RefHashTableOf<XMLRefInfo>* const) = 0;

    virtual void                         clearIdRefList() = 0;

    virtual void                         addId(const XMLCh * const ) = 0;

    virtual void                         addIdRef(const XMLCh * const ) = 0;

    virtual void                         toCheckIdRefList(bool) = 0;

    /**
      * EntityDeclPool
      *
      */
    virtual const NameIdPool<DTDEntityDecl>* getEntityDeclPool() const = 0;

    virtual const NameIdPool<DTDEntityDecl>* setEntityDeclPool(const NameIdPool<DTDEntityDecl>* const) = 0;    
           
    virtual void                             checkEntity(const XMLCh * const ) const = 0 ;

    /**
      * Union datatype handling
      *
      */

    virtual DatatypeValidator * getValidatingMemberType() const = 0 ;
    virtual void setValidatingMemberType(DatatypeValidator * validatingMemberType) = 0 ;

    //@}

   
protected :
    // -----------------------------------------------------------------------
    /**  Hidden Constructors */
    // -----------------------------------------------------------------------
    //@{
    ValidationContext(MemoryManager* const memMgr = XMLPlatformUtils::fgMemoryManager)
    :fMemoryManager(memMgr)
    {
    };
    //@}

    // -----------------------------------------------------------------------
    //  Data members
    //
    //  fMemoryManager
    //      Pluggable memory manager for dynamic allocation/deallocation.
    // -----------------------------------------------------------------------
    MemoryManager*                    fMemoryManager;

private :
    // -----------------------------------------------------------------------
    /** name  Unimplemented copy constructor and operator= */
    // -----------------------------------------------------------------------
    //@{
    ValidationContext(const ValidationContext& );
    ValidationContext& operator=(const ValidationContext& );
    //@}

};

XERCES_CPP_NAMESPACE_END

#endif

