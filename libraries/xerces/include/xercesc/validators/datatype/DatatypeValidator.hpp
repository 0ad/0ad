/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2001-2003 The Apache Software Foundation.  All rights
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
 * $Id: DatatypeValidator.hpp,v 1.24 2004/01/29 11:51:22 cargilld Exp $
 */

#if !defined(DATATYPEVALIDATOR_HPP)
#define DATATYPEVALIDATOR_HPP

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/RefHashTableOf.hpp>
#include <xercesc/util/KVStringPair.hpp>
#include <xercesc/util/XMLUniDefs.hpp>
#include <xercesc/util/regx/RegularExpression.hpp>
#include <xercesc/validators/schema/SchemaSymbols.hpp>
#include <xercesc/internal/XSerializable.hpp>
#include <xercesc/framework/psvi/XSSimpleTypeDefinition.hpp>
#include <xercesc/framework/ValidationContext.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class MemoryManager;

/**
  * DataTypeValidator defines the interface that data type validators must
  * obey. These validators can be supplied by the application writer and may
  * be useful as standalone code as well as plugins to the validator
  * architecture.
  *
  * Notice:
  * The datatype validator will own the facets hashtable passed to it during
  * construction, which means that the datatype validator will be responsible
  * for the deletion. The facets hashtable will be created during parsing and
  * passed to the appropriate datatype validator which in turn will delete it
  * upon its destruction.
  *
  */


class VALIDATORS_EXPORT DatatypeValidator : public XSerializable, public XMemory
{
public:
    // -----------------------------------------------------------------------
    // Constant data
    // -----------------------------------------------------------------------
	//facets
	enum {
        FACET_LENGTH         = 1,
        FACET_MINLENGTH      = 1<<1,
        FACET_MAXLENGTH      = 1<<2,
        FACET_PATTERN        = 1<<3,
        FACET_ENUMERATION    = 1<<4,
        FACET_MAXINCLUSIVE   = 1<<5,
        FACET_MAXEXCLUSIVE   = 1<<6,
        FACET_MININCLUSIVE   = 1<<7,
        FACET_MINEXCLUSIVE   = 1<<8,
        FACET_TOTALDIGITS    = 1<<9,
        FACET_FRACTIONDIGITS = 1<<10,
        FACET_ENCODING       = 1<<11,
        FACET_DURATION       = 1<<12,
        FACET_PERIOD         = 1<<13,
        FACET_WHITESPACE     = 1<<14
    };

    //2.4.2.6 whiteSpace - Datatypes
	enum {
        PRESERVE = 0,
        REPLACE  = 1,
        COLLAPSE = 2
    };

    enum ValidatorType {
        String,
        AnyURI,
        QName,
        Name,
        NCName,
        Boolean,
        Float,
        Double,
        Decimal,
        HexBinary,
        Base64Binary,
        Duration,
        DateTime,
        Date,
        Time,
        MonthDay,
        YearMonth,
        Year,
        Month,
        Day,
        ID,
        IDREF,
        ENTITY,
        NOTATION,
        List,
        Union,
        AnySimpleType,
        UnKnown
    };

    // -----------------------------------------------------------------------
    //  Public Destructor
    // -----------------------------------------------------------------------
	/** @name Destructor. */
    //@{

    virtual ~DatatypeValidator();

	//@}

    // -----------------------------------------------------------------------
    // Getter methods
    // -----------------------------------------------------------------------
    /** @name Getter Functions */
    //@{

    /**
      * Returns the final values of the simpleType
      */
    int getFinalSet() const;

    /**
      * Returns the datatype facet if any is set.
      */
	RefHashTableOf<KVStringPair>* getFacets() const;

    /**
      * Returns default value (collapse) for whiteSpace facet.
      * This function is overwritten in StringDatatypeValidator.
      */
    short getWSFacet () const;

    /**
      * Returns the base datatype validator if set.
      */
    DatatypeValidator* getBaseValidator() const;

    /**
      * Returns the 'class' type of datatype validator
      */
    ValidatorType getType() const;

    /**
      * Returns whether the type is atomic or not
      *
      * To be redefined in List/Union validators
      */
    virtual bool isAtomic() const;

    /**
      * Returns the datatype enumeration if any is set.
	  * Derived class shall provide their own copy.
      */
	virtual const RefArrayVectorOf<XMLCh>* getEnumString() const = 0;

    /**
     * returns true if this type is anonymous
     **/
    bool getAnonymous() const;

    /**
     * sets this type to be anonymous
     **/
    void setAnonymous();

    /**
     *  Fundamental Facet: ordered 
     */
    XSSimpleTypeDefinition::ORDERING getOrdered() const;

    /**
     * Fundamental Facet: cardinality. 
     */
    bool getFinite() const;

    /**
     * Fundamental Facet: bounded. 
     */
    bool getBounded() const;

    /**
     * Fundamental Facet: numeric. 
     */
    bool getNumeric() const;

    /**
     *    Canonical Representation
     *
     *    Derivative datatype may overwrite this method once
     *    it has its own canonical representation other than
     *    the default one.
     *
     * @param rawData:    data in raw string
     * @param memMgr:     memory manager
     * @param toValiate:  to validate the raw string or not
     *
     * @return: canonical representation of the data
     * 
     * Note:  
     *
     *    1. the return value is kept in memory allocated
     *       by the memory manager passed in or by dv's
     *       if no memory manager is provided.
     *
     *    2. client application is responsible for the 
     *       proper deallcation of the memory allocated
     *       for the returned value.
     *
     *    3. In the case where the rawData is not valid
     *       with regards to the fundamental datatype,
     *       a null string is returned.
     *
     */
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
	   * Checks that the "content" string is valid datatype.
       * If invalid, a Datatype validation exception is thrown.
	   *
	   * @param  content   A string containing the content to be validated
	   *
	   */
	virtual void validate
                 (
                  const XMLCh*             const content
                ,       ValidationContext* const context = 0
                ,       MemoryManager*     const manager = XMLPlatformUtils::fgMemoryManager
                  ) = 0;

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
      * Compares content in the Domain value vs. lexical value.
      *
      * e.g. If type is a float then 1.0 may be equivalent to 1 even though
      * both are lexically different.
      *
      * @param  value1    string to compare
      *
      * @param  value2    string to compare
      *
      * We will provide a default behavior that should be redefined at the
      * children level, if necessary (i.e. boolean case).
      */
    virtual int compare(const XMLCh* const value1, const XMLCh* const value2
        ,       MemoryManager*     const manager = XMLPlatformUtils::fgMemoryManager
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
    ) = 0;

    /**
     * Returns the uri,name of the type this validator is for
     */
    const XMLCh* getTypeName() const;

    /**
     * sets the uri,name that this  validator is for - typeName is uri,name string.
     * due to the internals of xerces this will set the uri to be the schema uri if
     * there is no comma in typeName
     */
    void setTypeName(const XMLCh* const typeName);

    /**
     * sets the uri,name that this  validator is for
     */
    void setTypeName(const XMLCh* const name, const XMLCh* const uri);

    /**
     * Returns the uri of the type this validator is for
     */
    const XMLCh* getTypeUri() const;

    /**
     * Returns the name of the type this validator is for
     */
    const XMLCh* getTypeLocalName() const;

    /**
     * Returns the plugged-in memory manager
     */
    MemoryManager* getMemoryManager() const;

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(DatatypeValidator)

    /***
      *
      *  Serialzie DatatypeValidator derivative 
      *
      *  Param
      *     serEng: serialize engine
      *     dv:     DatatypeValidator derivative
      *
      *  Return:
      *
      ***/
	static void storeDV(XSerializeEngine&        serEng
                      , DatatypeValidator* const dv);

    /***
      *
      *  Create a DatatypeValidator derivative from the binary
      *  stream.
      *
      *  Param
      *     serEng: serialize engine
      *
      *  Return:
      *     DatatypeValidator derivative 
      *
      ***/
	static DatatypeValidator* loadDV(XSerializeEngine& serEng);

protected:
    // -----------------------------------------------------------------------
    //  Protected Constructors
    // -----------------------------------------------------------------------
    /** @name Constructors */
    //@{

    /**
      *
      * @param  baseValidator  The base datatype validator for derived
      *                        validators. Null if native validator.
      *
      * @param  facets         A hashtable of datatype facets (except enum).
      *
      * @param  finalSet       'final' value of the simpleType
      */

	DatatypeValidator(DatatypeValidator* const baseValidator,
                      RefHashTableOf<KVStringPair>* const facets,
                      const int finalSet,
                      const ValidatorType type,
                      MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    //@}


	friend class DatatypeValidatorFactory;
    friend class XSObjectFactory;

    /**
      * facetDefined
	  */
	int   getFacetsDefined() const;
    void  setFacetsDefined(int);

    /**
      * fixed
	  */
	int   getFixed() const;
    void  setFixed(int);


    /**
      * fPattern
	  */
    const XMLCh* getPattern() const;
	void         setPattern(const XMLCh* );

    /**
      * fRegex
	  */
	RegularExpression* getRegex() const;
	void               setRegex(RegularExpression* const);

    /**
      * set fType
      */
    void setType(ValidatorType);

    /**
      * set fWhiteSpace
      */
    void setWhiteSpace(short);

    /**
      * get WSString
      */
    const XMLCh*   getWSstring(const short WSType) const;

    /**
     *  Fundamental Facet: ordered 
     */
    void setOrdered(XSSimpleTypeDefinition::ORDERING ordered);

    /**
     * Fundamental Facet: cardinality. 
     */
    void setFinite(bool finite);

    /**
     * Fundamental Facet: bounded. 
     */
    void setBounded(bool bounded);

    /**
     * Fundamental Facet: numeric. 
     */
    void setNumeric(bool numeric);

    // -----------------------------------------------------------------------
    //  Protected data members
    //
    //  fMemoryManager
    //      Pluggable memory manager for dynamic allocation/deallocation.
    // -----------------------------------------------------------------------
    MemoryManager*                fMemoryManager;

private:
    // -----------------------------------------------------------------------
    //  CleanUp methods
    // -----------------------------------------------------------------------
    void cleanUp();

    inline bool isBuiltInDV(DatatypeValidator* const);

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    DatatypeValidator(const DatatypeValidator&);
    DatatypeValidator& operator=(const DatatypeValidator&);

    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fFinalSet
    //      stores "final" values of simpleTypes
    //
    //  fBaseValidator
    //      This is a pointer to a base datatype validator. If value is null,
	//      it means we have a native datatype validator not a derived one.
	//		
    //  fFacets
    //      This is a hashtable of dataype facets.
    //
    //  fType
    //      Stores the class type of datatype validator
    //
    //  fFacetsDefined
    //      Stores the constaiting facets flag
    //
    //  fPattern
    //      the pointer to the String of the pattern. The actual data is
    //      in the Facets.
    //
    //  fRegex
    //      pointer to the RegularExpress object
    //
    //
    //  fFixed
    //      if {fixed} is true, then types for which this type is the
    //      {base type definition} cannot specify a value for a specific
    //      facet.
    //
    //  fTypeName
    //      the uri,name of the type this validator will validate
    //
    //  fTypeLocalName
    //      the name of the type this validator will validate
    //
    //  fTypeUri
    //      the uri of the type this validator will validate
    //  fAnonymous
    //      true if this type is anonynous
    //
    // -----------------------------------------------------------------------
    bool                                fAnonymous;
    short                               fWhiteSpace;
    int                                 fFinalSet;
    int                                 fFacetsDefined;
    int                                 fFixed;
    ValidatorType                       fType;
	DatatypeValidator*                  fBaseValidator;
	RefHashTableOf<KVStringPair>*       fFacets;
    XMLCh*                              fPattern;
    RegularExpression*                  fRegex;
    XMLCh*                              fTypeName;
    const XMLCh*                        fTypeLocalName;
    const XMLCh*                        fTypeUri;
    XSSimpleTypeDefinition::ORDERING    fOrdered;
    bool                                fFinite;
    bool                                fBounded;
    bool                                fNumeric;
};


// ---------------------------------------------------------------------------
//  DatatypeValidator: Getters
// ---------------------------------------------------------------------------
inline int DatatypeValidator::getFinalSet() const {

    return fFinalSet;
}

inline RefHashTableOf<KVStringPair>* DatatypeValidator::getFacets() const {

    return fFacets;
}

inline DatatypeValidator* DatatypeValidator::getBaseValidator() const {

	return fBaseValidator;
}

inline short DatatypeValidator::getWSFacet() const {

    return fWhiteSpace;
}

inline DatatypeValidator::ValidatorType DatatypeValidator::getType() const
{
    return fType;
}

inline int DatatypeValidator::getFacetsDefined() const
{
    return fFacetsDefined;
}

inline int DatatypeValidator::getFixed() const
{
    return fFixed;
}

inline const XMLCh* DatatypeValidator::getPattern() const
{
    return fPattern;
}

inline RegularExpression* DatatypeValidator::getRegex() const
{
    return fRegex;
}

inline const XMLCh* DatatypeValidator::getTypeName() const
{
    return fTypeName;
}

inline bool DatatypeValidator::getAnonymous() const
{
    return fAnonymous;
}

inline const XMLCh* DatatypeValidator::getTypeLocalName() const
{
    return fTypeLocalName;
}

inline const XMLCh* DatatypeValidator::getTypeUri() const
{
    return fTypeUri;
}

inline MemoryManager* DatatypeValidator::getMemoryManager() const
{
    return fMemoryManager;
}

inline XSSimpleTypeDefinition::ORDERING DatatypeValidator::getOrdered() const
{
    return fOrdered;
}

inline bool DatatypeValidator::getFinite() const
{
    return fFinite;
}
 
inline bool DatatypeValidator::getBounded() const
{
    return fBounded;
}

inline bool DatatypeValidator::getNumeric() const
{
    return fNumeric;
}

// ---------------------------------------------------------------------------
//  DatatypeValidator: Setters
// ---------------------------------------------------------------------------
inline void DatatypeValidator::setType(ValidatorType theType)
{
    fType = theType;
}

inline void DatatypeValidator::setWhiteSpace(short newValue)
{
    fWhiteSpace = newValue;
}

inline void DatatypeValidator::setFacetsDefined(int facets)
{
    fFacetsDefined |= facets;
}

inline void DatatypeValidator::setFixed(int fixed)
{
    fFixed |= fixed;
}

inline void DatatypeValidator::setPattern(const XMLCh* pattern)
{
    if (fPattern)
        fMemoryManager->deallocate(fPattern);//delete [] fPattern;
    fPattern = XMLString::replicate(pattern, fMemoryManager);
}

inline void DatatypeValidator::setRegex(RegularExpression* const regex)
{
    fRegex = regex;
}

inline bool DatatypeValidator::isAtomic() const {

    return true;
}

inline void DatatypeValidator::setAnonymous() {
    fAnonymous = true;
}

inline void DatatypeValidator::setOrdered(XSSimpleTypeDefinition::ORDERING ordered)
{
    fOrdered = ordered;
}

inline void DatatypeValidator::setFinite(bool finite)
{
    fFinite = finite;
}

inline void DatatypeValidator::setBounded(bool bounded)
{
    fBounded = bounded;
}

inline void DatatypeValidator::setNumeric(bool numeric)
{
    fNumeric = numeric;
}

// ---------------------------------------------------------------------------
//  DatatypeValidators: Compare methods
// ---------------------------------------------------------------------------
inline int DatatypeValidator::compare(const XMLCh* const lValue,
                                      const XMLCh* const rValue
                                      , MemoryManager*     const)
{
    return XMLString::compareString(lValue, rValue);
}

// ---------------------------------------------------------------------------
//  DatatypeValidators: Validation methods
// ---------------------------------------------------------------------------
inline bool
DatatypeValidator::isSubstitutableBy(const DatatypeValidator* const toCheck)
{
    const DatatypeValidator* dv = toCheck;

	while (dv != 0) {

        if (dv == this) {
            return true;
        }

        dv = dv->getBaseValidator();
    }

    return false;
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file DatatypeValidator.hpp
  */

