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
#include "Unit.h"
#include "Singleton.h"

class CVector3D;

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
	// remove and delete all units
	void DeleteAll();

	// return the units
	const std::vector<CUnit*>& GetUnits() const { return m_Units; }
	
	// iterate through units testing given ray against bounds of each unit; 
	// return the closest unit, or null if everything missed
	CUnit* PickUnit(const CVector3D& origin,const CVector3D& dir) const;

private:
	// list of all known units
	std::vector<CUnit*> m_Units;
};

#endif