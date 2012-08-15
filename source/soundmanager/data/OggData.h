/* Copyright (C) 2012 Wildfire Games.
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

#include "SoundData.h"
#include "lib/external_libraries/openal.h"
#include "vorbis/vorbisfile.h"

class COggData : public CSoundData
{
	ALuint m_Format;
	long m_Frequency;

public:
	COggData();
	virtual ~COggData();

	virtual bool InitOggFile(const wchar_t* fileLoc);
	virtual bool IsFileFinished();
	virtual bool IsOneShot();

	virtual int FetchDataIntoBuffer(int count, ALuint* buffers);
	virtual void ResetFile();

protected:
	OggVorbis_File  m_vf;
	int m_current_section;
	bool m_FileFinished;
	bool m_OneShot;
	ALuint m_Buffer[100];
	int m_BuffersUsed;

	bool AddDataBuffer(char* data, long length);
	void SetFormatAndFreq(int form, ALsizei freq);
	ALsizei  GetBufferCount();
	ALuint GetBuffer();
	ALuint* GetBufferPtr();
};


#endif // INCLUDED_OGGDATA_H
