#include "res/res.h"
#include "UnitManager.h"
#include <algorithm>

CUnitManager g_UnitMan;

void CUnitManager::AddUnit(CUnit* unit)
{
	m_Units.push_back(unit);
}

void CUnitManager::RemoveUnit(CUnit* unit)
{
	// find entry in list
	typedef std::vector<CUnit*>::iterator Iter;
	Iter i=std::find(m_Units.begin(),m_Units.end(),unit);
	if (i!=m_Units.end()) {
		m_Units.erase(i);
	}
}

void CUnitManager::DeleteAll()
{
	for (uint i=0;i<m_Units.size();i++) {
		delete m_Units[i];
	}
	m_Units.clear();
}
