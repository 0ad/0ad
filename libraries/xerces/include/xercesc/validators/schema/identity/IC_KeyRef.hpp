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
 * $Id: IC_KeyRef.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#if !defined(IC_KEYREF_HPP)
#define IC_KEYREF_HPP


// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/validators/schema/identity/IdentityConstraint.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class VALIDATORS_EXPORT IC_KeyRef: public IdentityConstraint
{
public:
    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
    IC_KeyRef(const XMLCh* const identityConstraintName,
              const XMLCh* const elemName,
              IdentityConstraint* const icKey,
              MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
	~IC_KeyRef();

	// -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    short getType() const;
    IdentityConstraint* getKey() const;

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(IC_KeyRef)

    IC_KeyRef(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

private:
    // -----------------------------------------------------------------------
    //  Unimplemented contstructors and operators
    // -----------------------------------------------------------------------
    IC_KeyRef(const IC_KeyRef& other);
    IC_KeyRef& operator= (const IC_KeyRef& other);

    // -----------------------------------------------------------------------
    //  Data members
    // -----------------------------------------------------------------------
    IdentityConstraint* fKey;
};


// ---------------------------------------------------------------------------
//  IC_KeyRef: Getter methods
// ---------------------------------------------------------------------------
inline short IC_KeyRef::getType() const {

    return IdentityConstraint::KEYREF;
}

inline IdentityConstraint* IC_KeyRef::getKey() const {

    return fKey;
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file IC_KeyRef.hpp
  */

