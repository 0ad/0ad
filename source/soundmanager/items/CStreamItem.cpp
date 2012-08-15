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

#include "CStreamItem.h"

#include "soundmanager/data/OggData.h"

#include <iostream>

CStreamItem::CStreamItem(CSoundData* sndData)
{
	ResetVars();
	if (InitOpenAL())
		Attach(sndData);
}

CStreamItem::~CStreamItem()
{
	Stop();

	int num_processed;
	alGetSourcei(m_ALSource, AL_BUFFERS_PROCESSED, &num_processed);
	
	if (num_processed > 0)
	{
		ALuint* al_buf = new ALuint[num_processed];
		alSourceUnqueueBuffers(m_ALSource, num_processed, al_buf);
		delete[] al_buf;
	}
}

bool CStreamItem::IdleTask()
{
	HandleFade();

	int proc_state;
	alGetSourceiv(m_ALSource, AL_SOURCE_STATE, &proc_state);
	
	if (proc_state == AL_STOPPED)
	{
		if (m_LastPlay)
			return (proc_state != AL_STOPPED);
	}
	else
	{
		COggData* tmp = (COggData*)m_SoundData;
		
		if (! tmp->IsFileFinished())
		{
			int num_processed;
			alGetSourcei(m_ALSource, AL_BUFFERS_PROCESSED, &num_processed);
			
			if (num_processed > 0)
			{
				ALuint* al_buf = new ALuint[num_processed];
				alSourceUnqueueBuffers(m_ALSource, num_processed, al_buf);
				int didWrite = tmp->FetchDataIntoBuffer(num_processed, al_buf);
				alSourceQueueBuffers(m_ALSource, didWrite, al_buf);
				delete[] al_buf;
			}
		}
		else if (GetLooping())
		{
			tmp->ResetFile();
		}
	}
	return true;
}

void CStreamItem::Attach(CSoundData* itemData)
{
	if (itemData != NULL)
	{
		m_SoundData = itemData->IncrementCount();
		alSourceQueueBuffers(m_ALSource, m_SoundData->GetBufferCount(), (const ALuint *)m_SoundData->GetBufferPtr());
	}
}

void CStreamItem::SetLooping(bool loops)
{
	m_Looping = loops;
}

