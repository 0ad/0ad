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

#ifndef INCLUDED_CMUSICPLAYER
#define INCLUDED_CMUSICPLAYER

#include <string>
#include <iostream>
#include <stdio.h>

#include "lib/file/vfs/vfs_path.h"

//#include "oal.h"

/*
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
*/

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
	void Open(const VfsPath& pathname);
	void Release();
	bool Play();
	bool IsPlaying();
	bool Update();

protected:	
//	bool Stream(ALuint buffer);
	void Check();
	void Empty();
	std::string ErrorString(int errorcode);

private:
	bool is_open;
		// between open() and release(); used to determine
		// if source is actually valid, for isPlaying check.
/*
	SOggFile memFile;

	Handle hf;

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
