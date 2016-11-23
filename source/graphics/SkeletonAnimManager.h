/* Copyright (C) 2009 Wildfire Games.
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

#include <map>
#include <set>
#include "lib/file/vfs/vfs_path.h"
#include <boost/unordered_map.hpp>

class CColladaManager;
class CSkeletonAnimDef;
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

private:
	// map of all known animations. Value is NULL if it failed to load.
	boost::unordered_map<VfsPath, CSkeletonAnimDef*> m_Animations;

	CColladaManager& m_ColladaManager;
};

#endif
