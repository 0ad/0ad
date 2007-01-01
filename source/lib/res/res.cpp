/**
 * =========================================================================
 * File        : res.cpp
 * Project     : 0 A.D.
 * Description : 
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

#include "precompiled.h"
#include "res.h"


AT_STARTUP(\
	error_setDescription(ERR::RES_UNKNOWN_FORMAT, "Unknown file format");\
	error_setDescription(ERR::RES_INCOMPLETE_HEADER, "File header not completely read");\
)
