/* Copyright (C) 2016 Wildfire Games.
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

/**
* =========================================================================
* File		: SoundGroup.h
* Project	 : 0 A.D.
* Description : Loads up a group of sound files with shared properties,
*				and provides a simple interface for playing them.
* =========================================================================
*/

/*
Example SoundGroup.xml
	<?xml version="1.0" encoding="utf-8"?>
	<SoundGroup>
		<Gain>1.0</Gain>
		<Looping>0</Looping>
		<Pitch>1.0</Pitch>
		<Priority>100</Priority>
		<RandOrder>0</RandOrder>
		<RandGain>0</RandGain>
		<RandPitch>0</RandPitch>
		<ConeGain>1.0</ConeGain>
		<ConeInner>360</ConeInner>
		<ConeOuter>360</ConeOuter>
		<Path>audio/resource/hellenes/soldier/</Path>
		<Sound>Attack_Attackx.ogg</Sound>
		<Sound>Attack_Chargex.ogg</Sound>
		<Sound>Attack_Engagex.ogg</Sound>
		<Sound>Attack_ForMyFamily.ogg</Sound>
	</SoundGroup>
*/

#ifndef INCLUDED_SOUNDGROUP_H
#define INCLUDED_SOUNDGROUP_H

#include "lib/config2.h"
#include "lib/file/vfs/vfs_path.h"
#include "simulation2/system/Entity.h"
#include "soundmanager/data/SoundData.h"

#include <vector>

class CVector3D;
class ISoundItem;

enum eSndGrpFlags
{
	eRandOrder		= 0x01,
	eRandGain		= 0x02,
	eRandPitch		= 0x04,
	eLoop			= 0x08,
	eOmnipresent	= 0x10,
	eDistanceless	= 0x20,
	eOwnerOnly = 0x40
};


class CSoundGroup
{
	NONCOPYABLE(CSoundGroup);
public:
	CSoundGroup(const VfsPath& pathnameXML);
	CSoundGroup();
	~CSoundGroup();

	// Play next sound in group
	// @param position world position of the entity generating the sound
	// (ignored if the eOmnipresent flag is set)
	void PlayNext(const CVector3D& position, entity_id_t source);

	float RadiansOffCenter(const CVector3D& position, bool& onScreen, float& itemRollOff);

	// Load a group
	bool LoadSoundGroup(const VfsPath& pathnameXML);

	void Reload();

	// Release all remaining loaded handles
	void ReleaseGroup();

	// Update SoundGroup, remove dead sounds from intensity count
	void Update(float TimeSinceLastFrame);

	// Set a flag using a value from eSndGrpFlags
	inline void SetFlag(int flag) { m_Flags = (unsigned char)(m_Flags | flag); }

	// Test flag, returns true if flag is set.
	inline bool TestFlag(int flag) { return (m_Flags & flag) != 0; }

private:
	void SetGain(float gain);

	void UploadPropertiesAndPlay(size_t theIndex, const CVector3D& position, entity_id_t source);

	void SetDefaultValues();

	size_t m_index;  // index of the next sound to play

#if CONFIG2_AUDIO
	std::vector<CSoundData*> snd_group;  // we store the handles so we can load now and play later
#endif
	std::vector<std::wstring> filenames; // we need the filenames so we can reload when necessary.

	VfsPath m_filepath; // the file path for the list of sound file resources

	float m_CurTime; // Time elapsed since soundgroup was created
	float m_TimeWindow; // The Intensity Threshold Window
	size_t m_IntensityThreshold; // the allowable intensity before a sound switch
	size_t m_Intensity;  // our current intensity (number of sounds played since m_CurTime - m_TimeWindow)
	float m_Decay;
	unsigned char m_Flags; // up to eight individual parameters, use with eSndGrpFlags.

	float m_Gain;
	float m_Pitch;
	float m_Priority;
	float m_ConeOuterGain;
	float m_PitchUpper;
	float m_PitchLower;
	float m_GainUpper;
	float m_GainLower;
	float m_ConeInnerAngle;
	float m_ConeOuterAngle;
};

#endif //#ifndef INCLUDED_SOUNDGROUP_H

