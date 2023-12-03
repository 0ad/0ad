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

#include "precompiled.h"

#include "OggData.h"

#if CONFIG2_AUDIO

#include "ps/CLogger.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "soundmanager/SoundManager.h"

COggData::COggData()
	: m_Format(0), m_Frequency(0), m_OneShot(false), m_BuffersUsed(0)
{
}

COggData::~COggData()
{
	AL_CHECK;
	if (ogg)
		ogg->Close();

	AL_CHECK;
	if ( m_BuffersUsed > 0 )
		alDeleteBuffers(m_BuffersUsed, &m_Buffer[0]);

	AL_CHECK;
	m_BuffersUsed = 0;
}

void COggData::SetFormatAndFreq(int form, ALsizei freq)
{
	m_Format = form;
	m_Frequency = freq;
}

bool COggData::IsStereo()
{
	return m_Format == AL_FORMAT_STEREO16;
}

bool COggData::InitOggFile(const VfsPath& itemPath)
{
	CSoundManager* sndManager = (CSoundManager*)g_SoundManager;
	if (!sndManager)
		return false;

	int buffersToStart = sndManager->GetBufferCount();
	if (OpenOggNonstream(g_VFS, itemPath, ogg) != INFO::OK)
		return false;

	m_FileFinished = false;

	SetFormatAndFreq(ogg->Format(), ogg->SamplingRate());
	SetFileName(itemPath);

	AL_CHECK;

	alGenBuffers(buffersToStart, m_Buffer);

	ALenum err = alGetError();
	if (err != AL_NO_ERROR)
	{
		LOGERROR("Failed to create initial buffer. OpenAL error: %s\n", alGetString(err));
		return false;
	}

	m_BuffersUsed = FetchDataIntoBuffer(buffersToStart, m_Buffer);
	if (m_FileFinished)
	{
		m_OneShot = true;
		if (m_BuffersUsed < buffersToStart)
		{
			alDeleteBuffers(buffersToStart - m_BuffersUsed, &m_Buffer[m_BuffersUsed]);
		}
	}
	AL_CHECK;

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
	ogg->ResetFile();
	m_FileFinished = false;
}

bool COggData::IsOneShot()
{
	return m_OneShot;
}

int COggData::FetchDataIntoBuffer(int count, ALuint* buffers)
{
	CSoundManager* sndManager = (CSoundManager*)g_SoundManager;
	if (!sndManager)
		return 0;

	long bufferSize = sndManager->GetBufferSize();

	u8* pcmout = new u8[bufferSize + 5000];
	int buffersWritten = 0;

	for (int i = 0; i < count && !m_FileFinished; ++i)
	{
		memset(pcmout, 0, bufferSize + 5000);
		Status totalRet = ogg->GetNextChunk(pcmout, bufferSize);
		m_FileFinished = ogg->atFileEOF();
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

#endif // CONFIG2_AUDIO
