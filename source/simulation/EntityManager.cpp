#include "precompiled.h"

#include "EntityManager.h"
#include "BaseEntityCollection.h"
#include "ConfigDB.h"

int SELECTION_CIRCLE_POINTS;
int SELECTION_BOX_POINTS;
int SELECTION_SMOOTHNESS_UNIFIED = 9;

CEntityManager::CEntityManager()
: m_entities()	// janwas: default-initialize entire array;
				// CHandle ctor sets m_entity and m_refcount to 0
{
	m_nextalloc = 0;
	m_extant = true;

	// Also load a couple of global entity settings
	CConfigValue* cfg = g_ConfigDB.GetValue( CFG_USER, "selection.outline.quality" );
	if( cfg ) cfg->GetInt( SELECTION_SMOOTHNESS_UNIFIED );
	if( SELECTION_SMOOTHNESS_UNIFIED < 0 ) SELECTION_SMOOTHNESS_UNIFIED = 0;
	SELECTION_CIRCLE_POINTS = 7 + 2 * SELECTION_SMOOTHNESS_UNIFIED;
	SELECTION_BOX_POINTS = 1 + SELECTION_SMOOTHNESS_UNIFIED;
}

CEntityManager::~CEntityManager()
{	
	m_extant = false;
	
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( m_entities[i].m_refcount )
		{
			delete( m_entities[i].m_entity );
			m_entities[i].m_entity = 0;
			m_entities[i].m_refcount = 0;
		}
}

void CEntityManager::deleteAll()
{
	m_extant = false;
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( m_entities[i].m_refcount )
		{
			delete( m_entities[i].m_entity );
			m_entities[i].m_refcount = 0;
		}
	m_nextalloc = 0;
	m_extant = true;
}

HEntity CEntityManager::create( CBaseEntity* base, CVector3D position, float orientation )
{
	assert( base );
	if( !base )
		return( HEntity() );

	while( m_entities[m_nextalloc].m_refcount )
	{
		m_nextalloc++;
		assert(m_nextalloc < MAX_HANDLES);
	}
	m_entities[m_nextalloc].m_entity = new CEntity( base, position, orientation );
	m_entities[m_nextalloc].m_entity->me = HEntity( m_nextalloc );
	return( HEntity( m_nextalloc++ ) );
}

HEntity CEntityManager::create( CStrW templatename, CVector3D position, float orientation )
{
	CBaseEntity* templateobj = g_EntityTemplateCollection.getTemplate( templatename );
	return( create( templateobj, position, orientation ) );
}

HEntity* CEntityManager::getByHandle( u16 index )
{
	if( index >= MAX_HANDLES ) return( NULL );
	if( !m_entities[index].m_refcount ) return( NULL );
	return( new HEntity( index ) );
}

std::vector<HEntity>* CEntityManager::matches( EntityPredicate predicate, void* userdata )
{
	std::vector<HEntity>* matchlist = new std::vector<HEntity>;
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( m_entities[i].m_refcount && !m_entities[i].m_entity->m_destroyed )
			if( predicate( m_entities[i].m_entity, userdata ) )
				matchlist->push_back( HEntity( i ) );
	return( matchlist );
}

std::vector<HEntity>* CEntityManager::getExtant()
{
	std::vector<HEntity>* activelist = new std::vector<HEntity>;
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( m_entities[i].m_refcount && !m_entities[i].m_entity->m_destroyed )
			activelist->push_back( HEntity( i ) );
	return( activelist );
}

/*
void CEntityManager::dispatchAll( CMessage* msg )
{
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( m_entities[i].m_refcount && m_entities[i].m_entity->m_extant )
			m_entities[i].m_entity->dispatch( msg );
}
*/

void CEntityManager::InitializeAll()
{
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( m_entities[i].m_refcount && !m_entities[i].m_entity->m_destroyed )
			m_entities[i].m_entity->Initialize();
}

void CEntityManager::TickAll()
{
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( m_entities[i].m_refcount && !m_entities[i].m_entity->m_destroyed )
			m_entities[i].m_entity->Tick();
}

void CEntityManager::updateAll( size_t timestep )
{
	std::vector<CEntity*>::iterator it;
	for( it = m_reaper.begin(); it < m_reaper.end(); it++ )
		delete( *it );
	m_reaper.clear();

	TickAll();

	for( int i = 0; i < MAX_HANDLES; i++ )
		if( m_entities[i].m_refcount && !m_entities[i].m_entity->m_destroyed )
			m_entities[i].m_entity->update( timestep );
}

void CEntityManager::interpolateAll( float relativeoffset )
{
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( m_entities[i].m_refcount && !m_entities[i].m_entity->m_destroyed )
			m_entities[i].m_entity->interpolate( relativeoffset );
}

void CEntityManager::renderAll()
{
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( m_entities[i].m_refcount && !m_entities[i].m_entity->m_destroyed )
			m_entities[i].m_entity->render();
}

void CEntityManager::destroy( u16 handle )
{
	m_reaper.push_back( m_entities[handle].m_entity );
	m_entities[handle].m_entity->me.m_handle = INVALID_HANDLE;
}

bool CEntityManager::m_extant = false;
