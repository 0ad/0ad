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
 * $Id: ValueStore.hpp,v 1.6 2003/12/16 18:41:15 knoaman Exp $
 */

#if !defined(VALUESTORE_HPP)
#define VALUESTORE_HPP

/**
  * This class stores values associated to an identity constraint.
  * Each value stored corresponds to a field declared for the identity
  * constraint.
  */

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/validators/schema/identity/FieldValueMap.hpp>
#include <xercesc/util/RefVectorOf.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Forward Declaration
// ---------------------------------------------------------------------------
class IdentityConstraint;
class XMLScanner;
class ValueStoreCache;


class VALIDATORS_EXPORT ValueStore : public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
    ValueStore(IdentityConstraint* const ic,
               XMLScanner* const scanner,
               MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
	~ValueStore();

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    IdentityConstraint* getIdentityConstraint() const;

    // -----------------------------------------------------------------------
    //  Helper methods
    // -----------------------------------------------------------------------
    void append(const ValueStore* const other);
    void startValueScope();
    void endValueScope();
    void addValue(FieldActivator* const fieldActivator,
                  IC_Field* const field,
                  DatatypeValidator* const dv,
                  const XMLCh* const value);
    bool contains(const FieldValueMap* const other);

    /**
      * @deprecated
      */
    void addValue(IC_Field* const field, DatatypeValidator* const dv,
                  const XMLCh* const value);


    // -----------------------------------------------------------------------
    //  Document handling methods
    // -----------------------------------------------------------------------
    void endDcocumentFragment(ValueStoreCache* const valueStoreCache);

    // -----------------------------------------------------------------------
    //  Error reporting methods
    // -----------------------------------------------------------------------
    void duplicateValue();
    void reportNilError(IdentityConstraint* const ic);

private:
    // -----------------------------------------------------------------------
    //  Unimplemented contstructors and operators
    // -----------------------------------------------------------------------
    ValueStore(const ValueStore& other);
    ValueStore& operator= (const ValueStore& other);

    // -----------------------------------------------------------------------
    //  Helper methods
    // -----------------------------------------------------------------------
    /**
      * Returns whether a field associated <DatatypeValidator, String> value
      * is a duplicate of another associated value.
      * It is a duplicate only if either of these conditions are true:
      * - The Datatypes are the same or related by derivation and the values
      *   are in the same valuespace.
      * - The datatypes are unrelated and the values are Stringwise identical.
      */
    bool isDuplicateOf(DatatypeValidator* const dv1, const XMLCh* const val1,
                       DatatypeValidator* const dv2, const XMLCh* const val2);


    // -----------------------------------------------------------------------
    //  Data
    // -----------------------------------------------------------------------
    bool                        fDoReportError;
    int                         fValuesCount;
    IdentityConstraint*         fIdentityConstraint;
    FieldValueMap               fValues;
    RefVectorOf<FieldValueMap>* fValueTuples;
    ValueStore*                 fKeyValueStore;
    XMLScanner*                 fScanner; // for error reporting - REVISIT
    MemoryManager*              fMemoryManager;
};

// ---------------------------------------------------------------------------
//  ValueStore: Getter methods
// ---------------------------------------------------------------------------
inline IdentityConstraint*
ValueStore::getIdentityConstraint() const {
    return fIdentityConstraint;
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file ValueStore.hpp
  */

