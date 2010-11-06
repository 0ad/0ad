/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lib/precompiled.h"	// common precompiled header

// "engine"-specific PCH:

#include "ps/Pyrogenesis.h"	// old error system

#if HAVE_PCH

// some other external libraries that are used in several places:
// .. CStr is included very frequently, so a reasonable amount of time is
//    saved by including it here. (~10% in a full rebuild, as of r2365)
#include "ps/CStr.h"
#include "scripting/SpiderMonkey.h"
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#endif // HAVE_PCH
