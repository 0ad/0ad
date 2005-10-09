#include "precompiled.h"

#include "LOSManager.h"

#include "Game.h"
#include "Player.h"
#include "Terrain.h"
#include "Entity.h"
#include "EntityManager.h"
#include "Unit.h"
#include "Bound.h"
#include "Model.h"

using namespace std;

CLOSManager::CLOSManager() 
{
	m_MapRevealed = false;
}

CLOSManager::~CLOSManager() 
{
}

void CLOSManager::Initialize() 
{
	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
	int tilesPerSide = terrain->GetVerticesPerSide() - 1;

	// Create the LOS data arrays

	m_Explored = new int*[tilesPerSide];
	for(int i=0; i<tilesPerSide; i++) 
	{
		m_Explored[i] = new int[tilesPerSide];
	}

	m_Visible = new int*[tilesPerSide];
	for(int i=0; i<tilesPerSide; i++) 
	{
		m_Visible[i] = new int[tilesPerSide];
	}

	// TODO: This memory should be freed somewhere when the engine supports 
	// multiple sessions without restarting the program.

	// Clear everything to unexplored
	for(int x=0; x<tilesPerSide; x++)
	{
		memset(m_Explored[x], 0, tilesPerSide*sizeof(int));
	}

	// Just Update() to set the visible array and also mark currently visible tiles as explored.
	// NOTE: this will have to be changed if we decide to use incremental LOS

	Update();
}

// NOTE: this will have to be changed if we decide to use incremental LOS
void CLOSManager::Update() 
{
	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
	int tilesPerSide = terrain->GetVerticesPerSide() - 1;

	// Clear the visible array
	for(int x=0; x<tilesPerSide; x++)
	{
		memset(m_Visible[x], 0, tilesPerSide*sizeof(int));
	}

	// Set visibility for each entity
	vector<CEntity*> extant;
	g_Game->GetWorld()->GetEntityManager()->GetExtant(extant);
	for(size_t i=0; i<extant.size(); i++) 
	{
		CEntity* e = extant[i];

		int los = e->m_los;
		if(los == 0)
		{
			continue;
		}

		int mask = (1 << e->GetPlayer()->GetPlayerID());

		int cx = min(int(e->m_position.X/CELL_SIZE), tilesPerSide-1);
		int cz = min(int(e->m_position.Z/CELL_SIZE), tilesPerSide-1);

		int minX = max(cx-los, 0);
		int minZ = max(cz-los, 0);
		int maxX = min(cx+los, tilesPerSide-1);
		int maxZ = min(cz+los, tilesPerSide-1);

		for(int x=minX; x<=maxX; x++) 
		{
			for(int z=minZ; z<=maxZ; z++) 
			{
				if((x-cx)*(x-cx) + (z-cz)*(z-cz) <= los*los) 
				{
					m_Visible[x][z] |= mask;
					m_Explored[x][z] |= mask;
				}
			}
		}
	}
}

ELOSStatus CLOSManager::GetStatus(int tx, int tz, CPlayer* player) 
{
	int mask = (1 << player->GetPlayerID());

	if(m_MapRevealed)
	{
		return LOS_VISIBLE;
	}
	else
	{
		if(m_Visible[tx][tz] & mask) 
		{
			return LOS_VISIBLE;
		}
		else if(m_Explored[tx][tz] & mask)
		{
			return LOS_EXPLORED;
		}
		else
		{
			return LOS_UNEXPLORED;
		}
	}
}

ELOSStatus CLOSManager::GetStatus(float fx, float fz, CPlayer* player) 
{
	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
	int tilesPerSide = terrain->GetVerticesPerSide() - 1;

	int ix = min(int(fx/CELL_SIZE), tilesPerSide-1);
	int iz = min(int(fz/CELL_SIZE), tilesPerSide-1);
	return GetStatus(ix, iz, player);
}

EUnitLOSStatus CLOSManager::GetUnitStatus(CUnit* unit, CPlayer* player)
{
	CVector3D centre;
	unit->GetModel()->GetBounds().GetCentre(centre);
	ELOSStatus status = GetStatus(centre.X, centre.Z, player);
	if(status == LOS_VISIBLE)
	{
		return UNIT_VISIBLE;
	}
	else if(status == LOS_EXPLORED)
	{
		if(unit->GetEntity() == 0 || unit->GetEntity()->m_permanent)
		{
			// both actors (which are usually for decoration) and units with the 
			// permanent flag should be remembered
			return UNIT_REMEMBERED;

			// TODO: the unit status system will have to be replaced with a "ghost actor"
			// system so that we can't remember units that we haven't seen and so we can 
			// see permanent units that have died but that we haven't been near lately
		}
		else
		{
			return UNIT_HIDDEN;
		}
	}
	else
	{
		return UNIT_HIDDEN;
	}
}


