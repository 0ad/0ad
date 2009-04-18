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


#include "precompiled.h"

#include "EntityManager.h"
#include "EntityTemplateCollection.h"
#include "EntityTemplate.h"
#include "ps/ConfigDB.h"
#include "ps/Player.h"
#include "ps/Profile.h"
#include "graphics/Terrain.h"
#include "ps/Game.h"
#include "maths/MathUtil.h"
#include "Entity.h"
#include "lib/timer.h"

#ifdef USE_DCDT
# include "dcdt/se/se_dcdt.h"
#endif // USE_DCDT

#include "PathfindEngine.h"
#include "ps/GameSetup/Config.h"

int AURA_CIRCLE_POINTS;
int SELECTION_CIRCLE_POINTS;
int SELECTION_BOX_POINTS;
int SELECTION_SMOOTHNESS_UNIFIED = 9;

CEntityManager::CEntityManager()
: m_collisionPatches(0)
, m_screenshotMode(false)
, m_entities()	// janwas: default-initialize entire array;
				// CHandle ctor sets m_entity and m_refcount to 0
{
	m_nextalloc = 0;
	m_extant = true;
	m_death = false;

	// Also load a couple of global entity settings
	CConfigValue* cfg = g_ConfigDB.GetValue( CFG_USER, "selection.outline.quality" );
	if( cfg ) cfg->GetInt( SELECTION_SMOOTHNESS_UNIFIED );
	if( SELECTION_SMOOTHNESS_UNIFIED < 0 ) SELECTION_SMOOTHNESS_UNIFIED = 0;
	SELECTION_CIRCLE_POINTS = 7 + 2 * SELECTION_SMOOTHNESS_UNIFIED;
	SELECTION_BOX_POINTS = 1 + SELECTION_SMOOTHNESS_UNIFIED;
	AURA_CIRCLE_POINTS = 7 + 3 * SELECTION_SMOOTHNESS_UNIFIED;
}


void CEntityManager::DeleteAllHelper()
{
	for( size_t i = 0; i < MAX_HANDLES; i++ )
	{
		if( m_entities[i].m_refcount )
		{
			delete( m_entities[i].m_entity );
			m_entities[i].m_entity = 0;
			m_entities[i].m_refcount = 0;
			m_refd[i] = false;
		}
	}
}

CEntityManager::~CEntityManager()
{	
	m_extant = false;
	DeleteAllHelper();

	// Delete entities that were killed, but not yet reaped by a call to UpdateAll,
	// to avoid memory leak warnings upon exiting
	std::vector<CEntity*>::iterator it;
	for( it = m_reaper.begin(); it < m_reaper.end(); it++ )
		delete( *it );
	m_reaper.clear();

	delete[] m_collisionPatches;
	m_collisionPatches = NULL;
}

void CEntityManager::DeleteAll()
{
	m_extant = false;
	
	DeleteAllHelper();
	
	m_nextalloc = 0;

	delete[] m_collisionPatches;
	m_collisionPatches = NULL;

	m_extant = true;
}

void CEntityManager::updateObstacle( CEntity* tempHandle )
{
	(void)tempHandle;

#ifdef USE_DCDT
	if(g_Pathfinder.dcdtInitialized)
	{
		SrPolygon poly;


		poly.size(0);	

		
		CVector2D p, q;
		CVector2D u, v;
		q.x = tempHandle->m_position.X;
		q.y = tempHandle->m_position.Z;
		float d = ((CBoundingBox*)tempHandle->m_bounds)->m_d;
		float w = ((CBoundingBox*)tempHandle->m_bounds)->m_w;

		u.x = sin( tempHandle->m_graphics_orientation.Y );
		u.y = cos( tempHandle->m_graphics_orientation.Y );
		v.x = u.y;
		v.y = -u.x;

		CBoundingObject* m_bounds = tempHandle->m_bounds;

		switch( m_bounds->m_type )
		{
			case CBoundingObject::BOUND_CIRCLE:
			{
				if(tempHandle->m_speed == 0)
				{
				
					poly.open(false);

					w = 0.5;
					d = 0.5;
				
					p = q + u * d + v * w;
					poly.push().set((float)(p.x), (float)(p.y));

					p = q - u * d + v * w ;
					poly.push().set((float)(p.x), (float)(p.y));

					p = q - u * d - v * w;
					poly.push().set((float)(p.x), (float)(p.y));

					p = q + u * d - v * w;
					poly.push().set((float)(p.x), (float)(p.y));

					int dcdtId = g_Pathfinder.dcdtPathfinder.insert_polygon(poly);
					tempHandle->m_dcdtId = dcdtId;

				
				}
				break;

			}
			case CBoundingObject::BOUND_OABB:
			{

				
				poly.open(false);
				
				// Tighten the bound so the units will not get stuck near the buildings
				//Note: the triangulation pathfinding code will not find a path for the unit if it is pushed into the bound of a unit.
				//
				w = w * 0.8;
				d = d * 0.8;
			
				p = q + u * d + v * w;
				poly.push().set((float)(p.x), (float)(p.y));

				p = q - u * d + v * w ;
				poly.push().set((float)(p.x), (float)(p.y));

				p = q - u * d - v * w;
				poly.push().set((float)(p.x), (float)(p.y));

				p = q + u * d - v * w;
				poly.push().set((float)(p.x), (float)(p.y));

				int dcdtId = g_Pathfinder.dcdtPathfinder.insert_polygon(poly);
				tempHandle->m_dcdtId = dcdtId;
				break;
			}

				

		}//end switch

		g_Pathfinder.dcdtPathfinder.DeleteAbstraction();
		g_Pathfinder.dcdtPathfinder.Abstract();

		if(g_ShowPathfindingOverlay)
		{
			g_Pathfinder.drawTriangulation();
		}
	}
#endif // USE_DCDT
}


HEntity CEntityManager::Create(CEntityTemplate* base, CVector3D position, float orientation,
	const std::set<CStr>& actorSelections, const CStrW* building)
{
	debug_assert( base );
	if( !base )
		return HEntity();

	// Find an unused handle for the unit
	size_t pos = 0;
	while( m_entities[m_nextalloc].m_refcount )
	{
		m_nextalloc++;
		if(m_nextalloc >= MAX_HANDLES)
		{
			debug_warn("Ran out of entity handles!");
			return HEntity();
		}
	}
	pos = m_nextalloc;
	m_nextalloc++;

	debug_assert(m_entities[pos].m_entity == 0);
	m_entities[pos].m_entity = new CEntity( base, position, orientation, actorSelections, building );
	if( m_collisionPatches)
		m_entities[pos].m_entity->UpdateCollisionPatch();
	m_entities[pos].m_entity->me = HEntity( pos );

	//Kai: invoking triangulation update for new objects
	updateObstacle(m_entities[pos].m_entity);

		
	
	return( HEntity( pos ) );
}

void CEntityManager::AddEntityClassData(const HEntity& handle)
{
	//Add data for this particular entity and player
	size_t playerID = handle->GetPlayer()->GetPlayerID();
	CStrW className, classList = handle->m_classes.GetMemberList();

	while ( (className = classList.BeforeFirst(L" ")) != classList )
	{
		if ( m_entityClassData[playerID].find(className) == m_entityClassData[playerID].end() )
			m_entityClassData[playerID][className] = 0;
		++m_entityClassData[playerID][className];
		classList = classList.AfterFirst(L" ");
	}

	//For last element
	if ( m_entityClassData[playerID].find(className) == m_entityClassData[playerID].end() )
		m_entityClassData[playerID][className] = 0;
	++m_entityClassData[playerID][className];
}

HEntity CEntityManager::Create( const CStrW& templateName, CPlayer* player, CVector3D position, float orientation, const CStrW* building )
{
	CEntityTemplate* base = g_EntityTemplateCollection.GetTemplate( templateName, player );
	debug_assert( base );
	if( !base )
		return HEntity();

	std::set<CStr> selections;

	HEntity ret = Create( base, position, orientation, selections, building );
	AddEntityClassData(ret);
	return ret;
}

HEntity CEntityManager::CreateFoundation( const CStrW& templateName, CPlayer* player, CVector3D position, float orientation )
{
	CEntityTemplate* base = g_EntityTemplateCollection.GetTemplate( templateName, player );
	debug_assert( base );
	if( !base )
		return HEntity();

	std::set<CStr> selections;

	if( base->m_foundation == L"" )
		return Create( base, position, orientation, selections );	// Entity has no foundation, so just create it

	// Else, place the foundation object, telling it to convert into the right template when built.
	CEntityTemplate* foundation = g_EntityTemplateCollection.GetTemplate( base->m_foundation );
	return Create( foundation, position, orientation, selections, &templateName );
}

HEntity* CEntityManager::GetByHandle( size_t index )
{
	if( index >= MAX_HANDLES ) return( NULL );
	if( !m_entities[index].m_refcount ) return( NULL );
	return( new HEntity( index ) );
}
CHandle *CEntityManager::GetHandle( size_t index )
{
	if (!m_entities[index].m_refcount )
		return NULL;
	return &m_entities[index];
}

void CEntityManager::GetMatchingAsHandles(std::vector<HEntity>& matchlist, EntityPredicate predicate, void* userdata)
{
	matchlist.clear();
	for( size_t i = 0; i < MAX_HANDLES; i++ )
	{
		if( IsEntityRefd(i) )
			if( predicate( m_entities[i].m_entity, userdata ) )
				matchlist.push_back( HEntity( i ) );
	}
}

void CEntityManager::GetExtantAsHandles( std::vector<HEntity>& results )
{
	results.clear();
	for( size_t i = 0; i < MAX_HANDLES; i++ )
		if( IsEntityRefd(i) )
			results.push_back( HEntity( i ) );
}

void CEntityManager::GetExtant( std::vector<CEntity*>& results )
{
	results.clear();
	for( size_t i = 0; i < MAX_HANDLES; i++ )
		if( IsEntityRefd(i) )
			results.push_back( m_entities[i].m_entity );
}

void CEntityManager::GetInRange( float x, float z, float radius, std::vector<CEntity*>& results )
{
	results.clear();

	float radiusSq = radius * radius;

	int cx = (int) ( x / COLLISION_PATCH_SIZE );
	int cz = (int) ( z / COLLISION_PATCH_SIZE );

	int r = (int) ( radius / COLLISION_PATCH_SIZE + 1 );

	int minX = std::max(cx-r, 0);
	int minZ = std::max(cz-r, 0);
	int maxX = std::min(cx+r, m_collisionPatchesPerSide-1);
	int maxZ = std::min(cz+r, m_collisionPatchesPerSide-1);
		
	for( int px = minX; px <= maxX; px++ ) 
	{
		for( int pz = minZ; pz <= maxZ; pz++ ) 
		{
			std::vector<CEntity*>& vec = m_collisionPatches[ px * m_collisionPatchesPerSide + pz ];
			for( size_t i=0; i<vec.size(); i++ )
			{
				CEntity* e = vec[i];
				debug_assert(e != 0);
				float dx = x - e->m_position.X;
				float dz = z - e->m_position.Z;
				if( dx*dx + dz*dz <= radiusSq )
				{
					results.push_back( e );
				}
			}
		}
	}
}

void CEntityManager::GetInLOS( CEntity* entity, std::vector<CEntity*>& results )
{
	GetInRange( entity->m_position.X, entity->m_position.Z, entity->m_los*CELL_SIZE, results );
}

/*
void CEntityManager::dispatchAll( CMessage* msg )
{
	for( size_t i = 0; i < MAX_HANDLES; i++ )
		if( m_entities[i].m_refcount && m_entities[i].m_entity->m_extant )
			m_entities[i].m_entity->dispatch( msg );
}
*/

TIMER_ADD_CLIENT(tc_1);
TIMER_ADD_CLIENT(tc_2);

void CEntityManager::InitializeAll()
{
	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
	int unitsPerSide = CELL_SIZE * ( terrain->GetVerticesPerSide() - 1 );
	m_collisionPatchesPerSide = unitsPerSide / COLLISION_PATCH_SIZE + 1;

	debug_assert(! m_collisionPatches);
	m_collisionPatches = new std::vector<CEntity*>[m_collisionPatchesPerSide * m_collisionPatchesPerSide];

	for( size_t i = 0; i < MAX_HANDLES; i++ )
	{
		if( IsEntityRefd(i) )
		{
			// [2006-06-26 2780ms total]
			CEntity* e = m_entities[i].m_entity;
			e->Initialize();

			// [2006-06-26 8ms total]
			e->UpdateCollisionPatch();
		}
	}
}

/*
void CEntityManager::TickAll()
{
	for( size_t i = 0; i < MAX_HANDLES; i++ )
		if( IsEntityRefd(i) && m_entities[i].m_entity->m_extant )
			m_entities[i].m_entity->Tick();
}
*/

void CEntityManager::UpdateAll( int timestep )
{
	PROFILE_START( "reaper" );
	std::vector<CEntity*>::iterator it;
	for( it = m_reaper.begin(); it < m_reaper.end(); it++ )
		delete( *it );
	m_reaper.clear();
	PROFILE_END( "reaper" );

	// PT: TickAll (which sends the 'Tick' event to all entities) has been
	// disabled, because:
	// * it's very slow (particularly when there are thousands of entities, e.g. trees);
	// * no entity currently responds to tick events;
	// * nobody can think of a situation where ticks would be required in the future;
	// * if they ever are needed, they can be done more efficiently (e.g. by
	//   adding a per-entity 'wants tick' flag);
	// * it's very slow.
/*
	PROFILE_START( "tick all" );
	TickAll();
	PROFILE_END( "tick all" );
*/

	PROFILE_START( "update all" );
	for( size_t i = 0; i < MAX_HANDLES; i++ )
		if( IsEntityRefd(i) )
			m_entities[i].m_entity->Update( timestep );
	PROFILE_END( "update all" );
}

void CEntityManager::InterpolateAll( float relativeoffset )
{
	for( size_t i = 0; i < MAX_HANDLES; i++ )
		// This needs to handle all entities, including destroyed/non-extant ones
		// (mainly dead bodies), so it can't use IsEntityRefd
		if( m_entities[i].m_refcount && m_entities[i].m_entity )
			m_entities[i].m_entity->Interpolate( relativeoffset );
}

void CEntityManager::RenderAll()
{
	for( size_t i = 0; i < MAX_HANDLES; i++ )
		if( IsEntityRefd(i) )
			m_entities[i].m_entity->Render();
}

void CEntityManager::ConformAll()
{
	PROFILE_START("conform all");
	for ( size_t i=0; i < MAX_HANDLES; i++ )
	{
		if( IsEntityRefd(i) )
		{
			m_entities[i].m_entity->UpdateXZOrientation();
			m_entities[i].m_entity->UpdateActorTransforms();
		}
	}
	PROFILE_END("conform all");
}

void CEntityManager::InvalidateAll()
{
	for( size_t i = 0; i < MAX_HANDLES; i++ )
		if( IsEntityRefd(i) )
			m_entities[i].m_entity->InvalidateActor();
}


void CEntityManager::RemoveUnitCount(CEntity* ent)
{
	size_t playerID = ent->GetPlayer()->GetPlayerID();
	CStrW className, classList = ent->m_classes.GetMemberList();

	while ( (className = classList.BeforeFirst(L" ")) != classList )
	{
		--m_entityClassData[playerID][className];
		classList = classList.AfterFirst(L" ");
	}
	--m_entityClassData[playerID][className];
}
void CEntityManager::Destroy( size_t handle )
{
	m_reaper.push_back( m_entities[handle].m_entity );
}

bool CEntityManager::m_extant = false;

std::vector<CEntity*>* CEntityManager::GetCollisionPatch( CEntity* e ) 
{
	if( !e->m_extant )
	{
		return 0;
	}

	int ix = (int) ( e->m_position.X / COLLISION_PATCH_SIZE );
	int iz = (int) ( e->m_position.Z / COLLISION_PATCH_SIZE );
	return &m_collisionPatches[ ix * m_collisionPatchesPerSide + iz ];
}
