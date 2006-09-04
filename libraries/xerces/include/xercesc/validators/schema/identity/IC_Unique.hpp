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
 * $Id: IC_Unique.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#if !defined(IC_UNIQUE_HPP)
#define IC_UNIQUE_HPP


/**
  * Schema unique identity constraint
  */

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/validators/schema/identity/IdentityConstraint.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class VALIDATORS_EXPORT IC_Unique: public IdentityConstraint
{
public:
    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
    IC_Unique(const XMLCh* const identityConstraintName,
              const XMLCh* const elemName,
              MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
	~IC_Unique();

	// -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    short getType() const;

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(IC_Unique)

    IC_Unique(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

private:
    // -----------------------------------------------------------------------
    //  Unimplemented contstructors and operators
    // -----------------------------------------------------------------------
    IC_Unique(const IC_Unique& other);
    IC_Unique& operator= (const IC_Unique& other);
};


// ---------------------------------------------------------------------------
//  IC_Unique: Getter methods
// ---------------------------------------------------------------------------
inline short IC_Unique::getType() const {

    return IdentityConstraint::UNIQUE;
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file IC_Unique.hpp
  */

