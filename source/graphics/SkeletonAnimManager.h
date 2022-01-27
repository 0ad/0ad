/* Copyright (C) 2022 Wildfire Games.
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

/*
 * Owner of all skeleton animations
 */

#ifndef INCLUDED_SKELETONANIMMANAGER
#define INCLUDED_SKELETONANIMMANAGER

#include "lib/file/vfs/vfs_path.h"

#include <memory>
#include <unordered_map>

class CColladaManager;
class CSkeletonAnimDef;
class CSkeletonAnim;
class CStr8;

///////////////////////////////////////////////////////////////////////////////
// CSkeletonAnimManager : owner class of all skeleton anims - manages creation,
// loading and destruction of animation data
class CSkeletonAnimManager
{
	NONCOPYABLE(CSkeletonAnimManager);
public:
	// constructor, destructor
	CSkeletonAnimManager(CColladaManager& colladaManager);
	~CSkeletonAnimManager();

	// return a given animation by filename; return null if filename doesn't
	// refer to valid animation file
	CSkeletonAnimDef* GetAnimation(const VfsPath& pathname);

	/**
	 * Load raw animation frame animation from given file, and build an
	 * animation specific to this model.
	 * @param pathname animation file to load
	 * @param name animation name (e.g. "idle")
	 * @param ID specific ID of the animation, to sync with props
	 * @param frequency influences the random choices
	 * @param speed animation speed as a factor of the default animation speed
	 * @param actionpos offset of 'action' event, in range [0, 1]
	 * @param actionpos2 offset of 'action2' event, in range [0, 1]
	 * @param sound offset of 'sound' event, in range [0, 1]
	 * @return new animation, or NULL on error
	 */
	std::unique_ptr<CSkeletonAnim> BuildAnimation(const VfsPath& pathname, const CStr8& name, const CStr8& ID, int frequency, float speed, float actionpos, float actionpos2, float soundpos);

private:
	// map of all known animations. Value is NULL if it failed to load.
	std::unordered_map<VfsPath, std::unique_ptr<CSkeletonAnimDef>> m_Animations;

	CColladaManager& m_ColladaManager;
};

#endif
