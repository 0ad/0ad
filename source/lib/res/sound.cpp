/*
* sound.cpp
* Author:			Graeme Kerry - graeme@wildfiregames.com
* Last Modified:	01/06/2004
*
* Contents:			implementation of sound fx resource functions
*/

#include "precompiled.h"

#include <stdio.h>
#include <fmod.h>

#include "h_mgr.h"
#include "sound.h"
#include "lib.h"

#ifdef _MSC_VER
#pragma comment(lib, "fmodvc.lib")
#endif


//define control block
struct Sound
{
	FSOUND_SAMPLE* clip;
	int channel;
};
//build its vtbl
H_TYPE_DEFINE(Sound);

static void Sound_init(Sound* s, va_list args)
{
	UNUSED(args);

	s->channel = -1;
}

static int Sound_reload(Sound* s, const char* filename, Handle)
{
	//only load if clip is empty
	if(s->clip == NULL)
		s->clip = FSOUND_Sample_Load(FSOUND_FREE,filename,FSOUND_LOOP_OFF | FSOUND_STEREO,0,0);

	//check if clip loaded successfully
	if(s->clip != NULL)
		return 0;
	else 
		return -1;
}

static void Sound_dtor(Sound* s)
{
	FSOUND_Sample_Free(s->clip);
}

Handle sound_load(const char* filename)
{
#ifdef _WIN32
	ONCE(
		FSOUND_Init(44100, 32, 0);
		atexit2(FSOUND_Close, 0, CC_STDCALL_0);
	);
#else
	ONCE(
		FSOUND_Init(44100, 32, 0);
		atexit2((void*)FSOUND_Close, 0, CC_CDECL_0);
	);
#endif

	return h_alloc(H_Sound,filename,0,0);
}

int sound_free(Handle& h)
{
	return h_free(h,H_Sound);
}

int sound_play(const Handle h)
{
	H_DEREF(h,Sound,s);

	s->channel = FSOUND_PlaySound(FSOUND_FREE,s->clip);
	return 0;
}

int sound_stop(const Handle h)
{
	H_DEREF(h,Sound,s);

	if(s->channel != -1)
		FSOUND_StopSound(s->channel);
	return 0;
}
