#ifndef DomMemDebug_HEADER_GUARD_
#define DomMemDebug_HEADER_GUARD_

/*
 * Copyright 1999-2002,2004 The Apache Software Foundation.
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
 * $Id: DomMemDebug.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#include <xercesc/util/XercesDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN


//
// This class aids in debugging memory management problems with the
//  reference counted DOM classes - DOMStrings, Nodes (including subclasses),
//  and NamedNodeMaps.
//
// Usage Example:
//      DomMemDebug  initialState;   // Captures allocation totals
//          ...                     //    Test code performs DOM
//          ...                     //    operations here.
//
//      DomMemDebug  exitState;     //   Captures post-test state.
//      ExitState.printDifference(initialState);  // Display leaks.
//
class DEPRECATED_DOM_EXPORT DomMemDebug
{
public:
    int         liveStringHandles;
    int         totalStringHandles;
    int         liveStringBuffers;
    int         totalStringBuffers;
    int         liveNodeImpls;
    int         totalNodeImpls;
    int         liveNamedNodeMaps;
    int         totalNamedNodeMaps;

public:
    DomMemDebug();
    ~DomMemDebug();

    void        print();
    void        printDifference(const DomMemDebug &other);
    bool        operator == (const DomMemDebug &other);
    bool        operator != (const DomMemDebug &other);
    void        operator =  (const DomMemDebug &other);
};


XERCES_CPP_NAMESPACE_END

#endif
