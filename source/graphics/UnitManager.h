///////////////////////////////////////////////////////////////////////////////
//
// Name:		UnitManager.h
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _UNITMANAGER_H
#define _UNITMANAGER_H

#include <vector>
#include <set>
#include "ps/Singleton.h"

class CUnit;
class CModel;
class CVector3D;
class CEntity;
class CStr;

// access to sole CUnitManager object
#define g_UnitMan CUnitManager::GetSingleton()

///////////////////////////////////////////////////////////////////////////////
// CUnitManager: simple container class holding all units within the world
class CUnitManager : public Singleton<CUnitManager>
{
public:
	// constructor, destructor
	CUnitManager();
	~CUnitManager();

	// add given unit to world
	void AddUnit(CUnit* unit);
	// remove given unit from world, but don't delete it
	void RemoveUnit(CUnit* unit);
	// remove given unit from world and delete it
	void DeleteUnit(CUnit* unit);
	// remove and delete all units
	void DeleteAll();

	// creates a new unit and adds it to the world
	CUnit* CreateUnit(const CStr& actorName, CEntity* entity, const std::set<CStr>& selections);

	// return the units
	const std::vector<CUnit*>& GetUnits() const { return m_Units; }
	
	// iterate through units testing given ray against bounds of each unit; 
	// return the closest unit, or null if everything missed
	CUnit* PickUnit(const CVector3D& origin, const CVector3D& dir, bool entitiesOnly) const;

	CUnit* FindByID(int id) const;

	int GetNewID() { return m_NextID++; }

	void SetNextID(int n) { m_NextID = n; }

private:
	// list of all known units
	std::vector<CUnit*> m_Units;
	// next ID number to be assigned to a unit created in the editor
	int m_NextID;
};

#endif
