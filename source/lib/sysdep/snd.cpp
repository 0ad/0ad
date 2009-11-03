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

/*
 * sound card detection.
 */

#include "precompiled.h"
#include "snd.h"

#if OS_WIN
# include "lib/sysdep/os/win/wsnd.h"
#endif


wchar_t snd_card[SND_CARD_LEN];
wchar_t snd_drv_ver[SND_DRV_VER_LEN];

void snd_detect()
{
	// note: OpenAL alGetString is worthless: it only returns
	// OpenAL API version and renderer (e.g. "Software").

#if OS_WIN
	win_get_snd_info();
#else
	// At least reset the values for unhandled platforms.
	debug_assert(SND_CARD_LEN >= 8 && SND_DRV_VER_LEN >= 8);	// protect strcpy
	SAFE_STRCPY(snd_card, "Unknown");
	SAFE_STRCPY(snd_drv_ver, "Unknown");
#endif
}
