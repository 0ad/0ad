/*
 * The Apache Software License, Version 1.1
 *
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
 *    permission, please contact apache@apache.org.
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
 * individuals on behalf of the Apache Software Foundation and was
 * originally based on software copyright (c) 2001, International
 * Business Machines, Inc., http://www.apache.org.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/**
 * $Id: GrammarResolver.hpp,v 1.18 2004/01/29 11:51:21 cargilld Exp $
 */

#if !defined(GRAMMARRESOLVER_HPP)
#define GRAMMARRESOLVER_HPP

#include <xercesc/framework/XMLGrammarPool.hpp>
#include <xercesc/util/RefHashTableOf.hpp>
#include <xercesc/util/StringPool.hpp>
#include <xercesc/validators/common/Grammar.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class DatatypeValidator;
class DatatypeValidatorFactory;
class XMLGrammarDescription;

/**
 * This class embodies the representation of a Grammar pool Resolver.
 * This class is called from the validator.
 *
 */

class VALIDATORS_EXPORT GrammarResolver : public XMemory
{
public:

    /** @name Constructor and Destructor */
    //@{
    /**
     *
     * Default Constructor
     */
    GrammarResolver(
                    XMLGrammarPool* const gramPool
                  , MemoryManager*  const manager = XMLPlatformUtils::fgMemoryManager
                    );
    /**
      * Destructor
      */
    ~GrammarResolver();

    //@}

    /** @name Getter methods */
    //@{
    /**
     * Retrieve the DatatypeValidator
     *
     * @param uriStr the namespace URI
     * @param typeName the type name
     * @return the DatatypeValidator associated with namespace & type name
     */
    DatatypeValidator* getDatatypeValidator(const XMLCh* const uriStr,
                                            const XMLCh* const typeName);

    /**
     * Retrieve the grammar that is associated with the specified namespace key
     *
     * @param  gramDesc   grammar description for the grammar
     * @return Grammar abstraction associated with the grammar description
     */
    Grammar* getGrammar( XMLGrammarDescription* const gramDesc ) ;

    /**
     * Retrieve the grammar that is associated with the specified namespace key
     *
     * @param  namespaceKey   Namespace key into Grammar pool
     * @return Grammar abstraction associated with the NameSpace key.
     */
    Grammar* getGrammar( const XMLCh* const namespaceKey ) ;

    /**
     * Get an enumeration of Grammar in the Grammar pool
     *
     * @return enumeration of Grammar in Grammar pool
     */
    RefHashTableOfEnumerator<Grammar> getGrammarEnumerator() const;

    /**
     * Get an enumeration of the referenced Grammars 
     *
     * @return enumeration of referenced Grammars
     */
    RefHashTableOfEnumerator<Grammar> getReferencedGrammarEnumerator() const;

    /**
     * Get an enumeration of the cached Grammars in the Grammar pool
     *
     * @return enumeration of the cached Grammars in Grammar pool
     */
    RefHashTableOfEnumerator<Grammar> getCachedGrammarEnumerator() const;

    /**
     * Get a string pool of schema grammar element/attribute names/prefixes
     * (used by TraverseSchema)
     *
     * @return a string pool of schema grammar element/attribute names/prefixes
     */
    XMLStringPool* getStringPool();

    /**
     * Is the specified Namespace key in Grammar pool?
     *
     * @param  nameSpaceKey    Namespace key
     * @return True if Namespace key association is in the Grammar pool.
     */
    bool containsNameSpace( const XMLCh* const nameSpaceKey );

    inline XMLGrammarPool* const getGrammarPool() const;

    inline MemoryManager* const getGrammarPoolMemoryManager() const;

    //@}

    /** @name Setter methods */
    //@{

    /**
      * Set the 'Grammar caching' flag
      */
    void cacheGrammarFromParse(const bool newState);

    /**
      * Set the 'Use cached grammar' flag
      */
    void useCachedGrammarInParse(const bool newState);

    //@}


    /** @name GrammarResolver methods */
    //@{
    /**
     * Add the Grammar with Namespace Key associated to the Grammar Pool.
     * The Grammar will be owned by the Grammar Pool.
     *
     * @param  grammarToAdopt  Grammar abstraction used by validator.
     */
    void putGrammar(Grammar* const               grammarToAdopt );

    /**
     * Returns the Grammar with Namespace Key associated from the Grammar Pool
     * The Key entry is removed from the table (grammar is not deleted if
     * adopted - now owned by caller).
     *
     * @param  nameSpaceKey    Key to associate with Grammar abstraction
     */
    Grammar* orphanGrammar(const XMLCh* const nameSpaceKey);

    /**
     * Cache the grammars in fGrammarBucket to fCachedGrammarRegistry.
     * If a grammar with the same key is already cached, an exception is
     * thrown and none of the grammars will be cached.
     */
    void cacheGrammars();

    /**
     * Reset internal Namespace/Grammar registry.
     */
    void reset();
    void resetCachedGrammar();

    /**
     * Returns an XSModel, either from the GrammarPool or by creating one
     */
    XSModel*    getXSModel();


    ValueVectorOf<SchemaGrammar*>* getGrammarsToAddToXSModel();

    //@}

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    GrammarResolver(const GrammarResolver&);
    GrammarResolver& operator=(const GrammarResolver&);

    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fStringPool            The string pool used by TraverseSchema to store
    //                         element/attribute names and prefixes.
    //                         Always owned by Grammar pool implementation
    //
    //  fGrammarBucket         The parsed Grammar Pool, if no caching option.
    //
    //  fGrammarFromPool       Referenced Grammar Set, not owned
    //
    //  fGrammarPool           The Grammar Set either plugged or created. 
    //
    //  fDataTypeReg           DatatypeValidatorFactory registery
    //
    //  fMemoryManager         Pluggable memory manager for dynamic memory
    //                         allocation/deallocation
    // -----------------------------------------------------------------------
    bool                            fCacheGrammar;
    bool                            fUseCachedGrammar;
    bool                            fGrammarPoolFromExternalApplication;
    XMLStringPool*                  fStringPool;
    RefHashTableOf<Grammar>*        fGrammarBucket;
    RefHashTableOf<Grammar>*        fGrammarFromPool;
    DatatypeValidatorFactory*       fDataTypeReg;
    MemoryManager*                  fMemoryManager;
    XMLGrammarPool*                 fGrammarPool;
    XSModel*                        fXSModel;
    XSModel*                        fGrammarPoolXSModel;
    ValueVectorOf<SchemaGrammar*>*  fGrammarsToAddToXSModel;
};

inline XMLStringPool* GrammarResolver::getStringPool() {

    return fStringPool;
}


inline void GrammarResolver::useCachedGrammarInParse(const bool aValue)
{
    fUseCachedGrammar = aValue;
}

inline XMLGrammarPool* const GrammarResolver::getGrammarPool() const
{
    return fGrammarPool;
}

inline MemoryManager* const GrammarResolver::getGrammarPoolMemoryManager() const
{
    return fGrammarPool->getMemoryManager();
}

inline ValueVectorOf<SchemaGrammar*>* GrammarResolver::getGrammarsToAddToXSModel()
{
    return fGrammarsToAddToXSModel;
}

XERCES_CPP_NAMESPACE_END

#endif
