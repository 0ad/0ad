/* Copyright (C) 2020 Wildfire Games.
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

#ifndef INCLUDED_ICMPSOUNDMANAGER
#define INCLUDED_ICMPSOUNDMANAGER

#include "simulation2/system/Interface.h"

#include "lib/file/vfs/vfs_path.h"
#include "maths/FixedVector3D.h"
#include "simulation2/helpers/Player.h"

/**
 * Interface to the engine's sound system.
 */
class ICmpSoundManager : public IComponent
{
public:
	/**
	 * Start playing audio defined by a sound group file.
	 * @param name VFS path of sound group .xml, relative to audio/.
	 * @param source entity emitting the sound (used for positioning).
	 */
	virtual void PlaySoundGroup(const std::wstring& name, entity_id_t source) = 0;

	/**
	 * Start playing audio defined by a sound group file.
	 * @param name VFS path of sound group .xml, relative to audio/.
	 * @param sourcePos 3d position of the sound emitter.
	 */
	virtual void PlaySoundGroupAtPosition(const std::wstring& name, const CFixedVector3D& sourcePos) = 0;

	/**
	 * Start playing audio defined by a sound group file for a player.
	 * @param name VFS path of sound group .xml, relative to audio/.
	 * @param player the player entity.
	 */
	virtual void PlaySoundGroupForPlayer(const VfsPath& groupPath, const player_id_t playerId) const = 0;

	virtual void StopMusic() = 0;

	DECLARE_INTERFACE_TYPE(SoundManager)
};

#endif // INCLUDED_ICMPSOUNDMANAGER
