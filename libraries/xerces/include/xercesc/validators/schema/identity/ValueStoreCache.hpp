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
 * $Id: ValueStoreCache.hpp,v 1.6 2003/05/16 21:43:22 knoaman Exp $
 */

#if !defined(VALUESTORECACHE_HPP)
#define VALUESTORECACHE_HPP

/**
  * This class is used to store the values for identity constraints.
  *
  * Sketch of algorithm:
  *  - When a constraint is first encountered, its values are stored in the
  *    (local) fIC2ValueStoreMap;
  *  - Once it is validated (i.e., wen it goes out of scope), its values are
  *    merged into the fGlobalICMap;
  *  - As we encounter keyref's, we look at the global table to validate them.
  *  - Validation always occurs against the fGlobalIDConstraintMap (which
  *    comprises all the "eligible" id constraints). When an endelement is
  *    found, this Hashtable is merged with the one below in the stack. When a
  *    start tag is encountered, we create a new fGlobalICMap.
  *    i.e., the top of the fGlobalIDMapStack always contains the preceding
  *    siblings' eligible id constraints; the fGlobalICMap contains
  *    descendants+self. Keyrefs can only match descendants+self.
  */

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/RefVectorOf.hpp>
#include <xercesc/util/RefHashTableOf.hpp>
#include <xercesc/util/RefHash2KeysTableOf.hpp>
#include <xercesc/util/RefStackOf.hpp>
#include <xercesc/validators/schema/identity/IdentityConstraint.hpp>
#include <xercesc/validators/schema/identity/IC_Field.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Forward Declcaration
// ---------------------------------------------------------------------------
class ValueStore;
class SchemaElementDecl;
class XMLScanner;


class VALIDATORS_EXPORT ValueStoreCache : public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
    ValueStoreCache(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
	~ValueStoreCache();

	// -----------------------------------------------------------------------
    //  Setter Methods
    // -----------------------------------------------------------------------
    void setScanner(XMLScanner* const scanner);

	// -----------------------------------------------------------------------
    //  Document Handling methods
    // -----------------------------------------------------------------------
    void startDocument();
    void startElement();
    void endElement();
    void endDocument();

	// -----------------------------------------------------------------------
    //  Initialization methods
    // -----------------------------------------------------------------------
    void initValueStoresFor(SchemaElementDecl* const elemDecl, const int initialDepth);


	// -----------------------------------------------------------------------
    //  Access methods
    // -----------------------------------------------------------------------
    ValueStore* getValueStoreFor(const IC_Field* const field, const int initialDepth);
    ValueStore* getValueStoreFor(const IdentityConstraint* const ic, const int intialDepth);
    ValueStore* getGlobalValueStoreFor(const IdentityConstraint* const ic);

	// -----------------------------------------------------------------------
    //  Helper methods
    // -----------------------------------------------------------------------
    /** This method takes the contents of the (local) ValueStore associated
      * with ic and moves them into the global hashtable, if ic is a <unique>
      * or a <key>. If it's a <keyRef>, then we leave it for later.
      */
    void transplant(IdentityConstraint* const ic, const int initialDepth);

private:
    // -----------------------------------------------------------------------
    //  Unimplemented contstructors and operators
    // -----------------------------------------------------------------------
    ValueStoreCache(const ValueStoreCache& other);
    ValueStoreCache& operator= (const ValueStoreCache& other);

    // -----------------------------------------------------------------------
    //  Helper methods
    // -----------------------------------------------------------------------
    void init();
    void cleanUp();

    // -----------------------------------------------------------------------
    //  Data
    // -----------------------------------------------------------------------
    RefVectorOf<ValueStore>*                 fValueStores;
    RefHashTableOf<ValueStore>*              fGlobalICMap;
    RefHash2KeysTableOf<ValueStore>*         fIC2ValueStoreMap;
    RefStackOf<RefHashTableOf<ValueStore> >* fGlobalMapStack;
    XMLScanner*                              fScanner;
    MemoryManager*                           fMemoryManager;
};

// ---------------------------------------------------------------------------
//  ValueStoreCache: Access methods
// ---------------------------------------------------------------------------
inline void ValueStoreCache::setScanner(XMLScanner* const scanner) {

    fScanner = scanner;
}

// ---------------------------------------------------------------------------
//  ValueStoreCache: Access methods
// ---------------------------------------------------------------------------
inline ValueStore*
ValueStoreCache::getValueStoreFor(const IC_Field* const field, const int initialDepth) {

    return fIC2ValueStoreMap->get(field->getIdentityConstraint(), initialDepth);
}

inline ValueStore*
ValueStoreCache::getValueStoreFor(const IdentityConstraint* const ic, const int initialDepth) {

    return fIC2ValueStoreMap->get(ic, initialDepth);
}

inline ValueStore*
ValueStoreCache::getGlobalValueStoreFor(const IdentityConstraint* const ic) {

    return fGlobalICMap->get(ic);
}

// ---------------------------------------------------------------------------
//  ValueStoreCache: Document handling methods
// ---------------------------------------------------------------------------
inline void ValueStoreCache::endDocument() {
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file ValueStoreCache.hpp
  */

