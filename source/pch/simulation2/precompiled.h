/* Copyright (C) 2019 Wildfire Games.
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

#define MINIMAL_PCH 1
#include "lib/precompiled.h"	// common precompiled header

// Minimal is a bit *too* minimal to let things compile, so include a few more headers
#include "lib/debug.h"

#if HAVE_PCH

// The simulation has many header files (component interfaces) which are quick to compile on their own
// but bring in a lot of headers indirectly. Adding these to the precompiled header
// cuts down compilation time by about half.
#include "simulation2/system/Component.h"
#include "simulation2/system/Interface.h"
#include "simulation2/system/InterfaceScripted.h"

#endif // HAVE_PCH
