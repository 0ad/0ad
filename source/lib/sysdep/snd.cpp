/**
 * =========================================================================
 * File        : snd.cpp
 * Project     : 0 A.D.
 * Description : sound card detection.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "snd.h"


char snd_card[SND_CARD_LEN];
char snd_drv_ver[SND_DRV_VER_LEN];

void snd_detect()
{
	// note: OpenAL alGetString is worthless: it only returns
	// OpenAL API version and renderer (e.g. "Software").

#if OS_WIN
	extern LibError win_get_snd_info();
	win_get_snd_info();
#else
	// At least reset the values for unhandled platforms.
	debug_assert(SND_CARD_LEN >= 8 && SND_DRV_VER_LEN >= 8);	// protect strcpy
	SAFE_STRCPY(snd_card, "Unknown");
	SAFE_STRCPY(snd_drv_ver, "Unknown");
#endif
}
