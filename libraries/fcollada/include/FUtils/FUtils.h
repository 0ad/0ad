/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/*
	Based on the FS Import classes:
	Copyright (C) 2005-2006 Feeling Software Inc
	Copyright (C) 2005-2006 Autodesk Media Entertainment
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FUtils.h
	Includes the common utilities classes and macros.
*/

/** @defgroup FUtils Utility Classes. */

#ifndef _F_UTILS_H_
#define _F_UTILS_H_

// STL
#ifdef _WIN32
#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE 1 // MSVS 2005 support.
#endif // _CRT_SECURE_NO_DEPRECATE
#pragma warning(disable:4702)

#ifdef _DEBUG
#define _HAS_ITERATOR_DEBUGGING 1
#endif // _DEBUG
#endif

#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <algorithm>
#include <iostream>
#include <fstream>

#undef min
#define min(a, b) std::min(a, b) /**< Retrieves the smallest of two values. */
#undef max
#define max(a, b) std::max(a, b) /**< Retrieves the largest of two values. */

#ifdef _WIN32
#pragma warning(default:4702)
#endif

// Pre-include the platform-specific macros and definitions
#include "FUtils/Platforms.h"
#include "FUtils/FUAssert.h"

// PLUG_CRT enforces CRT memory checks every 128 allocations
// and CRT memory leaks output
#if defined(_DEBUG) && defined(WIN32) && defined(MEMORY_DEBUG)
#define PLUG_CRT
#ifdef PLUG_CRT
#include <crtdbg.h>
#define DEBUG_CLIENTBLOCK new( _CLIENT_BLOCK, __FILE__, __LINE__)
#define new DEBUG_CLIENTBLOCK
#endif // PLUG_CRT
#endif // _DEBUG & WIN32 & MEMORY_DEBUG

// FMath
#define HAS_VECTORTYPES /**< Used by FCollada, implies that we are including all the common dynamically-sized arrays. */
#include "FMath/FMath.h"

// LibXML
#ifndef NO_LIBXML
#define HAS_LIBXML /**< Used by FCollada, implies that we are including LibXML functions in the library interface. */
#define LIBXML_STATIC /**< Used by LibXML, implies that we are statically-linking the LibXML. */
#include <libxml/tree.h>
#else
typedef struct _xmlNode xmlNode;
#endif
typedef fm::pvector<struct _xmlNode> xmlNodeList; /**< A dynamically-sized array of XML nodes. */

// SAFE_DELETE Macro set.
#define SAFE_DELETE(ptr) if ((ptr) != NULL) { delete (ptr); (ptr) = NULL; } /**< This macro safely deletes a pointer and sets the given pointer to NULL. */
#define SAFE_DELETE_ARRAY(ptr) if (ptr != NULL) { delete [] ptr; ptr = NULL; } /**< This macro safely deletes an heap array and sets the given pointer to NULL. */
#define SAFE_FREE(ptr) if (ptr != NULL) { free(ptr); ptr = NULL; } /**< This macro safely frees a memory block and sets the given pointer to NULL. */
#define SAFE_RELEASE(ptr) if ((ptr) != NULL) { (ptr)->Release(); (ptr) = NULL; } /**< This macro safely releases an interface and sets the given pointer to NULL. */
#define CLEAR_POINTER_VECTOR(a) { size_t l = (a).size(); for (size_t i = 0; i < l; ++i) SAFE_DELETE((a)[i]); (a).clear(); } /**< This macro deletes all the object pointers contained within a vector and clears it. */
#define CLEAR_ARRAY_VECTOR(a) { size_t l = (a).size(); for (size_t i = 0; i < l; ++i) SAFE_DELETE_ARRAY((a)[i]); (a).clear(); } /**< This macro deletes all the object array pointers contained within a vector and clears it. */
#define CLEAR_POINTER_STD_PAIR_CONT(cont, a){ for (cont::iterator it = (a).begin(); it != (a).end(); ++it) SAFE_DELETE((*it).second); (a).clear(); } /**< This macro deletes all the object pointers contained within any std container and clears it. */
#define CLEAR_POINTER_STD_CONT(cont, a){ for (cont::iterator it = (a).begin(); it != (a).end(); ++it) SAFE_DELETE((*it)); (a).clear(); } /**< This macro deletes all the object pointers contained within any std container and clears it. */

// Conversion macros
#define UNUSED(a) /**< Removes a piece of code during the pre-process. This macro is useful for these pesky unused variable warnings. */
#ifdef _DEBUG
#define UNUSED_NDEBUG(a) a
#else
#define UNUSED_NDEBUG(a) /**< Removes a piece of debug code during the pre-process. This macro is useful for these pesky unused variable warnings. */
#endif // _DEBUG

// Base RTTI object and object container class
#include "FUtils/FUObjectType.h"
#include "FUtils/FUObject.h"

// More complex utility classes
#include "FUtils/FUString.h"
#include "FUtils/FUCrc32.h"
#include "FUtils/FUDebug.h"
#include "FUtils/FUError.h"

#ifndef RETAIL
#define ENABLE_TEST /**< Used by FCollada, enables the compilation and gives access to the unit tests. Disabled in the retail configurations. */
#endif

#endif // _F_UTILS_H_
