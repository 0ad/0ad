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
 * $Id: AbstractNumericFacetValidator.hpp,v 1.11 2004/01/29 11:51:22 cargilld Exp $
 * $Log: AbstractNumericFacetValidator.hpp,v $
 * Revision 1.11  2004/01/29 11:51:22  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.10  2003/12/17 00:18:38  cargilld
 * Update to memory management so that the static memory manager (one used to call Initialize) is only for static data.
 *
 * Revision 1.9  2003/11/12 20:32:03  peiyongz
 * Statless Grammar: ValidationContext
 *
 * Revision 1.8  2003/10/17 21:13:24  peiyongz
 * loadNumber() moved to XMLNumber
 *
 * Revision 1.7  2003/10/02 19:21:06  peiyongz
 * Implementation of Serialization/Deserialization
 *
 * Revision 1.6  2003/05/15 18:53:26  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.5  2002/12/18 14:17:55  gareth
 * Fix to bug #13438. When you eant a vector that calls delete[] on its members you should use RefArrayVectorOf.
 *
 * Revision 1.4  2002/11/04 14:53:27  tng
 * C++ Namespace Support.
 *
 * Revision 1.3  2002/10/18 16:52:14  peiyongz
 * Patch to Bug#13640: Getter methods not public in
 *                                    DecimalDatatypeValidator
 *
 * Revision 1.2  2002/02/14 15:17:31  peiyongz
 * getEnumString()
 *
 * Revision 1.1.1.1  2002/02/01 22:22:39  peiyongz
 * sane_include
 *
 * Revision 1.3  2001/11/22 20:23:20  peiyongz
 * _declspec(dllimport) and inline warning C4273
 *
 * Revision 1.2  2001/11/12 20:37:57  peiyongz
 * SchemaDateTimeException defined
 *
 * Revision 1.1  2001/10/01 16:13:56  peiyongz
 * DTV Reorganization:new classes: AbstractNumericFactValidator/ AbstractNumericValidator
 *
 */

#if !defined(ABSTRACT_NUMERIC_FACET_VALIDATOR_HPP)
#define ABSTRACT_NUMERIC_FACET_VALIDATOR_HPP

#include <xercesc/validators/datatype/DatatypeValidator.hpp>
#include <xercesc/util/RefArrayVectorOf.hpp>
#include <xercesc/util/XMLNumber.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class VALIDATORS_EXPORT AbstractNumericFacetValidator : public DatatypeValidator
{
public:

    // -----------------------------------------------------------------------
    //  Public ctor/dtor
    // -----------------------------------------------------------------------
	/** @name Constructor. */
    //@{

    virtual ~AbstractNumericFacetValidator();

	//@}

	virtual const RefArrayVectorOf<XMLCh>* getEnumString() const;

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(AbstractNumericFacetValidator)

protected:

    AbstractNumericFacetValidator
    (
        DatatypeValidator* const baseValidator
        , RefHashTableOf<KVStringPair>* const facets
        , const int finalSet
        , const ValidatorType type
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

    void init(RefArrayVectorOf<XMLCh>*  const enums
        , MemoryManager* const manager);

    //
    // Abstract interface
    //
    virtual void assignAdditionalFacet(const XMLCh* const key
                                     , const XMLCh* const value
                                     , MemoryManager* const manager);

    virtual void inheritAdditionalFacet();

    virtual void checkAdditionalFacetConstraints(MemoryManager* const manager) const;

    virtual void checkAdditionalFacetConstraintsBase(MemoryManager* const manager) const;

    virtual int  compareValues(const XMLNumber* const lValue
                             , const XMLNumber* const rValue) = 0;

    virtual void checkContent(const XMLCh*             const content
                            ,       ValidationContext* const context
                            , bool                           asBase
                            ,       MemoryManager*     const manager) = 0;

// -----------------------------------------------------------------------
// Setter methods
// -----------------------------------------------------------------------

    virtual void  setMaxInclusive(const XMLCh* const) = 0;

    virtual void  setMaxExclusive(const XMLCh* const) = 0;

    virtual void  setMinInclusive(const XMLCh* const) = 0;

    virtual void  setMinExclusive(const XMLCh* const) = 0;

    virtual void  setEnumeration(MemoryManager* const manager) = 0;

    static const int INDETERMINATE;

public:
// -----------------------------------------------------------------------
// Getter methods
// -----------------------------------------------------------------------

    inline XMLNumber* const            getMaxInclusive() const;

    inline XMLNumber* const            getMaxExclusive() const;

    inline XMLNumber* const            getMinInclusive() const;

    inline XMLNumber* const            getMinExclusive() const;

    inline RefVectorOf<XMLNumber>*     getEnumeration() const;

protected:
    // -----------------------------------------------------------------------
    //  Protected data members
    //
    //      Allow access to derived class
    //
    // -----------------------------------------------------------------------
    bool                     fMaxInclusiveInherited;
    bool                     fMaxExclusiveInherited;
    bool                     fMinInclusiveInherited;
    bool                     fMinExclusiveInherited;
    bool                     fEnumerationInherited;

    XMLNumber*               fMaxInclusive;
    XMLNumber*               fMaxExclusive;
    XMLNumber*               fMinInclusive;
    XMLNumber*               fMinExclusive;

    RefVectorOf<XMLNumber>*  fEnumeration;    // save the actual value
    RefArrayVectorOf<XMLCh>*      fStrEnumeration;

private:

    void assignFacet(MemoryManager* const manager);

    void inspectFacet(MemoryManager* const manager);

    void inspectFacetBase(MemoryManager* const manager);

    void inheritFacet();

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    AbstractNumericFacetValidator(const AbstractNumericFacetValidator&);
    AbstractNumericFacetValidator& operator=(const AbstractNumericFacetValidator&);
};

// -----------------------------------------------------------------------
// Getter methods
// -----------------------------------------------------------------------

inline XMLNumber* const AbstractNumericFacetValidator::getMaxInclusive() const
{
    return fMaxInclusive;
}

inline XMLNumber* const AbstractNumericFacetValidator::getMaxExclusive() const
{
    return fMaxExclusive;
}

inline XMLNumber* const AbstractNumericFacetValidator::getMinInclusive() const
{
    return fMinInclusive;
}

inline XMLNumber* const AbstractNumericFacetValidator::getMinExclusive() const
{
    return fMinExclusive;
}

inline RefVectorOf<XMLNumber>* AbstractNumericFacetValidator::getEnumeration() const
{
    return fEnumeration;
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file AbstractNumericFacetValidator.hpp
  */
