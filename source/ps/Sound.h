/*
Sound.h
by Raj

Classes to play sounds or music using FMOD

Usage:	Create a CWindow object, call Create, call Run.

		If you want to handle events like mouse clicks, you need to derive your
		own class from CBaseWindow, and override the OnXXX() functions. For example,
		OnActivate() or OnPaint().
*/


-----support pan, volume, and crossfading



struct SSoundEffect
{
	Position
	Radius
	Volume
	Actual sound effect
	Layer
};


struct SoundScheme
{
	include several different sounds
	specify the looping time for this scheme
	and, 
}


Class methods

-SetCrossFadeSpeed: how quickly it takes to crossfade between current music and new music





#ifndef SOUND_H
#define SOUND_H

#pragma warning (disable: 4786)

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------

#include <Flamer.h>
#include <Sound.h>
#include <map>
#include <fmod.h>
#include <string>
#include <FileIO.h>

using namespace std;

DECLARE_ERROR(PS_SOUND_INIT);

typedef bool STREAM_OPTION;
const int STREAMING = true;
const int NO_STREAMING = false;

class CSample
{
public:

	operator FSOUND_STREAM * ();
	operator FSOUND_SAMPLE * ();

	operator = (FSOUND_STREAM *);
	operator = (FSOUND_SAMPLE *);

private:	
	union
	{
		FSOUND_STREAM *stream;
		FSOUND_SAMPLE *sample;
	};

};


// CSound: Class which plays sounds or music
class CSound
{
public:
	CSound();
	~CSound();

	PS_RESULT Init();
	PS_RESULT Release();

	PS_RESULT Load(string filename, string nickname, STREAM_OPTION useStreaming=false);
	PS_RESULT Play(string nickname);

	PS_RESULT SetMasterVolume(float vol);
	PS_RESULT SetSampleVolume(string sample, float vol);

	PS_RESULT Pause();
	PS_RESULT Resume();

private:
	map <string, CSample> soundList;

};


PS_RESULT CSound::Load(string filename, string nickname, STREAM_OPTION useStreaming)
{
	CSample newFile;

	if(useStreaming == STREAMING)
	{
			
	}
	else if(useStreaming == NO_STREAMING)
	{
		FSOUND_Sample_Load(FSOUND_FREE,        //let FSOUND select an arbitrary sample slot
						   filename.c_str(),   //name of the file to load
						   FSOUND_LOADMEMORY,
						   );
	}

	return PS_OK;
}

PS_RESULT CSound::Play(string nickname)
{
	return PS_OK;
}

PS_RESULT CSound::Release()
{
	return PS_OK;
}

PS_RESULT CSound::SetMasterVolume(float vol)
{
	return PS_OK;
}

PS_RESULT CSound::SetSampleVolume(string sample, float vol)
{
	return PS_OK;
}

PS_RESULT CSound::Pause()
{
	return PS_OK;
}

PS_RESULT CSound::Resume()
{
	return PS_OK;
}

#endif