/* Copyright (C) 2009 Wildfire Games.
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

//This keeps track of all the formations exisisting in the game.

#ifndef INCLUDED_FORMATIONMANAGER
#define INCLUDED_FORMATIONMANAGER

#include "ps/Singleton.h"
#include "scripting/DOMEvent.h"

#define g_FormationManager CFormationManager::GetSingleton()

class CEntity;
class CStr;
class CFormation;
class CVector2D;
class CEntityFormation;

struct CEntityList;

class CFormationManager : public Singleton<CFormationManager>
{
#define FormIterator std::vector<CEntityFormation*>::iterator

public:
	CFormationManager() {}
	~CFormationManager();
	void CreateFormation( CEntityList& entities, CStrW& name );
	//entity is any unit in the formation
	void DestroyFormation( size_t form );
	inline bool IsValidFormation( size_t index )
	{
		return ((size_t)index < m_formations.size() && index >= 0);
	}
	bool AddUnit( CEntity* entity, size_t& form );
	CEntityList AddUnitList( CEntityList& entities, size_t form );

	//Returns false if the formation is destroyed
	bool RemoveUnit( CEntity* entity );
	bool RemoveUnitList( CEntityList& entities );
	CEntityFormation* GetFormation(size_t form);
	void UpdateIndexes( size_t update );

private:
	std::vector<CEntityFormation*> m_formations;
};

#endif // INCLUDED_FORMATIONMANAGER
