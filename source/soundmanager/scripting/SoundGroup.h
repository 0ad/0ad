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
	eRandOrder = 0x01,
	eRandGain = 0x02,
	eRandPitch = 0x04,
	eLoop = 0x08,
	eOmnipresent = 0x10,
	eDistanceless = 0x20,
	eOwnerOnly = 0x40
};

// Loads up a group of sound files with shared properties,
// and provides a simple interface for playing them.
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
#if CONFIG2_AUDIO
	// We store the handles so we can load now and play later
	std::vector<CSoundData*> m_SoundGroups;
#endif
	// We need the filenames so we can reload when necessary.
	std::vector<std::wstring> m_Filenames;
	// The file path for the list of sound file resources
	VfsPath m_Filepath;
	size_t m_CurrentSoundIndex;
	float m_ConeInnerAngle;
	float m_ConeOuterAngle;
	float m_ConeOuterGain;
	// Time elapsed since soundgroup was created
	float m_CurTime;
	float m_Decay;
	float m_Gain;
	float m_GainUpper;
	float m_GainLower;
	// The allowable intensity before a sound switch
	float m_IntensityThreshold;
	float m_Pitch;
	float m_PitchLower;
	float m_PitchUpper;
	float m_Priority;
	// Up to eight individual parameters, use with eSndGrpFlags.
	uint8_t m_Flags;
};

#endif //#ifndef INCLUDED_SOUNDGROUP_H
