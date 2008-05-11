/**
 * =========================================================================
 * File        : UnitManager.h
 * Project     : 0 A.D.
 * Description : Container that owns all units
 * =========================================================================
 */

#ifndef INCLUDED_UNITMANAGER
#define INCLUDED_UNITMANAGER

#include <vector>
#include <set>

class CUnit;
class CModel;
class CVector3D;
class CEntity;
class CStr;
class CObjectManager;

///////////////////////////////////////////////////////////////////////////////
// CUnitManager: simple container class holding all units within the world
class CUnitManager
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

	CUnit* FindByID(size_t id) const;

	size_t GetNewID() { return m_NextID++; }

	void SetNextID(size_t n) { m_NextID = n; }

	void SetObjectManager(CObjectManager& objectManager) { m_ObjectManager = &objectManager; }

private:
	// list of all known units
	std::vector<CUnit*> m_Units;
	// next ID number to be assigned to a unit created in the editor
	size_t m_NextID;
	// graphical object manager; may be NULL if not set up
	CObjectManager* m_ObjectManager;
};

#endif
