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

#include "precompiled.h"

#include "OggData.h"

#include "soundmanager/SoundManager.h"

#include <wchar.h>
#include <iostream>

COggData::COggData()
{
	m_OneShot = false;
}

COggData::~COggData()
{
	alDeleteBuffers(m_BuffersUsed, m_Buffer);
	ov_clear(&m_vf);
}

void COggData::SetFormatAndFreq(int form, ALsizei freq)
{
	m_Format = form;
	m_Frequency = freq;
}

bool COggData::InitOggFile(const wchar_t* fileLoc)
{
	int buffersToStart = g_SoundManager->GetBufferCount();
	char nameH[300];
	sprintf(nameH, "%ls", fileLoc);
	
	FILE* f = fopen(nameH, "rb");
	m_current_section = 0;
	int err = ov_open_callbacks(f, &m_vf, NULL, 0, OV_CALLBACKS_DEFAULT);
	if (err < 0)
	{
		fprintf(stderr,"Input does not appear to be an Ogg bitstream :%d :%d.\n", err, ferror(f));
		return false;
	}

	m_FileName = CStrW(fileLoc);

	m_FileFinished = false;
	SetFormatAndFreq((m_vf.vi->channels == 1)? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16 , (ALsizei)m_vf.vi->rate);

	alGetError(); /* clear error */
	alGenBuffers(buffersToStart, m_Buffer);
	
	if(alGetError() != AL_NO_ERROR) 
	{
		printf("- Error creating initial buffer !!\n");
		return false;
	}
	else
	{
		m_BuffersUsed = FetchDataIntoBuffer(buffersToStart, m_Buffer);
		if (m_FileFinished)
		{
			m_OneShot = true;
			if (m_BuffersUsed < buffersToStart)
			{
				alDeleteBuffers(buffersToStart - m_BuffersUsed, &m_Buffer[m_BuffersUsed]);
			}
		}
	}
	return true;
}

ALsizei COggData::GetBufferCount()
{
	return m_BuffersUsed;
}

bool COggData::IsFileFinished()
{
	return m_FileFinished;
}

void COggData::ResetFile()
{
	ov_time_seek(&m_vf, 0);
	m_current_section = 0;
	m_FileFinished = false;
}

bool COggData::IsOneShot()
{
	return m_OneShot;
}

int COggData::FetchDataIntoBuffer(int count, ALuint* buffers)
{
	long bufferSize = g_SoundManager->GetBufferSize();
	
	char* pcmout = new char[bufferSize + 5000];
	int buffersWritten = 0;
	
	for (int i = 0; (i < count) && !m_FileFinished; i++)
	{
		char* readDest = pcmout;
		long totalRet = 0;
		while (totalRet < bufferSize)
		{
			long ret=ov_read(&m_vf,readDest, 4096,0,2,1, &m_current_section);
			if (ret == 0)
			{
				m_FileFinished=true;
				break;
			}
			else if (ret < 0)
			{
				/* error in the stream.  Not a problem, just reporting it in
				 case we (the app) cares.  In this case, we don't. */
			}
			else
			{
				totalRet += ret;
				readDest += ret;
			}
		}
		if (totalRet > 0)
		{
			buffersWritten++;
			alBufferData(buffers[i], m_Format, pcmout, (ALsizei)totalRet, (int)m_Frequency);
		}
	}
	delete[] pcmout;
	return buffersWritten;
}


ALuint COggData::GetBuffer()
{
	return m_Buffer[0];
}

ALuint* COggData::GetBufferPtr()
{
	return m_Buffer;
}





