/* Copyright (C) 2014 Wildfire Games.
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

#ifndef INCLUDED_ISOUNDMANAGER_H
#define INCLUDED_ISOUNDMANAGER_H

#include "lib/config2.h"
#include "lib/file/vfs/vfs_path.h"
#include "simulation2/system/Entity.h"

class CVector3D;

class ISoundManager
{
public:
	virtual ~ISoundManager() {};

	static void CreateSoundManager();
	static void SetEnabled(bool doEnable);
	static void CloseGame();

	virtual void StartWorker() = 0;

	virtual void IdleTask() = 0;
	virtual void Pause(bool pauseIt) = 0;

	virtual void SetMasterGain(float gain) = 0;
	virtual void SetMusicGain(float gain) = 0;
	virtual void SetAmbientGain(float gain) = 0;
	virtual void SetActionGain(float gain) = 0;
	virtual void SetUIGain(float gain) = 0;

	virtual void PlayAsUI(const VfsPath& itemPath, bool looping) = 0;
	virtual void PlayAsMusic(const VfsPath& itemPath, bool looping) = 0;
	virtual void PlayAsAmbient(const VfsPath& itemPath, bool looping) = 0;
	virtual void PlayAsGroup(const VfsPath& groupPath, CVector3D sourcePos, entity_id_t source, bool ownedSound) = 0;

	virtual bool InDistress() = 0;
};

extern ISoundManager*  g_SoundManager;

#endif // INCLUDED_ISOUNDMANAGER_H

