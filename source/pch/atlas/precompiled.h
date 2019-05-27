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

#define MINIMAL_PCH 2
#include "lib/precompiled.h"	// common precompiled header

// Atlas-specific PCH:

#if HAVE_PCH

// These headers are included in over 60% of files.
#include "tools/atlas/GameInterface/Messages.h"
#include "ps/CStr.h"
#include "ps/Game.h"

#include <boost/unordered_map.hpp>

#endif // HAVE_PCH
