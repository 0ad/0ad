///////////////////////////////////////////////////////////////////////////////
//
// Name:		UnitManager.cpp
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#include "precompiled.h"

#include "res/res.h"
#include "Model.h"
#include "UnitManager.h"
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
	for (uint i=0;i<m_Units.size();i++) {
		delete m_Units[i];
	}
	m_Units.clear();
}

///////////////////////////////////////////////////////////////////////////////
// PickUnit: iterate through units testing given ray against bounds of each 
// unit; return the closest unit, or null if everything missed
CUnit* CUnitManager::PickUnit(const CVector3D& origin,const CVector3D& dir) const
{
	// closest object found so far
	CUnit* hit = 0;
	// distance to closest object found so far
	float dist = 1.0e30f;
	// closest approach offset (easier to pick small stuff in forests than standard ScEd style selection)
	float minrel = 1.0e30f;

	for (uint i=0;i<m_Units.size();i++) {
		CUnit* unit=m_Units[i];
		float tmin,tmax;
		
		if (unit->GetModel()->GetBounds().RayIntersect(origin,dir,tmin,tmax))
		{
			// Point of closest approach
			CVector3D obj;
			unit->GetModel()->GetBounds().GetCentre( obj );
			CVector3D delta = obj - origin;
			float distance = delta.Dot( dir );
			CVector3D closest = origin + dir * distance;
			CVector3D offset = obj - closest;

			float rel = offset.GetLength();
			if (!hit || rel < minrel ) {
				hit=unit;
				dist=tmin;
				minrel = rel;
			}
		}
	}
	return hit;
}
