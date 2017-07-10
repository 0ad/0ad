/* Copyright (C) 2017 Wildfire Games.
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
#include "lib/snd.h"

#include <stdio.h>
#include <stdlib.h>

#include "lib/external_libraries/openal.h"

std::string snd_card;
std::string snd_drv_ver;

void snd_detect()
{
	// OpenAL alGetString might not return anything interesting on certain platforms
	// (see https://stackoverflow.com/questions/28960638 for an example).
	// However our previous code supported only Windows, and alGetString does work on
	// Windows, so this is an improvement.

	// Sound cards

	const ALCchar* devices = nullptr;
	if (alcIsExtensionPresent(nullptr, "ALC_enumeration_EXT") == AL_TRUE)
	{
		if (alcIsExtensionPresent(nullptr, "ALC_enumerate_all_EXT") == AL_TRUE)
			devices = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);
		else
			devices = alcGetString(nullptr, ALC_DEVICE_SPECIFIER);
	}
	WARN_IF_FALSE(devices);

	snd_card.clear();
	do {
		snd_card += devices;
		devices += strlen(devices) + 1;
		snd_card += "; ";
	} while (*devices);

	// Driver version
	snd_drv_ver = alGetString(AL_VERSION);
}
