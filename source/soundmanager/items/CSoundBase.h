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

#ifndef INCLUDED_CSOUNDBASE_H
#define INCLUDED_CSOUNDBASE_H

#include "lib/config2.h"

#if CONFIG2_AUDIO

#include "lib/external_libraries/openal.h"
#include "soundmanager/items/ISoundItem.h"
#include "soundmanager/data/SoundData.h"

#include <string>


class CSoundBase : public ISoundItem
{
protected:
	
	ALuint m_ALSource;
	CSoundData* m_SoundData;

	std::string* m_Name;
	bool m_LastPlay;
	bool m_Looping;
	bool m_ShouldBePlaying;
	
	double m_StartFadeTime;
	double m_EndFadeTime;
	ALfloat	m_StartVolume;
	ALfloat	m_EndVolume;

public:
	CSoundBase();
	
	virtual ~CSoundBase();
	
	virtual bool InitOpenAL();
	virtual void ResetVars();
	virtual void EnsurePlay();

	virtual void SetGain(ALfloat gain);
	virtual void SetRollOff(ALfloat gain);
	virtual	void SetPitch(ALfloat pitch);
	virtual	void SetDirection(const CVector3D& direction);
	virtual	void SetCone(ALfloat innerCone, ALfloat outerCone, ALfloat coneGain);
	virtual void SetLastPlay(bool last);

	void Play();
	void PlayAndDelete();
	bool IdleTask();
	void PlayLoop();
	void Stop();
	void StopAndDelete();
	void FadeToIn(ALfloat newVolume, double fadeDuration);

	void PlayAsMusic();
	void PlayAsAmbient();

	const char* Name();
	std::string GetName();

	virtual bool GetLooping();
	virtual void SetLooping(bool loops);
	virtual bool IsPlaying();
	virtual void SetLocation(const CVector3D& position);
	virtual void FadeAndDelete(double fadeTime);

protected:

	void SetNameFromPath(char* fileLoc);
	void ResetFade();
	bool HandleFade();

	
};

#endif // CONFIG2_AUDIO

#endif // INCLUDED_CSOUNDBASE_H

