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

#include "precompiled.h"

#include "CMusicPlayer.h"
#include "ps/CLogger.h"

#include <sstream>
#include <list>

#define LOG_CATEGORY L"audio"






//Class implementation
CMusicPlayer::CMusicPlayer(void)
{
/*
	oal_Init();

	if(!alIsExtensionPresent((ALubyte*)"AL_EXT_vorbis"))
		debug_warn(L"no OpenAL ogg extension");
*/
	is_open = false;
}

CMusicPlayer::~CMusicPlayer(void)
{
	Release();
}

void CMusicPlayer::Open(const VfsPath& UNUSED(pathname))
{
	// If a new file is opened while another is already in memory,
	// close the old one first.
	if (is_open)
		Release();

/*
	void* p;
	size_t sizeOfFile;
	if(vfs_load(pathname, p, sizeOfFile) != INFO::OK)
	{
		LOG(CLogger::Error, LOG_CATEGORY, L"CMusicPlayer::open(): vfs_load for %ls failed!\n", pathname.string().c_str());
		return;
	}
	else
		LOG(CLogger::Normal,  LOG_CATEGORY, L"CMusicPlayer::open(): file %ls loaded successfully\n", pathname.string().c_str());
	memFile.dataPtr = (char*)p;
	memFile.dataRead = 0;
	memFile.dataSize = sizeOfFile;	
*/
/*
	hf = vfs_open(pathname);

	for(int i = 0; i < NUM_BUFS; i++)
	{
		alGenBuffers(1, &bufs[i].al_buffer);
		bufs[i].raw_buf = malloc(RAW_BUF_SIZE);
	}
	
	alGenSources(1, &source);
	check();

	alSource3f(source,AL_POSITION,0.0,0.0,0.0);
	alSource3f(source,AL_VELOCITY,0.0,0.0,0.0);
	alSource3f(source,AL_DIRECTION,0.0,0.0,0.0);
	alSourcef(source,AL_ROLLOFF_FACTOR,0.0);
	alSourcei(source,AL_SOURCE_RELATIVE,AL_TRUE);
	check();
*/
	is_open = true;
}


void CMusicPlayer::Release()
{
/*
	if(!is_open)
		return;
	is_open = false;

	alSourceStop(source);
	empty();

	alDeleteSources(1,&source);
	source = 0;

	
	for(int i = 0; i < NUM_BUFS; i++)
	{
		alDeleteBuffers(1, &bufs[i].al_buffer);
		bufs[i].al_buffer = 0;
		mem_free(bufs[i].raw_buf);
	}

	check();

	mem_free(memFile.dataPtr);
*/
}


bool CMusicPlayer::IsPlaying()
{
	return false;
}
/*
	// guard against OpenAL using source when it's not initialized
	if(!is_open)
		return false;

	ALenum state = 0;
	alGetSourcei(source,AL_SOURCE_STATE,&state);
	check();
	return (state == AL_PLAYING);
}
*/
/*

bool CMusicPlayer::Issue(int slot_idx)
{
	Buf* buf = &bufs[slot_idx];

	ssize_t left = (ssize_t)memFile.dataSize - (ssize_t)memFile.dataRead;
	ssize_t size = std::min(64*KB, left);
	debug_assert(size >= 0);
	void* data = memFile.dataPtr;
	data = (char*)data + memFile.dataRead;
	memFile.dataRead += size;

	alBufferData(buf->al_buffer, AL_FORMAT_VORBIS_EXT, data, (ALsizei)size, 1);
	alSourceQueueBuffers(source, 1, buf->al_buffer);

	check();

	return true;
}
*/

bool CMusicPlayer::Play()
{
	Check();

	if(IsPlaying())
		return true;

	if(!is_open)
		debug_warn(L"play() called before open()");
/*
	if(!stream(0))
		return false;
	if(!stream(1))
		return false;
*/
	//bind the 2 buffers to the source

//	alSourcePlay(source);
//	check();

	return true;
}


bool CMusicPlayer::Update()
{
	if(!IsPlaying())
		return false;
/*
	//check which buffers have already been played
	int processed;
	alGetSourcei(source,AL_BUFFERS_PROCESSED, &processed);
	check();

	// start transfers on any buffers that have completed playing
	// any transfers that have completed: add the corresponding buffer

	bool active = true;

	while(processed-- && processed >= 0)
	{
		ALuint buffer;
		
		//remove buffer from queue
		alSourceUnqueueBuffers(source,1,&buffer);
		check();

		// to which Buf does the al_buffer belong?
		int i;
		for(i = 0; i < NUM_BUFS; i++)
			if(bufs[i].al_buffer == buffer)
				goto found;
		debug_warn(L"al_buffer not found!");
found:

		//fill buffer with new data if false is returned the there is no more data
		active = stream(buffer);

		//attach buffer to end of queue
		alSourceQueueBuffers(source,1,&buffer);
		check();
	}
	
	return active;
*/
	return false;
}


void CMusicPlayer::Check()
{
/*
	int error = alGetError();

	if(error != AL_NO_ERROR)
	{
		std::string str = errorString(error);
		LOG(CLogger::Error, LOG_CATEGORY, L"OpenAL error: %hs\n", str.c_str());
	}
*/
}


void CMusicPlayer::Empty()
{
/*
	int queued;
    alGetSourcei(source,AL_BUFFERS_QUEUED,&queued);

	while(queued-- > 0)
	{
		ALuint buffer;

		alSourceUnqueueBuffers(source,1,&buffer);
		check();
	}
*/
}




std::string CMusicPlayer::ErrorString(int UNUSED(errorcode))
{
	/*
	switch(errorcode)
    {
	case AL_INVALID_NAME:
		return "AL_INVALID_NAME";
	case AL_INVALID_ENUM:
		return "AL_INVALID_ENUM";
	case AL_INVALID_VALUE:
		return "AL_INVALID_VALUE";
	case AL_INVALID_OPERATION:
		return "AL_INVALID_OPERATION";
	case AL_OUT_OF_MEMORY:
		return "AL_OUT_OF_MEMORY";
        default:
			std::stringstream str;
			str << "Unknown Ogg error (code "<< errorcode << ")";
			return str.str();
    }
	*/
	return "ENOSYS";
}
