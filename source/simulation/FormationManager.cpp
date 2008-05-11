#include "precompiled.h"

#include "FormationManager.h"
#include "FormationCollection.h"
#include "Entity.h"
#include "ps/CStr.h"
#include "Formation.h"
#include "EntityFormation.h"
#include "EventHandlers.h"

#include "ps/Vector2D.h"

CFormationManager::~CFormationManager()
{
	for ( size_t i=0; i<m_formations.size(); i++ )
		 delete m_formations[i];
}
void CFormationManager::CreateFormation( CEntityList& entities, CStrW& name )
{
	if ( entities.empty() )
	{
		debug_warn("Attempting to create a formation with no entities");
		return;
	}
	CFormation* base = g_EntityFormationCollection.GetTemplate(name);
	if (!base)
		return;
	if ( entities.size() < (size_t)base->m_required )
		return;

	CEntityFormation* tmp = new CEntityFormation( base, m_formations.size() );
	m_formations.push_back( tmp );
	CEntityList accepted = AddUnitList( entities, (int)m_formations.size()-1 );

	//Find average position
	CVector3D average( 0.0f, 0.0f, 0.0f );
	for ( CEntityList::iterator it=accepted.begin(); it != accepted.end(); it++ )
		average += (*it)->m_position;
	average = average * ( 1.0f / (float)entities.size() );
	CVector2D average2D(average.X, average.Z);

	m_formations.back()->m_position = average2D;
	m_formations.back()->UpdateFormation();

	if ( accepted.size() < (size_t)base->m_required )
		DestroyFormation( m_formations.size()-1 );
}
void CFormationManager::DestroyFormation( size_t form )
{
	if ( form >= m_formations.size())
	{
		debug_warn("CFormationManager::DestroyFormation--invalid entity");
		return;
	}
	FormIterator it=m_formations.begin() + form;
	CEntityList entities = (*it)->GetEntityList();
	//Notify the script that we've "left" the formation
	for ( size_t i=0; i<entities.size(); i++ )
	{
		CEntity* entity = entities[i];
		entity->DispatchFormationEvent( CFormationEvent::FORMATION_LEAVE );
		(*it)->RemoveUnit(entity);
	}
	delete *it;
	*it = NULL;
	m_formations.erase( it );
	UpdateIndexes( form );
}
bool CFormationManager::AddUnit( CEntity* entity, size_t& form )
{
	if ( !IsValidFormation(form) )
		return false;

	if ( entity->m_formation > -1 )
	{
		if ( !RemoveUnit( entity ) )
			--form;
	}
	FormIterator it = m_formations.begin() + form;
	//Adding too many?
	if ( (*it)->m_numEntities == (*it)->m_base->m_numSlots )
	{
		CFormation* next = g_EntityFormationCollection.GetTemplate((*it)->m_base->m_next);
		if (next)
			(*it)->SwitchBase( next );
	}
	if ( (*it)->AddUnit(entity) )	//This unit might be better
	{
		entity->DispatchFormationEvent( CFormationEvent::FORMATION_ENTER );
		return true;
	}
	return false;
}
CEntityList CFormationManager::AddUnitList( CEntityList& entities, size_t form )
{
	CEntityList accepted;
	for ( CEntityList::iterator it=entities.begin(); it != entities.end(); it++ )
	{
		CEntity* entity = *it;
		if ( AddUnit( entity, form ) )
			accepted.push_back( *it );
	}
	return accepted;
}
bool CFormationManager::RemoveUnit( CEntity* entity )
{
	if ( !IsValidFormation(entity->m_formation) )
		return true;

	FormIterator it = m_formations.begin() + entity->m_formation;
	if ( (*it)->m_numEntities == (*it)->m_base->m_required )
	{
		CFormation* prior = g_EntityFormationCollection.GetTemplate((*it)->m_base->m_prior);
		//Disband formation
		if (!prior)
		{
			DestroyFormation( entity->m_formation );
			return false;
		}
	}
	entity->DispatchFormationEvent( CFormationEvent::FORMATION_LEAVE );
	(*it)->RemoveUnit( entity );
	return true;
}
bool CFormationManager::RemoveUnitList( CEntityList& entities )
{
	for ( CEntityList::iterator it=entities.begin(); it != entities.end(); it++ )
	{
		CEntity* entity = *it;
		if ( !RemoveUnit(entity) )
			return false;
	}
	return true;
}
CEntityFormation* CFormationManager::GetFormation(size_t form)
{
	if ( IsValidFormation(form) )
		return m_formations[form];
	return NULL;
}
void CFormationManager::UpdateIndexes( size_t update )
{
	for ( ; update < m_formations.size(); update++ )
		m_formations[update]->ResetIndex( update );
}
