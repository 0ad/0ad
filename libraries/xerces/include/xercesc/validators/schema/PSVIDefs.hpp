/*
 * Copyright 2003,2004 The Apache Software Foundation.
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

#if !defined(PSVIDEFS_HPP)
#define PSVIDEFS_HPP

XERCES_CPP_NAMESPACE_BEGIN

class VALIDATORS_EXPORT PSVIDefs
{
public:
    enum Validity {
        UNKNOWN = 1,
        INVALID = 2,
        VALID   = 3
    };
    
    enum Validation {
        NONE    = 1,
        PARTIAL = 2,
        FULL    = 3
    };

    enum Complexity {
        SIMPLE  = 1,
        COMPLEX = 2
    };

    enum PSVIScope
    {
        SCP_ABSENT    // declared in group/attribute group
	    , SCP_GLOBAL  // global decalarion or ref
	    , SCP_LOCAL   // local declaration
    };
};

XERCES_CPP_NAMESPACE_END

#endif
