#ifndef _UNITMANAGER_H
#define _UNITMANAGER_H

#include <vector>
#include "Unit.h"

class CUnitManager 
{
public:
	CUnitManager() {}

	void AddUnit(CUnit* unit);
	void RemoveUnit(CUnit* unit);

	void DeleteAll();

	const std::vector<CUnit*>& GetUnits() const { return m_Units; }

private:
	std::vector<CUnit*> m_Units;
};

extern CUnitManager g_UnitMan;

#endif