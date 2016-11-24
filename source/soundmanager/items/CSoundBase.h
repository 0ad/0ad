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

#ifndef INCLUDED_CSOUNDBASE_H
#define INCLUDED_CSOUNDBASE_H

#include "lib/config2.h"

#if CONFIG2_AUDIO

#include "lib/external_libraries/openal.h"
#include "ps/ThreadUtil.h"
#include "soundmanager/data/SoundData.h"
#include "soundmanager/items/ISoundItem.h"

class CSoundBase : public ISoundItem
{
protected:

	ALuint m_ALSource;
	CSoundData* m_SoundData;

	bool m_LastPlay;
	bool m_Looping;
	bool m_ShouldBePlaying;
	bool m_PauseAfterFade;
	bool m_IsPaused;

	double m_StartFadeTime;
	double m_EndFadeTime;

	ALfloat	m_StartVolume;
	ALfloat	m_EndVolume;
	CMutex m_ItemMutex;

public:
	CSoundBase();

	virtual ~CSoundBase();

	bool InitOpenAL();
	void ResetVars();
	void EnsurePlay();

	void SetGain(ALfloat gain);
	void SetRollOff(ALfloat gain);
	void SetPitch(ALfloat pitch);
	void SetDirection(const CVector3D& direction);
	void SetCone(ALfloat innerCone, ALfloat outerCone, ALfloat coneGain);
	void SetLastPlay(bool last);
	void ReleaseOpenAL();
	bool IsFading();
	bool Finished();

	void Play();
	void PlayAndDelete();
	void PlayLoop();
	void Stop();
	void StopAndDelete();
	void FadeToIn(ALfloat newVolume, double fadeDuration);

	bool GetLooping();
	bool IsPlaying();

	void SetLocation(const CVector3D& position);
	void FadeAndDelete(double fadeTime);
	void FadeAndPause(double fadeTime);

	void Pause();
	void Resume();

	const Path GetName();

	virtual void SetLooping(bool loops);
	virtual bool IdleTask();
	virtual void Attach(CSoundData* itemData);

protected:

	void SetNameFromPath(VfsPath& itemPath);
	void ResetFade();
	bool HandleFade();
};

#endif // CONFIG2_AUDIO

#endif // INCLUDED_CSOUNDBASE_H
