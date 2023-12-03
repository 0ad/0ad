/* Copyright (C) 2023 Wildfire Games.
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

#include "ps/CStr.h"
#include "simulation2/system/Entity.h"	// entity_id_t

#include <map>
#include <memory>
#include <set>

class CActorDef;
class CModelAbstract;
class CObjectEntry;
class CObjectManager;
class CUnitAnimation;


/////////////////////////////////////////////////////////////////////////////////////////////
// CUnit: simple "actor" definition - defines a sole object within the world
class CUnit
{
	NONCOPYABLE(CUnit);
private:
	// Private constructor. Needs complete list of selections for the variation.
	CUnit(CObjectManager& objectManager, const CActorDef& actor, const entity_id_t id, const uint32_t seed);

public:
	// Attempt to create a unit with the given actor.
	// If specific selections are wanted, call SetEntitySelection or SetActorSelections after creation.
	// Returns an empty `std::unique_ptr` on failure.
	static std::unique_ptr<CUnit> Create(const CStrW& actorName, const entity_id_t id,
		const uint32_t seed, CObjectManager& objectManager);

	~CUnit();

	// get unit's template object
	const CObjectEntry& GetObject() const { return *m_Object; }
	// get unit's model data
	CModelAbstract& GetModel() const { return *m_Model; }

	CUnitAnimation* GetAnimation() { return m_Animation; }

	/**
	 * Update the model's animation.
	 * @param frameTime time in seconds
	 */
	void UpdateModel(float frameTime);

	// Sets the entity-selection, and updates the unit to use the new
	// actor variation. Either set one key at a time, or a complete map.
	void SetEntitySelection(const CStr& key, const CStr& selection);
	void SetEntitySelection(const std::map<CStr, CStr>& selections);

	// Most units have a hopefully-unique ID number, so they can be referred to
	// persistently despite saving/loading maps. Default for new units is -1; should
	// usually be set to CUnitManager::GetNewID() after creation.
	entity_id_t GetID() const { return m_ID; }

	const std::set<CStr>& GetActorSelections() const { return m_ActorSelections; }

	/**
	 * Overwrite the seed-selected actor selections. Likely only useful for Atlas or debugging.
	 */
	void SetActorSelections(const std::set<CStr>& selections);

private:
	// Actor for the unit
	const CActorDef& m_Actor;
	// object from which unit was created; never NULL once fully created.
	CObjectEntry* m_Object = nullptr;
	// object model representation; never nullptr once fully created.
	std::unique_ptr<CModelAbstract> m_Model;

	CUnitAnimation* m_Animation = nullptr;

	// unique (per map) ID number for units created in the editor, as a
	// permanent way of referencing them.
	entity_id_t m_ID;

	// seed used when creating unit
	uint32_t m_Seed;

	// Actor-level selections for this unit. This is normally set at init time,
	// so that we always re-use the same aesthetic variants.
	// These have lower priority than entity-level selections.
	std::set<CStr> m_ActorSelections;
	// Entity-level selections for this unit (used for e.g. animation variants).
	std::map<CStr, CStr> m_EntitySelections;

	// object manager which looks after this unit's objectentry
	CObjectManager& m_ObjectManager;

	void ReloadObject();

	friend class CUnitAnimation;
};

#endif
