/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2001 The Apache Software Foundation.  All rights
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
 * $Id: UnionDatatypeValidator.hpp,v 1.17 2004/01/29 11:51:22 cargilld Exp $
 * $Log: UnionDatatypeValidator.hpp,v $
 * Revision 1.17  2004/01/29 11:51:22  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.16  2003/12/23 21:50:36  peiyongz
 * Absorb exception thrown in getCanonicalRepresentation and return 0,
 * only validate when required
 *
 * Revision 1.15  2003/12/17 00:18:39  cargilld
 * Update to memory management so that the static memory manager (one used to call Initialize) is only for static data.
 *
 * Revision 1.14  2003/11/28 18:53:07  peiyongz
 * Support for getCanonicalRepresentation
 *
 * Revision 1.13  2003/11/24 05:10:26  neilg
 * implement method for determining member type of union that validated some value
 *
 * Revision 1.12  2003/11/12 20:32:03  peiyongz
 * Statless Grammar: ValidationContext
 *
 * Revision 1.11  2003/10/07 19:39:03  peiyongz
 * Implementation of Serialization/Deserialization
 *
 * Revision 1.10  2003/08/16 18:42:49  neilg
 * fix for bug 22457.  Union types that are restrictions of other union types were previously considered not to inherit their parents member types.  This is at variance with the behaviour of the Java parser and apparently with the spec, so I have changed this.
 *
 * Revision 1.9  2003/05/15 18:53:27  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.8  2003/02/06 13:51:55  gareth
 * fixed bug with multiple attributes being validated by the same union type.
 *
 * Revision 1.7  2003/01/29 19:55:19  gareth
 * updated to deal with null pointer issue with fValidatedDatatype.
 *
 * Revision 1.6  2003/01/29 19:53:35  gareth
 * we now store information about which validator was used to validate.
 *
 * Revision 1.5  2003/01/10 16:48:47  tng
 * [Bug 14912] crashes inside UnionDatatypeValidator::isSubstitutableBy.   Patch from Alberto Massari.
 *
 * Revision 1.4  2002/12/18 14:17:55  gareth
 * Fix to bug #13438. When you eant a vector that calls delete[] on its members you should use RefArrayVectorOf.
 *
 * Revision 1.3  2002/11/04 14:53:28  tng
 * C++ Namespace Support.
 *
 * Revision 1.2  2002/02/14 15:17:31  peiyongz
 * getEnumString()
 *
 * Revision 1.1.1.1  2002/02/01 22:22:43  peiyongz
 * sane_include
 *
 * Revision 1.9  2001/12/13 16:48:29  peiyongz
 * Avoid dangling pointer
 *
 * Revision 1.8  2001/09/05 20:49:10  knoaman
 * Fix for complexTypes with mixed content model.
 *
 * Revision 1.7  2001/08/31 16:53:41  knoaman
 * Misc. fixes.
 *
 * Revision 1.6  2001/08/24 17:12:01  knoaman
 * Add support for anySimpleType.
 * Remove parameter 'baseValidator' from the virtual method 'newInstance'.
 *
 * Revision 1.5  2001/08/21 20:05:41  peiyongz
 * put back changes introduced in 1.3
 *
 * Revision 1.3  2001/08/16 14:41:38  knoaman
 * implementation of virtual methods.
 *
 * Revision 1.2  2001/07/24 21:23:40  tng
 * Schema: Use DatatypeValidator for ID/IDREF/ENTITY/ENTITIES/NOTATION.
 *
 * Revision 1.1  2001/07/13 14:10:40  peiyongz
 * UnionDTV
 *
 */

#if !defined(UNION_DATATYPEVALIDATOR_HPP)
#define UNION_DATATYPEVALIDATOR_HPP

#include <xercesc/validators/datatype/DatatypeValidator.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class VALIDATORS_EXPORT UnionDatatypeValidator : public DatatypeValidator
{
public:

    // -----------------------------------------------------------------------
    //  Public ctor/dtor
    // -----------------------------------------------------------------------
	/** @name Constructors and Destructor. */
    //@{

    UnionDatatypeValidator
    (
        MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

    //
    // constructor for native Union datatype validator
    // <simpleType name="nativeUnion">
    //      <union   memberTypes="member1 member2 ...">
    // </simpleType>
    //
    UnionDatatypeValidator
    (
        RefVectorOf<DatatypeValidator>* const memberTypeValidators
        , const int finalSet
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

    //
    // constructor for derived Union datatype validator
    // <simpleType name="derivedUnion">
    //      <restriction base="nativeUnion">
    //          <pattern     value="patter_value"/>
    //          <enumeration value="enum_value"/>
    //      </restriction>
    // </simpleType>
    //
    UnionDatatypeValidator
    (
        DatatypeValidator* const baseValidator
        , RefHashTableOf<KVStringPair>* const facets
        , RefArrayVectorOf<XMLCh>* const enums
        , const int finalSet
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
        , RefVectorOf<DatatypeValidator>* const memberTypeValidators = 0
        , const bool memberTypesInherited = true
    );

    virtual ~UnionDatatypeValidator();

	//@}

	virtual const RefArrayVectorOf<XMLCh>* getEnumString() const;

    // -----------------------------------------------------------------------
    // Getter methods
    // -----------------------------------------------------------------------
    /** @name Getter Functions */
    //@{
    /**
      * Returns whether the type is atomic or not
      */
    virtual bool isAtomic() const;

    virtual const XMLCh* getCanonicalRepresentation
                        (
                          const XMLCh*         const rawData
                        ,       MemoryManager* const memMgr = 0
                        ,       bool                 toValidate = false
                        ) const;

    //@}

    // -----------------------------------------------------------------------
    // Validation methods
    // -----------------------------------------------------------------------
    /** @name Validation Function */
    //@{

    /**
     * validate that a string matches the boolean datatype
     * @param content A string containing the content to be validated
     *
     * @exception throws InvalidDatatypeException if the content is
     * is not valid.
     */

	virtual void validate
                 (
                  const XMLCh*             const content
                ,       ValidationContext* const context = 0
                ,       MemoryManager*     const manager = XMLPlatformUtils::fgMemoryManager
                  );

    /**
      * Checks whether a given type can be used as a substitute
      *
      * @param  toCheck    A datatype validator of the type to be used as a
      *                    substitute
      *
      * To be redefined in UnionDatatypeValidator
      */

    virtual bool isSubstitutableBy(const DatatypeValidator* const toCheck);

    //@}

    // -----------------------------------------------------------------------
    // Compare methods
    // -----------------------------------------------------------------------
    /** @name Compare Function */
    //@{

    /**
     * Compare two boolean data types
     *
     * @param content1
     * @param content2
     * @return
     */
    int compare(const XMLCh* const, const XMLCh* const
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
        );

    //@}

    /**
      * Returns an instance of the base datatype validator class
	  * Used by the DatatypeValidatorFactory.
      */
    virtual DatatypeValidator* newInstance
    (
        RefHashTableOf<KVStringPair>* const facets
        , RefArrayVectorOf<XMLCh>* const enums
        , const int finalSet
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(UnionDatatypeValidator)


    RefVectorOf<DatatypeValidator>* getMemberTypeValidators() const;


    /**
     * Returns the type name that was actually used to validate the last time validate was called
     * note - this does not mean that it fully validated sucessfully
     **/
    const XMLCh* getMemberTypeName() const;

    /**
     * Returns the type uri that was actually used to validate the last time validate was called
     * note - this does not mean that it fully validated sucessfully
     **/
    const XMLCh* getMemberTypeUri() const;

    /**
     * Returns true if the type that was actually used to validate the last time validate was called 
     * is anonymous
     */
    bool getMemberTypeAnonymous() const;


    /**
     * Returns the member DatatypeValidator used to validate the content the last time validate 
     * was called
     */
    const DatatypeValidator* getMemberTypeValidator() const;

    /**
     * Called inbetween uses of this validator to reset PSVI information
     */
    void reset();

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------    
    UnionDatatypeValidator(const UnionDatatypeValidator&);
    UnionDatatypeValidator& operator=(const UnionDatatypeValidator&);

    virtual void checkContent(const XMLCh*             const content
                            ,       ValidationContext* const context
                            , bool                           asBase
                            ,       MemoryManager*     const manager);

    void init(DatatypeValidator*            const baseValidator
            , RefHashTableOf<KVStringPair>* const facets
            , RefArrayVectorOf<XMLCh>*      const enums
            , MemoryManager*                const manager);

    void cleanUp();
    
    RefArrayVectorOf<XMLCh>*  getEnumeration() const;

    void                 setEnumeration(RefArrayVectorOf<XMLCh>*, bool);


    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fEnumeration
    //      we own it (or not, depending on state of fEnumerationInherited).
    //
    //  fMemberTypeValidators
    //      we own it (or not, depending on the state of fMemberTypesInherited).
    //
    //  fValidatedDatatype
    //      the dataTypeValidator  that was actually used to validate the last time validate was called
    //
    // -----------------------------------------------------------------------

     bool                             fEnumerationInherited;
     bool                             fMemberTypesInherited;
     RefArrayVectorOf<XMLCh>*         fEnumeration;
     RefVectorOf<DatatypeValidator>*  fMemberTypeValidators;
     DatatypeValidator*               fValidatedDatatype;
};

inline DatatypeValidator* UnionDatatypeValidator::newInstance
(
      RefHashTableOf<KVStringPair>* const facets
    , RefArrayVectorOf<XMLCh>* const      enums
    , const int                           finalSet
    , MemoryManager* const                manager
)
{
    return (DatatypeValidator*) new (manager) UnionDatatypeValidator(this, facets, enums, finalSet, manager, fMemberTypeValidators, true);
}

inline void UnionDatatypeValidator::validate( const XMLCh*             const content
                                           ,        ValidationContext* const context
                                           ,        MemoryManager*     const manager)
{
    checkContent(content, context, false, manager);
}

inline void UnionDatatypeValidator::cleanUp()
{
    //~RefVectorOf will delete all adopted elements
    if ( !fEnumerationInherited && fEnumeration)
        delete fEnumeration;

    if (!fMemberTypesInherited && fMemberTypeValidators)
        delete fMemberTypeValidators;
    
}

inline RefArrayVectorOf<XMLCh>* UnionDatatypeValidator:: getEnumeration() const
{
    return fEnumeration;
}

inline void UnionDatatypeValidator::setEnumeration(RefArrayVectorOf<XMLCh>* enums
                                                 , bool                inherited)
{
    if (enums)
    {
        if (  !fEnumerationInherited && fEnumeration)
            delete fEnumeration;

        fEnumeration = enums;
        fEnumerationInherited = inherited;
        setFacetsDefined(DatatypeValidator::FACET_ENUMERATION);
    }
}

//
// get the native UnionDTV's fMemberTypeValidators
//
inline
RefVectorOf<DatatypeValidator>* UnionDatatypeValidator::getMemberTypeValidators() const
{
    return this->fMemberTypeValidators;
}

inline bool UnionDatatypeValidator::isAtomic() const {



    if (!fMemberTypeValidators) {
        return false;
    }

    unsigned int memberSize = fMemberTypeValidators->size();

    for (unsigned int i=0; i < memberSize; i++) {
        if (!fMemberTypeValidators->elementAt(i)->isAtomic()) {
            return false;
        }
    }

    return true;
}

inline bool UnionDatatypeValidator::isSubstitutableBy(const DatatypeValidator* const toCheck) {

    if (toCheck == this) {
        return true;
    }

    if (fMemberTypeValidators) {
        unsigned int memberSize = fMemberTypeValidators->size();

        for (unsigned int i=0; i < memberSize; i++) {
            if (fMemberTypeValidators->elementAt(i)->isSubstitutableBy(toCheck)) {
                return true;
            }
        }
    }
    return false;
}

inline const XMLCh* UnionDatatypeValidator::getMemberTypeName() const {
    if(fValidatedDatatype) {
        return fValidatedDatatype->getTypeLocalName();
    }
    return 0;
}

inline const XMLCh* UnionDatatypeValidator::getMemberTypeUri() const 
{
    if(fValidatedDatatype) {
        return fValidatedDatatype->getTypeUri();
    }
    return 0;
}

inline bool UnionDatatypeValidator::getMemberTypeAnonymous() const {
    if(fValidatedDatatype) {
        return fValidatedDatatype->getAnonymous();
    }
    return 0;
}

inline const DatatypeValidator* UnionDatatypeValidator::getMemberTypeValidator() const {
    return fValidatedDatatype;
}

inline void UnionDatatypeValidator::reset() {
    fValidatedDatatype = 0;
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file UnionDatatypeValidator.hpp
  */

