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

#include "CBufferItem.h"

#if CONFIG2_AUDIO

#include "soundmanager/SoundManager.h"
#include "soundmanager/data/SoundData.h"

CBufferItem::CBufferItem(CSoundData* sndData)
{
	ResetVars();
	if (InitOpenAL())
		Attach(sndData);
}


CBufferItem::~CBufferItem()
{
	Stop();
	ReleaseOpenALBuffer();
}


void CBufferItem::ReleaseOpenALBuffer()
{
	if ( m_ALSource == 0 )
		return;

	int num_processed;
	AL_CHECK;
	alGetSourcei(m_ALSource, AL_BUFFERS_PROCESSED, &num_processed);
	AL_CHECK;

	if (num_processed > 0)
	{
		ALuint* al_buf = new ALuint[num_processed];
		alSourceUnqueueBuffers(m_ALSource, num_processed, al_buf);

		AL_CHECK;
		delete[] al_buf;
	}
	alSourcei(m_ALSource, AL_BUFFER, 0);
	((CSoundManager*)g_SoundManager)->ReleaseALSource(m_ALSource);
	AL_CHECK;

	m_ALSource = 0;
}

bool CBufferItem::IdleTask()
{
	if ( m_ALSource == 0 )
		return false;

	HandleFade();

	if (m_LastPlay)
	{
		CScopeLock lock(m_ItemMutex);
		int proc_state;
		alGetSourcei(m_ALSource, AL_SOURCE_STATE, &proc_state);
		AL_CHECK;
		m_ShouldBePlaying = (proc_state != AL_STOPPED);
		return (proc_state != AL_STOPPED);
	}

	if (GetLooping())
	{
		int num_processed;
		AL_CHECK;
		alGetSourcei(m_ALSource, AL_BUFFERS_PROCESSED, &num_processed);

		AL_CHECK;
		for (int i = 0; i < num_processed; i++)
		{
			ALuint al_buf;
			alSourceUnqueueBuffers(m_ALSource, 1, &al_buf);
			AL_CHECK;
			alSourceQueueBuffers(m_ALSource, 1, &al_buf);
			AL_CHECK;
		}
	}

	return true;
}

void CBufferItem::Attach(CSoundData* itemData)
{
	if ( m_ALSource == 0 )
		return;

	AL_CHECK;
	if (m_SoundData != NULL)
	{
		CSoundData::ReleaseSoundData(m_SoundData);
		m_SoundData = 0;
	}
	AL_CHECK;
	if (itemData != NULL)
	{
		m_SoundData = itemData->IncrementCount();
		alSourceQueueBuffers(m_ALSource, m_SoundData->GetBufferCount(),(const ALuint *) m_SoundData->GetBufferPtr());
		AL_CHECK;
	}
}

void CBufferItem::SetLooping(bool loops)
{
	m_Looping = loops;
}

#endif // CONFIG2_AUDIO
