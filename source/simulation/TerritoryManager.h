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

// TerritoryManager.h
// 
// Calculates territory boundaries and maintains territory data.
//
// Usage: 


#ifndef INCLUDED_TERRITORYMANAGER
#define INCLUDED_TERRITORYMANAGER

#include "ps/Vector2D.h"
#include "maths/Vector3D.h"
#include "EntityHandles.h"

class CUnit;
class CPlayer;

const float TERRITORY_PRECISION_STEP = 1.0f;

class CTerritory
{
public:
	CPlayer* owner;						// owner of the territory, or Gaia for none
	HEntity centre;						// centre object of this territory
	std::vector<CVector2D> boundary;	// boundary polygon, in map coordinates

private:
	// cached coordinates for the polygon's edge segments (conformed to the terrain)
	std::vector<std::vector<CVector3D> > edgeCoords;

public:
	CTerritory(CPlayer* owner_, HEntity centre_, std::vector<CVector2D> boundary_)
		: owner(owner_), centre(centre_), boundary(boundary_) {}

	const std::vector<CVector3D>& GetEdgeCoords(size_t edge);

	void ClearEdgeCache();
};

class CTerritoryManager
{
	std::vector<CTerritory*> m_Territories;
	CTerritory*** m_TerritoryMatrix;	// m_TerritoryMatrix[x][z] points to the territory for tile (x, z)

	size_t m_TilesPerSide;
	bool m_DelayedRecalculate;

public:
	CTerritoryManager();
	~CTerritoryManager();

	void Initialize();	// initialize, called after the game is fully loaded
	void Recalculate();	// recalculate the territory boundaries
	void DelayedRecalculate();	// recalculate the territory boundaries when next rendered
	void RenderTerritories();
	CTerritory* GetTerritory(int x, int z);			// get the territory to which the given tile belongs
	CTerritory* GetTerritory(float x, float z);		// get the territory to which the given world-space point belongs

	std::vector<CTerritory*>& GetTerritories() { return m_Territories; }

private:
	void CalculateBoundary( std::vector<CEntity*>& centres, size_t index, std::vector<CVector2D>& boundary );
};

#endif
