
#include "precompiled.h"

#include "EntityManager.h"
#include "EntityTemplateCollection.h"
#include "ps/ConfigDB.h"
#include "ps/Profile.h"
#include "graphics/Terrain.h"
#include "ps/Game.h"
#include "maths/MathUtil.h"
#include "Entity.h"

int SELECTION_CIRCLE_POINTS;
int SELECTION_BOX_POINTS;
int SELECTION_SMOOTHNESS_UNIFIED = 9;

CEntityManager::CEntityManager()
: m_collisionPatches(0)
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
}


void CEntityManager::deleteAllHelper()
{
	for( int i = 0; i < MAX_HANDLES; i++ )
	{
		if( m_entities[i].m_refcount )
		{
			delete( m_entities[i].m_entity );
			m_entities[i].m_entity = 0;
			m_entities[i].m_refcount = 0;
		}
	}
}

bool CEntityManager::isEntityRefd(int index)
{
	return m_entities[index].m_refcount && !m_entities[index].m_entity->entf_get(ENTF_DESTROYED);
}


CEntityManager::~CEntityManager()
{	
	m_extant = false;
	deleteAllHelper();

	// Delete entities that were killed, but not yet reaped by a call to updateAll,
	// to avoid memory leak warnings upon exiting
	std::vector<CEntity*>::iterator it;
	for( it = m_reaper.begin(); it < m_reaper.end(); it++ )
		delete( *it );
	m_reaper.clear();

	delete[] m_collisionPatches;
	m_collisionPatches = 0;
}

void CEntityManager::deleteAll()
{
	m_extant = false;
	deleteAllHelper();
	m_nextalloc = 0;
	m_extant = true;
}

HEntity CEntityManager::create( CEntityTemplate* base, CVector3D position, float orientation, const std::set<CStr8>& actorSelections,
								const CStrW* building)
{
	debug_assert( base );
	if( !base )
		return HEntity();

	while( m_entities[m_nextalloc].m_refcount )
	{
		m_nextalloc++;
		if(m_nextalloc >= MAX_HANDLES)
		{
			debug_warn("Ran out of entity handles!");
			return HEntity();
		}
	}

	m_entities[m_nextalloc].m_entity = new CEntity( base, position, orientation, actorSelections, building );
	if( m_collisionPatches)
		m_entities[m_nextalloc].m_entity->updateCollisionPatch();
	m_entities[m_nextalloc].m_entity->me = HEntity( m_nextalloc );
	return( HEntity( m_nextalloc++ ) );
}

HEntity CEntityManager::create( const CStrW& templateName, CPlayer* player, CVector3D position, float orientation, const CStrW* building )
{
	CEntityTemplate* base = g_EntityTemplateCollection.getTemplate( templateName, player );
	debug_assert( base );
	if( !base )
		return HEntity();

	std::set<CStr8> selections;

	return create( base, position, orientation, selections, building );
}

HEntity CEntityManager::createFoundation( const CStrW& templateName, CPlayer* player, CVector3D position, float orientation )
{
	CEntityTemplate* base = g_EntityTemplateCollection.getTemplate( templateName, player );
	debug_assert( base );
	if( !base )
		return HEntity();

	std::set<CStr8> selections;

	if( base->m_foundation == L"" )
		return create( base, position, orientation, selections );	// Entity has no foundation, so just create it

	// Else, place the foundation object, telling it to convert into the right template when built.
	CEntityTemplate* foundation = g_EntityTemplateCollection.getTemplate( base->m_foundation );
	return create( foundation, position, orientation, selections, &templateName );
}

HEntity* CEntityManager::getByHandle( u16 index )
{
	if( index >= MAX_HANDLES ) return( NULL );
	if( !m_entities[index].m_refcount ) return( NULL );
	return( new HEntity( index ) );
}
CHandle *CEntityManager::getHandle( int index )
{
	if (!m_entities[index].m_refcount )
		return NULL;
	return &m_entities[index];
}

std::vector<HEntity>* CEntityManager::matches( EntityPredicate predicate, void* userdata )
{
	std::vector<HEntity>* matchlist = new std::vector<HEntity>;
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( isEntityRefd(i) )
			if( predicate( m_entities[i].m_entity, userdata ) )
				matchlist->push_back( HEntity( i ) );
	return( matchlist );
}

std::vector<HEntity>* CEntityManager::getExtant()
{
	std::vector<HEntity>* activelist = new std::vector<HEntity>;
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( isEntityRefd(i) )
			activelist->push_back( HEntity( i ) );
	return( activelist );
}

void CEntityManager::GetExtant( std::vector<CEntity*>& results )
{
	results.clear();
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( isEntityRefd(i) && m_entities[i].m_entity->m_extant )
			results.push_back( m_entities[i].m_entity );
}

void CEntityManager::GetInRange( float x, float z, float radius, std::vector<CEntity*>& results )
{
	results.clear();

	int cx = (int) ( x / COLLISION_PATCH_SIZE );
	int cz = (int) ( z / COLLISION_PATCH_SIZE );

	int r = (int) ( radius / COLLISION_PATCH_SIZE + 1 );

	int minX = MAX(cx-r, 0);
	int minZ = MAX(cz-r, 0);
	int maxX = MIN(cx+r, m_collisionPatchesPerSide-1);
	int maxZ = MIN(cz+r, m_collisionPatchesPerSide-1);
		
	for( int px = minX; px <= maxX; px++ ) 
	{
		for( int pz = minZ; pz <= maxZ; pz++ ) 
		{
			std::vector<CEntity*>& vec = m_collisionPatches[ px * m_collisionPatchesPerSide + pz ];
			for( std::vector<CEntity*>::iterator it = vec.begin(); it != vec.end(); it++ )
			{
				CEntity* e = *it;
				float dx = x - e->m_position.X;
				float dz = z - e->m_position.Z;
				if(dx*dx + dz*dz <= radius*radius)
				{
					results.push_back( e );
				}
			}
		}
	}
}

/*
void CEntityManager::dispatchAll( CMessage* msg )
{
	for( int i = 0; i < MAX_HANDLES; i++ )
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

	m_collisionPatches = new std::vector<CEntity*>[m_collisionPatchesPerSide * m_collisionPatchesPerSide];

	for( int i = 0; i < MAX_HANDLES; i++ )
	{
		if( isEntityRefd(i) )
		{
			// [2006-06-26 2780ms total]
			CEntity* e = m_entities[i].m_entity;
			e->Initialize();

			// [2006-06-26 8ms total]
			e->updateCollisionPatch();
		}
	}
}

void CEntityManager::TickAll()
{
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( isEntityRefd(i) && m_entities[i].m_entity->m_extant )
			m_entities[i].m_entity->Tick();
}

void CEntityManager::updateAll( size_t timestep )
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
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( isEntityRefd(i) )
			m_entities[i].m_entity->update( timestep );
	PROFILE_END( "update all" );
}

void CEntityManager::interpolateAll( float relativeoffset )
{
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( isEntityRefd(i) )
			m_entities[i].m_entity->interpolate( relativeoffset );
}

void CEntityManager::renderAll()
{
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( isEntityRefd(i) )
			m_entities[i].m_entity->render();
}
void CEntityManager::conformAll()
{
	PROFILE_START("conform all");
	for ( int i=0; i < MAX_HANDLES; i++ )
	{
		if( isEntityRefd(i) )
		{
			CEntity* entity = m_entities[i].m_entity;
			CVector2D targetXZ = g_Game->GetWorld()->GetTerrain()->getSlopeAngleFace( entity->m_position.X, entity->m_position.Z, entity );
	
			while( targetXZ.x > PI ) targetXZ.x -= 2 * PI;
			while( targetXZ.x < -PI ) targetXZ.x += 2 * PI;
			while( targetXZ.y > PI ) targetXZ.y -= 2 * PI;
			while( targetXZ.y < -PI ) targetXZ.y += 2 * PI;
	
			entity->m_orientation.X = clamp( targetXZ.x, -entity->m_base->m_anchorConformX, entity->m_base->m_anchorConformX );
			entity->m_orientation.Z = clamp( targetXZ.y, -entity->m_base->m_anchorConformZ, entity->m_base->m_anchorConformZ );
			entity->m_orientation_unclamped.x = targetXZ.x;
			entity->m_orientation_unclamped.y = targetXZ.y;
			entity->updateActorTransforms();
		}
	}
	PROFILE_END("conform all");
}

void CEntityManager::invalidateAll()
{
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( isEntityRefd(i) )
			m_entities[i].m_entity->invalidateActor();
}

void CEntityManager::destroy( u16 handle )
{
	m_reaper.push_back( m_entities[handle].m_entity );
	m_entities[handle].m_entity->me.m_handle = INVALID_HANDLE;
}

bool CEntityManager::m_extant = false;

std::vector<CEntity*>* CEntityManager::getCollisionPatch( CEntity* e ) 
{
	if( !e->m_extant )
	{
		return 0;
	}

	int ix = (int) ( e->m_position.X / COLLISION_PATCH_SIZE );
	int iz = (int) ( e->m_position.Z / COLLISION_PATCH_SIZE );
	return &m_collisionPatches[ ix * m_collisionPatchesPerSide + iz ];
}
