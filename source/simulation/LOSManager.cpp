#include "precompiled.h"

#include "LOSManager.h"

#include "ps/Game.h"
#include "ps/Player.h"
#include "graphics/Terrain.h"
#include "Entity.h"
#include "EntityManager.h"
#include "EntityTemplate.h"
#include "graphics/Unit.h"
#include "maths/Bound.h"
#include "graphics/Model.h"
#include "lib/allocators.h"
#include "lib/timer.h"

using namespace std;


CLOSManager::CLOSManager() : m_LOSSetting(0)
{
#ifdef _2_los
	m_Explored = 0;
	m_Visible  = 0;
#else
	m_VisibilityMatrix = 0;
#endif
}

CLOSManager::~CLOSManager() 
{
#ifdef _2_los
	matrix_free((void**)m_Explored);
	m_Explored = 0;
	matrix_free((void**)m_Visible);
	m_Visible = 0;
#else
	matrix_free((void**)m_VisibilityMatrix);
	m_VisibilityMatrix = 0;
#endif
}

void CLOSManager::Initialize(uint losSetting) 
{
	// Set special LOS setting
	m_LOSSetting = losSetting;

	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
	m_TilesPerSide = terrain->GetVerticesPerSide() - 1;
	m_TilesPerSide_1 = m_TilesPerSide-1;


	// Create the LOS data arrays
#ifdef _2_los
	m_Explored = (int**)matrix_alloc(m_TilesPerSide, m_TilesPerSide, sizeof(int));
	m_Visible  = (int**)matrix_alloc(m_TilesPerSide, m_TilesPerSide, sizeof(int));
#else
	m_VisibilityMatrix = (u16**)matrix_alloc(m_TilesPerSide, m_TilesPerSide, sizeof(u16));
#endif

	// TODO: This memory should be freed somewhere when the engine supports 
	// multiple sessions without restarting the program.
	// JW: currently free it in the dtor

	// Set initial values
#ifdef _2_los
	uint explored_value = (m_LOSSetting == EXPLORED || m_LOSSetting == ALL_VISIBLE)? 0xFF : 0;
	uint vis_value      = (m_LOSSetting == ALL_VISIBLE)? 0xFF : 0;
#else
	u16 vis_value = 0;
	if(m_LOSSetting == EXPLORED || m_LOSSetting == ALL_VISIBLE)
		for(int i = 0; i < 8; i++) vis_value |= LOS_EXPLORED << (i*2);
	if(m_LOSSetting == ALL_VISIBLE)
		for(int i = 0; i < 8; i++) vis_value |= LOS_VISIBLE << (i*2);
#endif
	for(uint x=0; x<m_TilesPerSide; x++)
	{
#ifdef _2_los
		memset(m_Explored[x], explored_value, m_TilesPerSide*sizeof(int));
		memset(m_Visible [x], vis_value     , m_TilesPerSide*sizeof(int));
#else
		for(uint y=0; y<m_TilesPerSide; y++)
		for(uint x=0; x<m_TilesPerSide; x++)
			m_VisibilityMatrix[y][x] = vis_value;
#endif
	}

	// Just Update() to set the visible array and also mark currently visible tiles as explored.
	// NOTE: this will have to be changed if we decide to use incremental LOS
	Update();
}

// NOTE: this will have to be changed if we decide to use incremental LOS
void CLOSManager::Update() 
{
	if(m_LOSSetting == ALL_VISIBLE)
		return;

	// Clear the visible array
#ifdef _2_los
	for(int x=0; x<m_TilesPerSide; x++)
	{
		memset(m_Visible[x], 0, m_TilesPerSide*sizeof(int));
	}
#else
	u16 not_all_vis = 0xFFFF;
	for(int i = 0; i < 8; i++)
		not_all_vis &= ~(LOS_VISIBLE << (i*2));
	for(uint y=0; y<m_TilesPerSide; y++)
	for(uint x=0; x<m_TilesPerSide; x++)
		m_VisibilityMatrix[y][x] &= not_all_vis;
#endif

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

#ifdef _2_los
		int mask = (1 << e->GetPlayer()->GetPlayerID());
#else
		uint shift = e->GetPlayer()->GetPlayerID()*2;
#endif

		int cx, cz;
		CTerrain::CalcFromPosition(e->m_position.X, e->m_position.Z, cx, cz);

		int minX = MAX(cx-los, 0);
		int minZ = MAX(cz-los, 0);
		int maxX = MIN(cx+los, (int)m_TilesPerSide_1);
		int maxZ = MIN(cz+los, (int)m_TilesPerSide_1);

		for(int x=minX; x<=maxX; x++) 
		{
			for(int z=minZ; z<=maxZ; z++) 
			{
				if((x-cx)*(x-cx) + (z-cz)*(z-cz) <= los*los) 
				{
#ifdef _2_los
					m_Visible[x][z] |= mask;
					m_Explored[x][z] |= mask;
#else
					m_VisibilityMatrix[x][z] |= (LOS_EXPLORED|LOS_VISIBLE) << shift;
#endif
				}
			}
		}
	}
}


uint LOS_GetTokenFor(uint player_id)
{
#ifdef _2_los
	return 1u << player_id;
#else
	return player_id*2;
#endif
}

//TIMER_ADD_CLIENT(tc_getstatus);

ELOSStatus CLOSManager::GetStatus(int tx, int tz, CPlayer* player) 
{
//TIMER_ACCRUE(tc_getstatus);

	// Ensure that units off the map don't cause the visibility arrays to be
	// accessed out of bounds
	if ((unsigned)tx >= m_TilesPerSide || (unsigned)tz >= m_TilesPerSide)
		return LOS_VISIBLE; // because we don't want them to be permanently hidden

	// TODO: Make the mask depend on the player's diplomacy (just OR all his allies' masks)

#ifdef _2_los

	const int mask = player->GetLOSToken();
	if((m_Visible[tx][tz] & mask) || m_LOSSetting == ALL_VISIBLE) 
	{
		return LOS_VISIBLE;
	}
	else if((m_Explored[tx][tz] & mask) || m_LOSSetting == EXPLORED)
	{
		return LOS_EXPLORED;
	}
	else
	{
		return LOS_UNEXPLORED;
	}
#else
	const uint shift = player->GetLOSToken();
	return (ELOSStatus)((m_VisibilityMatrix[tx][tz] >> shift) & 3);
#endif
}


ELOSStatus CLOSManager::GetStatus(float fx, float fz, CPlayer* player) 
{
	int ix, iz;
	CTerrain::CalcFromPosition(fx, fz, ix, iz);
	return GetStatus(ix, iz, player);
}

EUnitLOSStatus CLOSManager::GetUnitStatus(CUnit* unit, CPlayer* player)
{
	CVector3D centre;

	// For entities, we must use the simulation position so that we stay synchronised
	// (because the output of this function will presumably affect AI)
	CEntity* entity = unit->GetEntity();
	if (entity)
		centre = entity->m_position;
	else
		centre = unit->GetModel()->GetTransform().GetTranslation();

	ELOSStatus status = GetStatus(centre.X, centre.Z, player);

	if(status & LOS_VISIBLE)
		return UNIT_VISIBLE;

	if(status & LOS_EXPLORED)
	{
		if(!entity || entity->m_base->m_visionPermanent)
		{
			// both actors (which are usually for decoration) and units with the 
			// permanent flag should be remembered
			return UNIT_REMEMBERED;

			// TODO: the unit status system will have to be replaced with a "ghost actor"
			// system so that we can't remember units that we haven't seen and so we can 
			// see permanent units that have died but that we haven't been near lately
		}
	}

	return UNIT_HIDDEN;
}

EUnitLOSStatus CLOSManager::GetUnitStatus(CEntity* entity, CPlayer* player)
{
	return GetUnitStatus( entity->m_actor, player );
}

