/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * sound card detection.
 */

#include "precompiled.h"
#include "lib/sysdep/snd.h"

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
	ENSURE(SND_CARD_LEN >= 8 && SND_DRV_VER_LEN >= 8);	// protect strcpy
	wcscpy_s(snd_card, ARRAY_SIZE(snd_card), L"Unknown");
	wcscpy_s(snd_drv_ver, ARRAY_SIZE(snd_drv_ver), L"Unknown");
#endif
}
