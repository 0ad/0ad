// Last modified: May 15 2004, Mark Thompson (mark@wildfiregames.com)

#ifndef ENTITY_INCLUDED
#define ENTITY_INCLUDED

#include <deque>
#include "EntityProperties.h"

#include "BaseEntity.h"
#include "Vector3D.h"
#include "Unit.h"
#include "UnitManager.h"
#include "EntityOrders.h"
#include "EntityHandles.h"
#include "EntityMessage.h"

class CEntityManager;

class CEntity
{
	friend CEntityManager;
private:
	// Intrinsic properties
public:
	CStr m_name;
	float m_speed;
	CVector3D m_position;
	float m_targetorientation;
	float m_orientation;
	CBaseEntity* m_base;
	CUnit* m_actor;

	std::deque<CEntityOrder> m_orderQueue;

	// Extended properties table

	std::hash_map<CStr,CGenericProperty,CStr_hash_compare> m_properties;

	CEntity( CBaseEntity* base, CVector3D position, float orientation );
public:

	// Handle-to-self.
	HEntity me;
	void dispatch( CMessage* msg );
	void update( float timestep );
	void updateActorTransforms();
	float getExactGroundLevel( float x, float y );
	void snapToGround();
	void pushOrder( CEntityOrder& order );
};

extern void PASAPScenario();

#endif