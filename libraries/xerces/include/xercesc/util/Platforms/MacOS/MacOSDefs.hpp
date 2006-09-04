/*
 * Copyright 1999-2000,2004 The Apache Software Foundation.
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
 * $Id: MacOSDefs.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */


#ifndef MACOS_DEFS_HPP
#define MACOS_DEFS_HPP

// ---------------------------------------------------------------------------
//  MacOS runs in big endian mode.
// ---------------------------------------------------------------------------
#define ENDIANMODE_BIG

// ---------------------------------------------------------------------------
//  Define all the required platform types
//
//	FileHandle is a pointer to XMLMacAbstractFile. Due to namespace
//	declaration issues, it is declared here as a void*.
// ---------------------------------------------------------------------------
typedef void*   FileHandle;

#endif
