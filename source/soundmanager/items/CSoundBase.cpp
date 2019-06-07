/* Copyright (C) 2019 Wildfire Games.
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

#include "CSoundBase.h"

#if CONFIG2_AUDIO

#include "lib/timer.h"
#include "soundmanager/SoundManager.h"
#include "soundmanager/data/SoundData.h"
#include "ps/CLogger.h"

CSoundBase::CSoundBase()
{
	ResetVars();
}

CSoundBase::~CSoundBase()
{
	Stop();
	ReleaseOpenAL();
}

void CSoundBase::ReleaseOpenAL()
{
	if (m_ALSource != 0)
	{
		AL_CHECK;
		alSourcei(m_ALSource, AL_BUFFER, 0L);
		AL_CHECK;
		((CSoundManager*)g_SoundManager)->ReleaseALSource(m_ALSource);
		AL_CHECK;
		m_ALSource = 0;
	}
	if (m_SoundData != 0)
	{
		AL_CHECK;
		CSoundData::ReleaseSoundData(m_SoundData);
		AL_CHECK;
		m_SoundData = 0;
	}
}

void CSoundBase::Attach(CSoundData* itemData)
{
	UNUSED2(itemData);
}

void CSoundBase::ResetVars()
{
	m_ALSource = 0;
	m_SoundData = 0;
	m_LastPlay = false;
	m_Looping = false;
	m_StartFadeTime = 0;
	m_EndFadeTime = 0;
	m_StartVolume = 0;
	m_EndVolume = 0;
	m_ShouldBePlaying = false;
	m_IsPaused = false;
	ResetFade();
}

void CSoundBase::ResetFade()
{
	m_StartFadeTime = 0;
	m_EndFadeTime = 0;
	m_StartVolume = 0;
	m_EndVolume = 0;
	m_PauseAfterFade = false;
}

bool CSoundBase::Finished()
{
  return !m_ShouldBePlaying && !IsPlaying();
}

bool CSoundBase::InitOpenAL()
{
	alGetError(); /* clear error */
	m_ALSource = ((CSoundManager*)g_SoundManager)->GetALSource( this );

	AL_CHECK;

	if ( m_ALSource )
	{
		alSourcef(m_ALSource,AL_PITCH,1.0f);
		AL_CHECK;
		alSourcef(m_ALSource,AL_GAIN,1.0f);
		AL_CHECK;
		alSourcei(m_ALSource,AL_LOOPING,AL_FALSE);
		AL_CHECK;
		return true;
	}
	else
	{
//		LOGERROR("Source not allocated by SoundManager\n", 0);
	}
	return false;
}

void CSoundBase::SetGain(ALfloat gain)
{
	AL_CHECK;

	if ( m_ALSource )
	{
		std::lock_guard<std::mutex> lock(m_ItemMutex);
		alSourcef(m_ALSource, AL_GAIN, gain);
		AL_CHECK;
	}
}

void CSoundBase::SetRollOff(ALfloat rolls)
{
	if ( m_ALSource )
	{
		std::lock_guard<std::mutex> lock(m_ItemMutex);
		alSourcef(m_ALSource, AL_REFERENCE_DISTANCE, 70.0f);
		AL_CHECK;
		alSourcef(m_ALSource, AL_MAX_DISTANCE, 200.0);
		AL_CHECK;
		alSourcef(m_ALSource, AL_ROLLOFF_FACTOR, rolls);
		AL_CHECK;
	}
}

void CSoundBase::EnsurePlay()
{
	if (m_ShouldBePlaying && !m_IsPaused && !IsPlaying())
		Play();
}

void CSoundBase::SetCone(ALfloat innerCone, ALfloat outerCone, ALfloat coneGain)
{
	if ( m_ALSource )
	{
		std::lock_guard<std::mutex> lock(m_ItemMutex);
		AL_CHECK;
		alSourcef(m_ALSource, AL_CONE_INNER_ANGLE, innerCone);
		AL_CHECK;
		alSourcef(m_ALSource, AL_CONE_OUTER_ANGLE, outerCone);
		AL_CHECK;
		alSourcef(m_ALSource, AL_CONE_OUTER_GAIN, coneGain);
		AL_CHECK;
	}
}

void CSoundBase::SetPitch(ALfloat pitch)
{
	if ( m_ALSource )
	{
		std::lock_guard<std::mutex> lock(m_ItemMutex);
		alSourcef(m_ALSource, AL_PITCH, pitch);
		AL_CHECK;
	}
}

void CSoundBase::SetDirection(const CVector3D& direction)
{
	if ( m_ALSource )
	{
		std::lock_guard<std::mutex> lock(m_ItemMutex);
		alSourcefv(m_ALSource, AL_DIRECTION, direction.GetFloatArray());
		AL_CHECK;
	}
}


bool CSoundBase::IsPlaying()
{
	if ( m_ALSource )
	{
		std::lock_guard<std::mutex> lock(m_ItemMutex);
		int proc_state;
		alGetSourcei(m_ALSource, AL_SOURCE_STATE, &proc_state);
		AL_CHECK;

		return (proc_state == AL_PLAYING);
	}
	return false;
}

void CSoundBase::SetLastPlay(bool last)
{
	m_LastPlay = last;
}

bool CSoundBase::IdleTask()
{
	return true;
}

void CSoundBase::SetLocation (const CVector3D& position)
{
	if ( m_ALSource != 0 )
	{
		std::lock_guard<std::mutex> lock(m_ItemMutex);
		alSourcefv(m_ALSource,AL_POSITION, position.GetFloatArray());
		AL_CHECK;
	}
}

bool CSoundBase::HandleFade()
{
	AL_CHECK;
	if (m_ALSource == 0)
		return true;

	if (m_StartFadeTime != 0)
	{
		double currTime = timer_Time();
		double pctDone = std::min(1.0, (currTime - m_StartFadeTime) / (m_EndFadeTime - m_StartFadeTime));
		pctDone = std::max(0.0, pctDone);
		ALfloat curGain = ((m_EndVolume - m_StartVolume) * pctDone) + m_StartVolume;

		if  (curGain == 0)
		{
			if ( m_PauseAfterFade )
				Pause();
			else
				Stop();
		}
		else if (curGain == m_EndVolume)
		{
			if (m_ALSource != 0)
				alSourcef(m_ALSource, AL_GAIN, curGain);
			ResetFade();
		}
		else if (m_ALSource != 0)
			alSourcef(m_ALSource, AL_GAIN, curGain);

		AL_CHECK;
	}
	return true;
}

bool CSoundBase::IsFading()
{
	return ((m_ALSource != 0) && (m_StartFadeTime != 0));
}


bool CSoundBase::GetLooping()
{
	return m_Looping;
}

void CSoundBase::SetLooping(bool loops)
{
	m_Looping = loops;
	if (m_ALSource != 0)
	{
		alSourcei(m_ALSource, AL_LOOPING, loops ? AL_TRUE : AL_FALSE);
		AL_CHECK;
	}
}

void CSoundBase::Play()
{
	std::lock_guard<std::mutex> lock(m_ItemMutex);

	m_ShouldBePlaying = true;
	m_IsPaused = false;
	AL_CHECK;
	if (m_ALSource != 0)
	{
		alSourcePlay(m_ALSource);
		ALenum err = alGetError();
		if (err != AL_NO_ERROR)
		{
			if (err == AL_INVALID)
				((CSoundManager*)g_SoundManager)->SetDistressThroughError();
			else
				((CSoundManager*)g_SoundManager)->al_ReportError(err, __func__, __LINE__);
		}
	}
}

void CSoundBase::PlayAndDelete()
{
	SetLastPlay(true);
	Play();
}

void CSoundBase::FadeAndPause(double fadeTime)
{
	m_PauseAfterFade = true;
	FadeToIn(0, fadeTime);
}

void CSoundBase::FadeAndDelete(double fadeTime)
{
	SetLastPlay(true);
	FadeToIn(0, fadeTime);
}

void CSoundBase::StopAndDelete()
{
	SetLastPlay(true);
	Stop();
}

void CSoundBase::PlayLoop()
{
	if (m_ALSource != 0)
	{
		SetLooping(true);
		Play();
		AL_CHECK;
	}
}

void CSoundBase::FadeToIn(ALfloat newVolume, double fadeDuration)
{
	if (m_ALSource != 0)
	{
		ALenum proc_state;
		alGetSourcei(m_ALSource, AL_SOURCE_STATE, &proc_state);
		if (proc_state == AL_PLAYING)
		{
			m_StartFadeTime = timer_Time();
			m_EndFadeTime = m_StartFadeTime + fadeDuration;
			alGetSourcef(m_ALSource, AL_GAIN, &m_StartVolume);
			m_EndVolume = newVolume;
		}
		AL_CHECK;
	}
}

void CSoundBase::Stop()
{
	m_ShouldBePlaying = false;
	if (m_ALSource != 0)
	{
		std::lock_guard<std::mutex> lock(m_ItemMutex);

		AL_CHECK;
		alSourcei(m_ALSource, AL_LOOPING, AL_FALSE);
		AL_CHECK;
		alSourceStop(m_ALSource);
		AL_CHECK;
	}
}

const Path CSoundBase::GetName()
{
	if (m_SoundData)
		return m_SoundData->GetFileName();

	return Path();
}

void CSoundBase::Pause()
{
	if (m_ALSource != 0)
	{
		m_IsPaused = true;
		alSourcePause(m_ALSource);
		AL_CHECK;
	}
}

void CSoundBase::Resume()
{
	if (m_ALSource != 0)
	{
		alSourcePlay(m_ALSource);
		AL_CHECK;
	}
}

#endif // CONFIG2_AUDIO

