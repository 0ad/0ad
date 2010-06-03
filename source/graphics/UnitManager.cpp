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

/*
 * Container that owns all units
 */

#include "precompiled.h"

#include <float.h>

#include "Model.h"
#include "UnitManager.h"
#include "Unit.h"
#include "ObjectManager.h"
#include "ObjectEntry.h"
#include "ps/Game.h"
#include "ps/World.h"

#include <algorithm>

///////////////////////////////////////////////////////////////////////////////
// CUnitManager constructor
CUnitManager::CUnitManager()
{
}

///////////////////////////////////////////////////////////////////////////////
// CUnitManager destructor
CUnitManager::~CUnitManager()
{
	DeleteAll();
}


///////////////////////////////////////////////////////////////////////////////
// AddUnit: add given unit to world
void CUnitManager::AddUnit(CUnit* unit)
{
	m_Units.push_back(unit);
}

///////////////////////////////////////////////////////////////////////////////
// RemoveUnit: remove given unit from world, but don't delete it
void CUnitManager::RemoveUnit(CUnit* unit)
{
	// find entry in list
	typedef std::vector<CUnit*>::iterator Iter;
	Iter i=std::find(m_Units.begin(),m_Units.end(),unit);
	if (i!=m_Units.end()) {
		m_Units.erase(i);
	}
}

///////////////////////////////////////////////////////////////////////////////
// DeleteUnit: remove given unit from world and delete it
void CUnitManager::DeleteUnit(CUnit* unit)
{
	RemoveUnit(unit);
	delete unit;
}

///////////////////////////////////////////////////////////////////////////////
// DeleteAll: remove and delete all units
void CUnitManager::DeleteAll()
{
	for (size_t i=0;i<m_Units.size();i++) {
		delete m_Units[i];
	}
	m_Units.clear();
}


///////////////////////////////////////////////////////////////////////////////
// PickUnit: iterate through units testing given ray against bounds of each 
// unit; return the closest unit, or null if everything missed
CUnit* CUnitManager::PickUnit(const CVector3D& origin, const CVector3D& dir, bool entitiesOnly) const
{
	// closest object found so far
	CUnit* hit = 0;
	// distance to closest object found so far
	float dist = FLT_MAX;
	// closest approach offset (easier to pick small stuff in forests than standard ScEd style selection)
	float minrel = FLT_MAX;

	for (size_t i=0; i<m_Units.size(); i++) {
		CUnit* unit = m_Units[i];
		float tmin, tmax;
		
		if (unit->GetModel().GetBounds().RayIntersect(origin, dir, tmin, tmax))
		{
			// Point of closest approach
			CVector3D obj;
			unit->GetModel().GetBounds().GetCentre(obj);
			CVector3D delta = obj - origin;
			float distance = delta.Dot(dir);
			CVector3D closest = origin + dir * distance;
			CVector3D offset = obj - closest;

			float rel = offset.Length();
			if (rel < minrel) {
				hit = unit;
				dist = tmin;
				minrel = rel;
			}
		}
	}
	return hit;
}

///////////////////////////////////////////////////////////////////////////////
// CreateUnit: create a new unit and add it to the world
CUnit* CUnitManager::CreateUnit(const CStrW& actorName, CEntity* entity, const std::set<CStr>& selections)
{
	if (! m_ObjectManager)
		return NULL;

	CUnit* unit = CUnit::Create(actorName, entity, selections, *m_ObjectManager);
	if (unit)
		AddUnit(unit);
	return unit;
}
