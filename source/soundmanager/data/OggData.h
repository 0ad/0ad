/* Copyright (C) 2015 Wildfire Games.
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

#ifndef INCLUDED_OGGDATA_H
#define INCLUDED_OGGDATA_H

#include "lib/config2.h"

#if CONFIG2_AUDIO

#include "ogg.h"
#include "SoundData.h"

#include "lib/external_libraries/openal.h"
#include "lib/file/vfs/vfs_path.h"

class COggData : public CSoundData
{
	ALuint m_Format;
	long m_Frequency;

public:
	COggData();
	virtual ~COggData();

	virtual bool InitOggFile(const VfsPath& itemPath);
	virtual bool IsFileFinished();
	virtual bool IsOneShot();
	virtual bool IsStereo();

	virtual int FetchDataIntoBuffer(int count, ALuint* buffers);
	virtual void ResetFile();

protected:
	OggStreamPtr  ogg;
	bool m_FileFinished;
	bool m_OneShot;
	ALuint m_Buffer[100];
	int m_BuffersUsed;

	bool AddDataBuffer(char* data, long length);
	void SetFormatAndFreq(int form, ALsizei freq);
	int  GetBufferCount();
	unsigned int GetBuffer();
	unsigned int* GetBufferPtr();
};

#endif // CONFIG2_AUDIO
#endif // INCLUDED_OGGDATA_H
