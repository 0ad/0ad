#include "precompiled.h"

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
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( m_entities[i].m_refcount )
			delete( m_entities[i].m_entity );
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

HEntity* CEntityManager::getByHandle( u16 index )
{
	if( index >= MAX_HANDLES ) return( NULL );
	if( !m_entities[index].m_refcount ) return( NULL );
	return( new HEntity( index ) );
}
std::vector<HEntity>* CEntityManager::matches( EntityPredicate predicate )
{
	std::vector<HEntity>* matchlist = new std::vector<HEntity>;
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( m_entities[i].m_refcount && m_entities[i].m_entity->m_extant )
			if( predicate( m_entities[i].m_entity ) )
				matchlist->push_back( HEntity( i ) );
	return( matchlist );
}

std::vector<HEntity>* CEntityManager::matches( EntityPredicate predicate1, EntityPredicate predicate2 )
{
	std::vector<HEntity>* matchlist = new std::vector<HEntity>;
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( m_entities[i].m_refcount && m_entities[i].m_entity->m_extant )
			if( predicate1( m_entities[i].m_entity ) && predicate2( m_entities[i].m_entity ) )
				matchlist->push_back( HEntity( i ) );
	return( matchlist );
}

std::vector<HEntity>* CEntityManager::getExtant()
{
	std::vector<HEntity>* activelist = new std::vector<HEntity>;
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( m_entities[i].m_refcount && m_entities[i].m_entity->m_extant )
			activelist->push_back( HEntity( i ) );
	return( activelist );
}

void CEntityManager::dispatchAll( CMessage* msg )
{
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( m_entities[i].m_refcount && m_entities[i].m_entity->m_extant )
			m_entities[i].m_entity->dispatch( msg );
}

void CEntityManager::updateAll( float timestep )
{
	std::vector<CEntity*>::iterator it;
	for( it = m_reaper.begin(); it < m_reaper.end(); it++ )
		delete( *it );
	m_reaper.clear();

	for( int i = 0; i < MAX_HANDLES; i++ )
		if( m_entities[i].m_refcount && m_entities[i].m_entity->m_extant )
			m_entities[i].m_entity->update( timestep );
}

void CEntityManager::interpolateAll( float relativeoffset )
{
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( m_entities[i].m_refcount && m_entities[i].m_entity->m_extant )
			m_entities[i].m_entity->interpolate( relativeoffset );
}

void CEntityManager::renderAll()
{
	for( int i = 0; i < MAX_HANDLES; i++ )
		if( m_entities[i].m_refcount && m_entities[i].m_entity->m_extant )
			m_entities[i].m_entity->render();
}

void CEntityManager::destroy( u16 handle )
{
	m_reaper.push_back( m_entities[handle].m_entity );
	m_entities[handle].m_entity->me.m_handle = INVALID_HANDLE;
	delete( m_entities[handle].m_entity );
}

bool CEntityManager::m_extant = false;
