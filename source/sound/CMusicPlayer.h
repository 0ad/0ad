#ifndef CMUSICPLAYER_H
#define CMUSICPLAYER_H

#include <string>
#include <iostream>
#include <stdio.h>

#include "oal.h"

#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#define AUDIO_BUFFER_SIZE (64*1024)

struct SOggFile
{
	char *dataPtr;
	size_t dataSize;
	size_t dataRead;
};

class CMusicPlayer
{
public:
	CMusicPlayer(void);
	~CMusicPlayer(void);
	void open(char *filename);
	void release();
	bool play();
	bool isPlaying();
	bool update();

protected:	
	bool stream(ALuint buffer);
	void check();
	void empty();
	std::string errorString(int errorcode);

private:
	bool is_open;
		// between open() and release(); used to determine
		// if source is actually valid, for isPlaying check.

	SOggFile memFile;
	ov_callbacks vorbisCallbacks;
	OggVorbis_File oggStream;
	vorbis_info *info;
	ALuint buffers[2]; //the 2 buffers, one will be filled whilst the other is being played
	ALuint source;
	ALenum format;
};

#endif
