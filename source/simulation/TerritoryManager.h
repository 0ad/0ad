// TerritoryManager.h
//
// Matei Zaharia matei@sprint.ca / matei@wildfiregames.com
// 
// Calculates territory boundaries and maintains territory data.
//
// Usage: 


#ifndef TERRITORY_MANAGER_INCLUDED
#define TERRITORY_MANAGER_INCLUDED

#include "ps/Singleton.h"
#include "ps/Game.h"
#include "ps/World.h"
#include "ps/Player.h"
#include "ps/Vector2D.h"
#include "EntityHandles.h"

class CUnit;
class CPlayer;

class CTerritory
{
public:
	CPlayer* owner;						// owner of the territory, or Gaia for none
	CEntity* centre;						// centre object of this territory
	std::vector<CVector2D> boundary;		// boundary polygon, in map coordinates

	CTerritory(CPlayer* owner_, CEntity* centre_, std::vector<CVector2D> boundary_)
		: owner(owner_), centre(centre_), boundary(boundary_) {}
};

class CTerritoryManager
{
	std::vector<CTerritory*> m_Territories;
	CTerritory*** m_TerritoryMatrix;	// m_TerritoryMatrix[x][z] points to the territory for tile (x, z)

	uint m_TilesPerSide;

public:
	CTerritoryManager();
	~CTerritoryManager();

	void Initialize();	// initialize, called after the game is fully loaded
	void Recalculate();	// recalculate the territory boundaries

	CTerritory* GetTerritory(int x, int z);			// get the territory to which the given tile belongs
	CTerritory* GetTerritory(float x, float z);		// get the territory to which the given world-space point belongs

	std::vector<CTerritory*>& GetTerritories() { return m_Territories; }
private:
	void CalculateBoundary( std::vector<CEntity*>& centres, size_t index, std::vector<CVector2D>& boundary );
};

#endif
