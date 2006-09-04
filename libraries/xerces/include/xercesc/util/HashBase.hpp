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

#if !defined(HASHBASE_HPP)
#define HASHBASE_HPP

#include <xercesc/util/XMemory.hpp>
#include <xercesc/util/PlatformUtils.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
 * The <code>HashBase</code> interface is the general outline of a hasher.
 * Hashers are used in <code>RefHashTableOf</code> hashtables to hash any
 * type of key.  An examples is the <code>HashXMLCh</code> class which is
 * designed to produce hash values for XMLCh* strings.  Any hasher inheriting
 * from <code>HashBase</code> may be specified when the RefHashTableOf hashtable is constructed.
 */
class XMLUTIL_EXPORT HashBase : public XMemory
{

public:
	
	/**
      * Returns a hash value based on the key
      *
      * @param key the key to be hashed
	  * @param mod the modulus the hasher should use
      */
	virtual unsigned int getHashVal(const void *const key, unsigned int mod
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager) = 0;

	/**
      * Compares two keys and determines if they are semantically equal
      *
      * @param key1 the first key to be compared
	  * @param key2 the second key to be compared
	  *
	  * @return true if they are equal
      */
	virtual bool equals(const void *const key1, const void *const key2) = 0;

    virtual ~HashBase() {};

    HashBase() {};

private:
	// -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    HashBase(const HashBase&);
    HashBase& operator=(const HashBase&);    
};

XERCES_CPP_NAMESPACE_END

#endif
