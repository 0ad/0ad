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

#include "CSoundItem.h"

#if CONFIG2_AUDIO

#include "soundmanager/data/SoundData.h"
#include "soundmanager/SoundManager.h"

#include <iostream>

CSoundItem::CSoundItem()
{
	ResetVars();
}

CSoundItem::CSoundItem(CSoundData* sndData)
{
	ResetVars();
	if (InitOpenAL())
		Attach(sndData);
}

CSoundItem::~CSoundItem()
{
	AL_CHECK
	
	Stop();

	if (m_ALSource != 0)
	{
		alSourcei(m_ALSource, AL_BUFFER, 0);
		AL_CHECK
	}	
}

bool CSoundItem::IdleTask()
{
	HandleFade();

	if (m_LastPlay && (m_ALSource != 0) )
	{
		int proc_state;
		alGetSourceiv(m_ALSource, AL_SOURCE_STATE, &proc_state);
		AL_CHECK
		return (proc_state != AL_STOPPED);
	}
	return true;
}

void CSoundItem::Attach(CSoundData* itemData)
{
	if (itemData != NULL && (m_ALSource != 0) )
	{
		m_SoundData = itemData->IncrementCount();
		alSourcei(m_ALSource, AL_BUFFER, m_SoundData->GetBuffer());
		
		AL_CHECK
	}
}

#endif // CONFIG2_AUDIO

