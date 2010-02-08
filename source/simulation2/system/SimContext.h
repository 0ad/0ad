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

#ifndef INCLUDED_SIMCONTEXT
#define INCLUDED_SIMCONTEXT

class CComponentManager;
class CUnitManager;
class CTerrain;

/**
 * Contains pointers to various 'global' objects that are needed by the simulation code,
 * to allow easy access without using real (evil) global variables.
 */
class CSimContext
{
public:
	CSimContext();
	~CSimContext();

	CComponentManager& GetComponentManager() const;
	void SetComponentManager(CComponentManager* man);

	bool HasUnitManager() const;
	CUnitManager& GetUnitManager() const;

	CTerrain& GetTerrain() const;

private:
	CComponentManager* m_ComponentManager;
	CUnitManager* m_UnitManager;
	CTerrain* m_Terrain;

	friend class CSimulation2Impl;
};

#endif // INCLUDED_SIMCONTEXT
