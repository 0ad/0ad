#ifndef CMUSICPLAYER_H
#define CMUSICPLAYER_H

#include <string>
#include <iostream>
#include <stdio.h>

//#include "oal.h"

/*
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
*/

#include "res/res.h"
/*
struct SOggFile
{
	char *dataPtr;
	size_t dataSize;
	size_t dataRead;
};

const size_t RAW_BUF_SIZE = 32*KB;
const int NUM_SLOTS = 3;

*/

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
//	bool stream(ALuint buffer);
	void check();
	void empty();
	std::string errorString(int errorcode);

private:
	bool is_open;
		// between open() and release(); used to determine
		// if source is actually valid, for isPlaying check.
/*
	SOggFile memFile;

	Handle hf;
/*
	struct IOSlot
	{
		ALuint al_buffer;
		Handle hio;
		void* raw_buf;
	};
	IOSlot slots[NUM_SLOTS];
*/
//	ALuint source;
};

#endif
