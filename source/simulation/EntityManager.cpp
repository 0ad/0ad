
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
	m_refd.resize( MAX_HANDLES );

	// Also load a couple of global entity settings
	CConfigValue* cfg = g_ConfigDB.GetValue( CFG_USER, "selection.outline.quality" );
	if( cfg ) cfg->GetInt( SELECTION_SMOOTHNESS_UNIFIED );
	if( SELECTION_SMOOTHNESS_UNIFIED < 0 ) SELECTION_SMOOTHNESS_UNIFIED = 0;
	SELECTION_CIRCLE_POINTS = 7 + 2 * SELECTION_SMOOTHNESS_UNIFIED;
	SELECTION_BOX_POINTS = 1 + SELECTION_SMOOTHNESS_UNIFIED;
	AURA_CIRCLE_POINTS = 7 + 3 * SELECTION_SMOOTHNESS_UNIFIED;
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
			m_refd[i] = false;
		}
	}
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
	m_collisionPatches = NULL;
}

void CEntityManager::deleteAll()
{
	m_extant = false;
	
	deleteAllHelper();
	
	m_nextalloc = 0;

	delete[] m_collisionPatches;
	m_collisionPatches = NULL;

	m_extant = true;
}

HEntity CEntityManager::create(CEntityTemplate* base, CVector3D position, float orientation,
							   const std::set<CStr>& actorSelections,
							   const CStrW* building, int desiredHandle)
{
	debug_assert( base );
	if( !base )
		return HEntity();

	int pos = 0;

	if(desiredHandle >= 0) 
	{
		if( m_entities[desiredHandle].m_refcount )
		{
			debug_warn("Tried to create an entity at index %d, which is already taken.", desiredHandle);
			return HEntity();
		}
		else
		{
			pos = desiredHandle;
		}
	}
	else 
	{
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
	}

	m_entities[pos].m_entity = new CEntity( base, position, orientation, actorSelections, building );
	if( m_collisionPatches)
		m_entities[pos].m_entity->updateCollisionPatch();
	m_entities[pos].m_entity->me = HEntity( pos );

	return( HEntity( pos ) );
}

void CEntityManager::AddEntityClassData(const HEntity& handle)
{
	//Add data for this particular entity and player
	size_t playerID = handle->GetPlayer()->GetPlayerID();
	CStrW className, classList = handle->m_classes.getMemberList();

	while ( (className = classList.BeforeFirst(L" ")) != classList )
	{
		if ( m_entityClassData[playerID].find(className) == m_entityClassData[playerID].end() )
			m_entityClassData[playerID][className] = 0;
		++m_entityClassData[playerID][className];
		classList = classList.AfterFirst(L" ");
	}
	++m_entityClassData[playerID][className];
}

HEntity CEntityManager::create( const CStrW& templateName, CPlayer* player, CVector3D position, float orientation, const CStrW* building )
{
	CEntityTemplate* base = g_EntityTemplateCollection.getTemplate( templateName, player );
	debug_assert( base );
	if( !base )
		return HEntity();

	std::set<CStr> selections;

	HEntity ret = create( base, position, orientation, selections, building );
	AddEntityClassData(ret);
	return ret;
}

HEntity CEntityManager::createFoundation( const CStrW& templateName, CPlayer* player, CVector3D position, float orientation )
{
	CEntityTemplate* base = g_EntityTemplateCollection.getTemplate( templateName, player );
	debug_assert( base );
	if( !base )
		return HEntity();

	std::set<CStr> selections;

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

	float radiusSq = radius * radius;

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

	debug_assert(! m_collisionPatches);
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

/*
void CEntityManager::TickAll()
{
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( isEntityRefd(i) && m_entities[i].m_entity->m_extant )
			m_entities[i].m_entity->Tick();
}
*/

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
			m_entities[i].m_entity->updateXZOrientation();
			m_entities[i].m_entity->updateActorTransforms();
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
	
	//Remove trigger-helper data
	CEntity* ent = m_entities[handle].m_entity;
	size_t playerID = (size_t)ent->GetPlayer()->GetPlayerID();
	CStrW className, classList = ent->m_classes.getMemberList();

	while ( (className = classList.BeforeFirst(L" ")) != classList )
	{
		--m_entityClassData[playerID][className];
		classList = classList.AfterFirst(L" ");
	}
	--m_entityClassData[playerID][className];

	ent->me.m_handle = INVALID_HANDLE;
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
