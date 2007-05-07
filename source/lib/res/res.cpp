/**
 * =========================================================================
 * File        : res.cpp
 * Project     : 0 A.D.
 * Description : 
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "res.h"


AT_STARTUP(\
	error_setDescription(ERR::RES_UNKNOWN_FORMAT, "Unknown file format");\
	error_setDescription(ERR::RES_INCOMPLETE_HEADER, "File header not completely read");\
)
