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

#ifndef INCLUDED_OBJECTENTRY
#define INCLUDED_OBJECTENTRY

class CModelAbstract;
class CSkeletonAnim;
class CObjectBase;
class CObjectManager;
class CSimulation2;

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "graphics/Color.h"
#include "lib/file/vfs/vfs_path.h"
#include "ps/CStr.h"

#include "graphics/ObjectBase.h"

class CObjectEntry
{
	NONCOPYABLE(CObjectEntry);

public:
	CObjectEntry(const std::shared_ptr<CObjectBase>& base, CSimulation2& simulation);
	~CObjectEntry();

	// Construct this actor, using the specified variation selections
	bool BuildVariation(const std::vector<const std::set<CStr>*>& completeSelections,
		const std::vector<u8>& variationKey, CObjectManager& objectManager);

	// Base actor. Contains all the things that don't change between
	// different variations of the actor.
	std::shared_ptr<CObjectBase> m_Base;

	// samplers list
	std::vector<CObjectBase::Samp> m_Samplers;
	// model name
	VfsPath m_ModelName;
	// color (used when doing alpha-channel coloring, but not doing player-color)
	CColor m_Color;
		// (probable TODO: make color a per-model thing, rather than per-objectEntry,
		// so we can have lots of color variations without wasting memory on
		// lots of objectEntries)

	std::wstring m_ProjectileModelName;

	/**
	 * Returns a randomly-chosen animation matching the given ID, or animationName if ID is empty.
	 * The chosen animation is picked randomly from the GetAnimations list
	 * with the frequencies as weights (if there are any defined).
	 * This method should always return an animation
	 */
	CSkeletonAnim* GetRandomAnimation(const CStr& animationName, const CStr& ID = "") const;

	/**
	 * Returns all the animations matching the given ID or animationName if ID is empty.
	 * If none found returns Idle animations (which are always added)
	 */
	std::vector<CSkeletonAnim*> GetAnimations(const CStr& animationName, const CStr& ID = "") const;

	// corresponding model
	CModelAbstract* m_Model;

private:

	CSimulation2& m_Simulation;

	using SkeletonAnimMap = std::multimap<CStr, std::unique_ptr<CSkeletonAnim>>;
	SkeletonAnimMap m_Animations;
		// TODO: something more memory-efficient than storing loads of similar strings for each unit?
};


#endif
