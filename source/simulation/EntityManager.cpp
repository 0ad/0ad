#include "EntityManager.h"
#include "BaseEntityCollection.h"


CEntityManager::CEntityManager()
{
	m_nextalloc = 0;
	m_extant = true;
}

CEntityManager::~CEntityManager()
{
	m_extant = false;
}

HEntity CEntityManager::create( CBaseEntity* base, CVector3D position, float orientation )
{
	assert( base );
	while( m_entities[m_nextalloc].m_refcount )
		m_nextalloc++;
	m_entities[m_nextalloc].m_entity = new CEntity( base, position, orientation );
	m_entities[m_nextalloc].m_entity->me = HEntity( m_nextalloc );
	return( HEntity( m_nextalloc++ ) );
}

HEntity CEntityManager::create( CStr templatename, CVector3D position, float orientation )
{
	CBaseEntity* templateobj = g_EntityTemplateCollection.getTemplate( templatename );
	return( create( templateobj, position, orientation ) );
}

std::vector<HEntity>* CEntityManager::matches( EntityPredicate predicate )
{
	std::vector<HEntity>* matchlist = new std::vector<HEntity>;
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( m_entities[i].m_refcount )
			if( predicate( m_entities[i].m_entity ) )
				matchlist->push_back( HEntity( i ) );
	return( matchlist );
}

std::vector<HEntity>* CEntityManager::getActive()
{
	std::vector<HEntity>* activelist = new std::vector<HEntity>;
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( m_entities[i].m_refcount )
			activelist->push_back( HEntity( i ) );
	return( activelist );
}

void CEntityManager::dispatchAll( CMessage* msg )
{
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( m_entities[i].m_refcount )
			m_entities[i].m_entity->dispatch( msg );
}

void CEntityManager::kill( HEntity entity )
{
	m_reaper.push_back( entity );
}

void CEntityManager::updateAll( float timestep )
{
	std::vector<HEntity>::iterator it;
	for( it = m_reaper.begin(); it < m_reaper.end(); it++ )
		(*it)->me = HEntity();
	m_reaper.clear();

	for( int i = 0; i < MAX_HANDLES; i++ )
		if( m_entities[i].m_refcount )
			m_entities[i].m_entity->update( timestep );
}

void CEntityManager::renderAll()
{
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( m_entities[i].m_refcount )
			m_entities[i].m_entity->render();
}

void CEntityManager::destroy( u16 handle )
{
	m_entities[handle].m_entity->me.m_handle = INVALID_HANDLE;
	delete( m_entities[handle].m_entity );
}

bool CEntityManager::m_extant = false;