/* Copyright (C) 2013 Wildfire Games.
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
#include "precompiled.h"
#include "lib/sysdep/sysdep.h"
#include "ps/CStr.h"

CStr ps_generate_guid(void)
{
	// TODO: Ideally this will be guaranteed unique (and verified
	// cryptographically) since we'll rely on it to identify hosts
	// and associate them with player controls (e.g. to support
	// leaving/rejoining in-progress games), and we don't want
	// a host to masquerade as someone else.
	// For now, just try to pick a very random number.

	CStr guid;
	for (size_t i = 0; i < 2; ++i)
	{
		u32 r = 0;
		sys_generate_random_bytes((u8*)&r, sizeof(r));
		char buf[32];
		sprintf_s(buf, ARRAY_SIZE(buf), "%08X", r);
		guid += buf;
	}

	return guid;
}
