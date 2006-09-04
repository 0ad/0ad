/*
 * Copyright 1999-2004 The Apache Software Foundation.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * $Id: ValidationContextImpl.hpp 191054 2005-06-17 02:56:35Z jberry $
 */

#if !defined(VALIDATION_CONTEXTIMPL_HPP)
#define VALIDATION_CONTEXTIMPL_HPP

#include <xercesc/framework/ValidationContext.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLPARSER_EXPORT ValidationContextImpl : public ValidationContext
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
    virtual ~ValidationContextImpl();

    ValidationContextImpl(MemoryManager* const memMgr = XMLPlatformUtils::fgMemoryManager);

    //@}

    // -----------------------------------------------------------------------
    /** @name The ValidationContextImpl Interface */
    // -----------------------------------------------------------------------
    //@{

    /**
      * IDRefList
      *
      */
    virtual RefHashTableOf<XMLRefInfo>*  getIdRefList() const;

    virtual void                         setIdRefList(RefHashTableOf<XMLRefInfo>* const);

    virtual void                         clearIdRefList();

    virtual void                         addId(const XMLCh * const );

    virtual void                         addIdRef(const XMLCh * const );

    virtual void                         toCheckIdRefList(bool);

    /**
      * EntityDeclPool
      *
      */
    virtual const NameIdPool<DTDEntityDecl>* getEntityDeclPool() const;

    virtual const NameIdPool<DTDEntityDecl>* setEntityDeclPool(const NameIdPool<DTDEntityDecl>* const);    
           
    virtual void                             checkEntity(const XMLCh * const ) const;


    /**
      * Union datatype handling
      *
      */

    virtual DatatypeValidator * getValidatingMemberType() const;
    virtual void setValidatingMemberType(DatatypeValidator * validatingMemberType) ;

    //@}
  
private:
    // -----------------------------------------------------------------------
    /** name  Unimplemented copy constructor and operator= */
    // -----------------------------------------------------------------------
    //@{
    ValidationContextImpl(const ValidationContextImpl& );
    ValidationContextImpl& operator=(const ValidationContextImpl& );
    //@}

    // -----------------------------------------------------------------------
    //  Data members
    //
    //  fIDRefList:  owned/adopted
    //      This is a list of XMLRefInfo objects. This member lets us do all
    //      needed ID-IDREF balancing checks.
    //
    //  fEntityDeclPool: referenced only
    //      This is a pool of EntityDecl objects, which contains all of the
    //      general entities that are declared in the DTD subsets, plus the
    //      default entities (such as &gt; &lt; ...) defined by the XML Standard.
    //
    //  fToAddToList
    //  fValidatingMemberType
    //      The member type in a union that actually
    //      validated some text.  Note that the validationContext does not
    //      own this object, and the value of getValidatingMemberType
    //      will not be accurate unless the type of the most recently-validated
    //      element/attribute is in fact a union datatype.
    // -----------------------------------------------------------------------

    RefHashTableOf<XMLRefInfo>*         fIdRefList;
    const NameIdPool<DTDEntityDecl>*    fEntityDeclPool;
    bool                                fToCheckIdRefList;
    DatatypeValidator *                 fValidatingMemberType;

};



inline DatatypeValidator * ValidationContextImpl::getValidatingMemberType() const
{
    return fValidatingMemberType;
}

inline void ValidationContextImpl::setValidatingMemberType(DatatypeValidator * validatingMemberType) 
{
    fValidatingMemberType = validatingMemberType;
}

XERCES_CPP_NAMESPACE_END

#endif

