/* Copyright (C) 2010 Wildfire Games.
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

#ifndef INCLUDED_UNIT
#define INCLUDED_UNIT

#include <set>

#include "ps/CStr.h"

class CModel;
class CObjectEntry;
class CObjectManager;
class CSkeletonAnim;
class CUnitAnimation;

// note: we can't declare as static const size_t invalidId = ~size_t(0) in
// the class because it seems to be a grey area in the C++ standard whether
// or not the constant is propagated or needs an external definition.
// an enum causes conversion warnings in MSC, so we go with a file-scope
// constant.
const size_t invalidUnitId = ~size_t(0);


/////////////////////////////////////////////////////////////////////////////////////////////
// CUnit: simple "actor" definition - defines a sole object within the world
class CUnit
{
	NONCOPYABLE(CUnit);
private:
	// Private constructor. Needs complete list of selections for the variation.
	CUnit(CObjectEntry* object, CObjectManager& objectManager,
		const std::set<CStr>& actorSelections);

public:
	// Attempt to create a unit with the given actor, with a set of
	// suggested selections (with the rest being randomised).
	// Returns NULL on failure.
 	static CUnit* Create(const CStrW& actorName, const std::set<CStr>& selections, CObjectManager& objectManager);

	// destructor
	~CUnit();

	// get unit's template object
	const CObjectEntry& GetObject() const { return *m_Object; }
	// get unit's model data
	CModel& GetModel() const { return *m_Model; }

	/// See CUnitAnimation::SetAnimationState
	void SetAnimationState(const CStr& name, bool once, float speed, float desync, bool keepSelection, const CStrW& soundgroup);

	/// See CUnitAnimation::SetAnimationSync
	void SetAnimationSync(float actionTime, float repeatTime);

	/**
	 * Update the model's animation.
	 * @param frameTime time in seconds
	 */
	void UpdateModel(float frameTime);

	// Sets the entity-selection, and updates the unit to use the new
	// actor variation.
	void SetEntitySelection(const CStr& selection);

	// Most units have a hopefully-unique ID number, so they can be referred to
	// persistently despite saving/loading maps. Default for new units is -1; should
	// usually be set to CUnitManager::GetNewID() after creation.
	size_t GetID() const { return m_ID; }
	void SetID(size_t id) { m_ID = id; }

	const std::set<CStr>& GetActorSelections() const { return m_ActorSelections; }
	
	void SetActorSelections(const std::set<CStr>& selections);

private:
	// object from which unit was created; never NULL
	CObjectEntry* m_Object;
	// object model representation; never NULL
	CModel* m_Model;

	CUnitAnimation* m_Animation;

	// unique (per map) ID number for units created in the editor, as a
	// permanent way of referencing them. ~0 for non-editor units.
	size_t m_ID;

	// actor-level selections for this unit
	std::set<CStr> m_ActorSelections;
	// entity-level selections for this unit
	std::set<CStr> m_EntitySelections;

	// object manager which looks after this unit's objectentry
	CObjectManager& m_ObjectManager;

	void ReloadObject();

	friend class CUnitAnimation;
};

#endif
