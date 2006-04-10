#include "precompiled.h"

#include "lib.h"
#include "snd.h"

char snd_card[SND_CARD_LEN];
char snd_drv_ver[SND_DRV_VER_LEN];

void snd_detect()
{
#if OS_WIN
	extern LibError win_get_snd_info();
	win_get_snd_info();
#else
	// At least reset the values for unhandled platforms. Should perhaps do
	// something like storing the OpenAL version or similar.
	debug_assert(SND_CARD_LEN >= 8 && SND_DRV_VER_LEN >= 8);	// protect strcpy
	SAFE_STRCPY(snd_card, "Unknown");
	SAFE_STRCPY(snd_drv_ver, "Unknown");
#endif
}