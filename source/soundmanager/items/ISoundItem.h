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

#ifndef INCLUDED_ISOUNDITEM_H
#define INCLUDED_ISOUNDITEM_H

#include "lib/config2.h"

#if CONFIG2_AUDIO

#include "maths/Vector3D.h"
#include "ps/CStr.h"
#include "soundmanager/data/SoundData.h"

class ISoundItem
{

public:
	virtual ~ISoundItem(){};
	virtual bool GetLooping() = 0;
	virtual void SetLooping(bool loop) = 0;
	virtual bool IsPlaying() = 0;


	virtual const Path GetName() = 0;
	virtual bool IdleTask() = 0;
	virtual	bool IsFading() = 0;
	virtual bool Finished() = 0;

	virtual void Play() = 0;
	virtual void Stop() = 0;

	virtual void Attach(CSoundData* itemData) = 0;

	virtual void EnsurePlay() = 0;

	virtual void PlayAndDelete() = 0;
	virtual void StopAndDelete() = 0;
	virtual void FadeToIn(float newVolume, double fadeDuration) = 0;
	virtual void FadeAndDelete(double fadeTime) = 0;
	virtual void FadeAndPause(double fadeTime) = 0;
	virtual void PlayLoop() = 0;

	virtual void SetCone(float innerCone, float outerCone, float coneGain) = 0;
	virtual void SetPitch(float pitch) = 0;
	virtual void SetGain(float gain) = 0;
	virtual void SetLocation(const CVector3D& position) = 0;
	virtual void SetRollOff(float gain) = 0;

	virtual void Pause() = 0;
	virtual void Resume() = 0;
};

#endif // CONFIG2_AUDIO

#endif // INCLUDED_ISOUNDITEM_H
