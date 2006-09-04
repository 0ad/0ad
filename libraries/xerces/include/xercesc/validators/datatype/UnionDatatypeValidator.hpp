/*
 * Copyright 2001,2004 The Apache Software Foundation.
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
 * $Id: UnionDatatypeValidator.hpp 191054 2005-06-17 02:56:35Z jberry $
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
     * @deprecated
     **/
    const XMLCh* getMemberTypeName() const;

    /**
     * Returns the type uri that was actually used to validate the last time validate was called
     * note - this does not mean that it fully validated sucessfully
     * @deprecated
     **/
    const XMLCh* getMemberTypeUri() const;

    /**
     * Returns true if the type that was actually used to validate the last time validate was called 
     * is anonymous
     * @deprecated
     */
    bool getMemberTypeAnonymous() const;


    /**
     * Returns the member DatatypeValidator used to validate the content the last time validate 
     * was called
     * @deprecated
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

