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
 * $Id: DatatypeValidatorFactory.hpp,v 1.15 2004/01/29 11:51:22 cargilld Exp $
 */

#if !defined(DATATYPEVALIDATORFACTORY_HPP)
#define DATATYPEVALIDATORFACTORY_HPP

/**
 * This class implements a factory of Datatype Validators. Internally the
 * DatatypeValidators are kept in a registry.
 * There is one instance of DatatypeValidatorFactory per Parser.
 * There is one datatype Registry per instance of DatatypeValidatorFactory,
 * such registry is first allocated with the number DatatypeValidators needed.
 * e.g.
 * If Parser finds an XML document with a DTD, a registry of DTD validators (only
 * 9 validators) get initialized in the registry.
 * The initialization process consist of instantiating the Datatype and
 * facets and registering the Datatype into registry table.
 * This implementation uses a Hahtable as a registry. The datatype validators created
 * by the factory will be deleted by the registry.
 *
 * As the Parser parses an instance document it knows if validation needs
 * to be checked. If no validation is necesary we should not instantiate a
 * DatatypeValidatorFactory.
 * If validation is needed, we need to instantiate a DatatypeValidatorFactory.
 */

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/validators/datatype/DatatypeValidator.hpp>
#include <xercesc/validators/datatype/XMLCanRepGroup.hpp>
#include <xercesc/util/RefVectorOf.hpp>

#include <xercesc/internal/XSerializable.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  DatatypeValidatorFactory: Local declaration
// ---------------------------------------------------------------------------
typedef RefHashTableOf<KVStringPair> KVStringPairHashTable;
typedef RefHashTableOf<DatatypeValidator> DVHashTable;
typedef RefArrayVectorOf<XMLCh> XMLChRefVector;


class VALIDATORS_EXPORT DatatypeValidatorFactory : public XSerializable, public XMemory
{
public:

    // -----------------------------------------------------------------------
    //  Public Constructors and Destructor
    // -----------------------------------------------------------------------
    /** @name Constructors */
    //@{

    DatatypeValidatorFactory
    (
        MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

    //@}

    /** @name Destructor. */
    //@{

    ~DatatypeValidatorFactory();

    //@}

    // -----------------------------------------------------------------------
    // Getter methods
    // -----------------------------------------------------------------------
    /** @name Getter Functions */
    //@{

    /**
     * Returns the datatype validator
     *
     * @param  dvType   Datatype validator name/type
     */
    DatatypeValidator* getDatatypeValidator(const XMLCh* const dvType) const;

    /**
     * Returns the user defined registry of types
     **/
    DVHashTable* getUserDefinedRegistry() const;


    /**
     * Returns the built in  registry of types
     **/
    static DVHashTable* getBuiltInRegistry();

    //@}

    // -----------------------------------------------------------------------
    // Registry Initialization methods
    // -----------------------------------------------------------------------
    /** @name Registry Initialization Functions */
    //@{

    /**
     * Initializes registry with primitive and derived Simple types.
     *
     * This method does not clear the registry to clear the registry you
     * have to call resetRegistry.
     *
     * The net effect of this method is to start with the smallest set of
     * datatypes needed by the validator.
     *
     * If we start with Schema's then we initialize to full set of
     * validators.	
     */
    void expandRegistryToFullSchemaSet();

    //@}

    // -----------------------------------------------------------------------
    // Canonical Representation Group
    // -----------------------------------------------------------------------
           void                        initCanRepRegistory();

    static XMLCanRepGroup::CanRepGroup getCanRepGroup(const DatatypeValidator* const);

    // -----------------------------------------------------------------------
    // Validator Factory methods
    // -----------------------------------------------------------------------
    /** @name Validator Factory Functions */
    //@{

    /**
     * Creates a new datatype validator of type baseValidator's class and
     * adds it to the registry
     *
     * @param  typeName        Datatype validator name
     *
     * @param  baseValidator   Base datatype validator
     *
     * @param  facets          datatype facets if any
     *
     * @param  enums           vector of values for enum facet
     *
     * @param  isDerivedByList Indicates whether the datatype is derived by
     *                         list or not
     *
     * @param  finalSet       'final' values of the simpleType
     *
     * @param  isUserDefined  Indicates whether the datatype is built-in or
     *                        user defined
     */
    DatatypeValidator* createDatatypeValidator
    (
        const XMLCh* const                    typeName
        , DatatypeValidator* const            baseValidator
        , RefHashTableOf<KVStringPair>* const facets
        , RefArrayVectorOf<XMLCh>* const      enums
        , const bool                          isDerivedByList
        , const int                           finalSet = 0
        , const bool                          isUserDefined = true
        , MemoryManager* const                manager = XMLPlatformUtils::fgMemoryManager
    );

    /**
     * Creates a new datatype validator of type UnionDatatypeValidator and
     * adds it to the registry
     *
     * @param  typeName       Datatype validator name
     *
     * @param  validators     Vector of datatype validators
     *
     * @param  finalSet       'final' values of the simpleType
     *
     * @param  isUserDefined  Indicates whether the datatype is built-in or
     *                        user defined
     */
    DatatypeValidator* createDatatypeValidator
    (
          const XMLCh* const                    typeName
        , RefVectorOf<DatatypeValidator>* const validators
        , const int                             finalSet
        , const bool                            isUserDefined = true
        , MemoryManager* const                  manager = XMLPlatformUtils::fgMemoryManager
    );

    //@}

    /**
      * Reset datatype validator registry
      */
    void resetRegistry();

    // -----------------------------------------------------------------------
    //  Notification that lazy data has been deleted
    // -----------------------------------------------------------------------
    static void reinitRegistry();

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(DatatypeValidatorFactory)

private:

    // -----------------------------------------------------------------------
    //  CleanUp methods
    // -----------------------------------------------------------------------
    void cleanUp();

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    DatatypeValidatorFactory(const DatatypeValidatorFactory&);
    DatatypeValidatorFactory& operator=(const DatatypeValidatorFactory&);

    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fUserDefinedRegistry
    //      This is a hashtable of user defined dataype validators.
    //
    //  fBuiltInRegistry
    //      This is a hashtable of built-in primitive datatype validators.
    // -----------------------------------------------------------------------
    XERCES_CPP_NAMESPACE_QUALIFIER RefHashTableOf<XERCES_CPP_NAMESPACE_QUALIFIER DatatypeValidator>*        fUserDefinedRegistry;
    static XERCES_CPP_NAMESPACE_QUALIFIER RefHashTableOf<DatatypeValidator>* fBuiltInRegistry;
    static XERCES_CPP_NAMESPACE_QUALIFIER RefHashTableOf<XMLCanRepGroup>*    fCanRepRegistry;
    XERCES_CPP_NAMESPACE_QUALIFIER MemoryManager* const fMemoryManager;

    friend class XPath2ContextImpl;
};

inline DatatypeValidator*
DatatypeValidatorFactory::getDatatypeValidator(const XMLCh* const dvType) const
{
	if (dvType) {
        if (fBuiltInRegistry && fBuiltInRegistry->containsKey(dvType)) {
		    return fBuiltInRegistry->get(dvType);
        }

        if (fUserDefinedRegistry && fUserDefinedRegistry->containsKey(dvType)) {
		    return fUserDefinedRegistry->get(dvType);

        }
    }
	return 0;
}

inline DVHashTable*
DatatypeValidatorFactory::getUserDefinedRegistry() const {
    return fUserDefinedRegistry;
}

inline DVHashTable*
DatatypeValidatorFactory::getBuiltInRegistry() {
    return fBuiltInRegistry;
}
// ---------------------------------------------------------------------------
//  DatatypeValidator: CleanUp methods
// ---------------------------------------------------------------------------
inline void DatatypeValidatorFactory::cleanUp() {

	delete fUserDefinedRegistry;
	fUserDefinedRegistry = 0;
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file DatatypeValidatorFactory.hpp
  */

