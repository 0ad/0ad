/**
 * =========================================================================
 * File        : wposix_types.h
 * Project     : 0 A.D.
 * Description : type definitions needed for wposix.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2004-2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

//
// <inttypes.h>
//

typedef signed char int8_t;
typedef short int16_t;

// already defined by MinGW
#if MSC_VERSION
typedef int int32_t;
#endif

#if MSC_VERSION || ICC_VERSION || LCC_VERSION
typedef __int64 int64_t;
#else
typedef long long int64_t;
#endif

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#if MSC_VERSION || ICC_VERSION || LCC_VERSION
typedef unsigned __int64 uint64_t;
#else
typedef unsigned long long uint64_t;
#endif

// note: we used to define [u]intptr_t here (if not already done).
// however, it's not necessary with VC7 and later, and the compiler's
// definition squelches 'pointer-to-int truncation' warnings, so don't.


//
// <sys/types.h>
//

typedef long ssize_t;
