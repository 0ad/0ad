// Last modified: May 15 2004, Mark Thompson (mark@wildfiregames.com)

#include "Entity.h"
#include "Model.h"
#include "Terrain.h"
#include "EntityManager.h"
#include "BaseEntityCollection.h"


CEntity::CEntity( CBaseEntity* base, CVector3D position, float orientation )
{
	// Set our parent unit and build us an actor.

	m_base = base;
	m_actor = new CUnit;

	m_actor->m_Object = m_base->m_actorObject;
	m_actor->m_Model = ( m_actor->m_Object->m_Model ) ? m_actor->m_Object->m_Model->Clone() : NULL;

	// Register the actor with the renderer.

	g_UnitMan.AddUnit( m_actor );

	// Set up our instance data

	m_speed = m_base->m_speed;
	m_position = position;
	m_orientation = orientation;

	snapToGround();
	updateActorTransforms();

	// Register the addresses of our native properties with the properties table
	m_properties["speed"] = &m_speed;
	m_properties["orientation"] = &m_orientation;
	m_properties["position"] = &m_position;
}

bool isWaypoint( CEntity* e )
{
	return( e->m_base->m_name == CStr( "Waypoint" ) );
}

void CEntity::updateActorTransforms()
{
	CMatrix3D m;

	m.SetYRotation( m_orientation );

	m.Translate( m_position );

	m_actor->m_Model->SetTransform( m );
}

float CEntity::getExactGroundLevel( float x, float y )
{
	// TODO MT: If OK with Rich, move to terrain core. Once this works, that is.

	x /= 4.0f;
	y /= 4.0f;

	int xi = (int)floor( x );
	int yi = (int)floor( y );
	float xf = x - (float)xi;
	float yf = y - (float)yi;

	u16* heightmap = g_Terrain.GetHeightMap();
	unsigned long mapsize = g_Terrain.GetVerticesPerSide();

	float h00 = heightmap[yi*mapsize + xi];
	float h01 = heightmap[yi*mapsize + xi + mapsize];
	float h10 = heightmap[yi*mapsize + xi + 1];
	float h11 = heightmap[yi*mapsize + xi + mapsize + 1];

	/*
	if( xf < ( 1.0f - yf ) )
	{
		return( HEIGHT_SCALE * ( ( 1 - xf - yf ) * h00 + xf * h10 + yf * h01 ) );
	}
	else
		return( HEIGHT_SCALE * ( ( xf + yf - 1 ) * h11 + ( 1 - xf ) * h01 + ( 1 - yf ) * h10 ) );
	*/

	/*
	if( xf > yf ) 
	{
		return( HEIGHT_SCALE * ( ( 1 - xf ) * h00 + ( xf - yf ) * h10 + yf * h11 ) );
	}
	else
		return( HEIGHT_SCALE * ( ( 1 - yf ) * h00 + ( yf - xf ) * h01 + xf * h11 ) );
	*/

	return( HEIGHT_SCALE * ( ( 1 - yf ) * ( ( 1 - xf ) * h00 + xf * h10 ) + yf * ( ( 1 - xf ) * h01 + xf * h11 ) ) );

}

void CEntity::snapToGround()
{
	m_position.Y = getExactGroundLevel( m_position.X, m_position.Z );
}

void CEntity::update( float timestep )
{
	while( !m_orderQueue.empty() )
	{
		CEntityOrder* current = &m_orderQueue.front();

		switch( current->m_type )
		{
		case CEntityOrder::ORDER_GOTO_NOPATHING:
			{
				float deltax = (float)current->m_data[0].location.x - m_position.X;
				float deltay = (float)current->m_data[0].location.y - m_position.Z;

				m_targetorientation = atan2( -deltax, -deltay );
				float deltatheta = m_targetorientation - m_orientation;
				if( deltatheta > PI )
					deltatheta -= 2 * PI;
				if( deltatheta < -PI )
					deltatheta += 2 * PI;

				/*
				if( deltatheta > 1.0f * timestep )
					deltatheta = 1.0f * timestep;
				if( deltatheta < -1.0f * timestep )
					deltatheta = -1.0f * timestep;
				*/

				m_orientation += deltatheta;

				float len = sqrt( deltax * deltax + deltay * deltay );
				float scale = timestep * m_speed;
				if( scale > len )
					scale = len;

				deltax = -sinf( m_orientation ) * scale; deltay = -cosf( m_orientation ) * scale;
				m_position.X += deltax;
				m_position.Z += deltay;

				snapToGround();
				updateActorTransforms();
				if( len < 0.1f )
					m_orderQueue.pop_front();
				return;
				break;
			}
		case CEntityOrder::ORDER_GOTO:
			{
				CEntityOrder pathfind_solution;
				pathfind_solution.m_type = CEntityOrder::ORDER_GOTO_NOPATHING;
				pathfind_solution.m_data[0] = current->m_data[0];
				m_orderQueue.pop_front();
				m_orderQueue.push_front( pathfind_solution );
				if( m_actor->m_Model->GetAnimation() != m_actor->m_Object->m_WalkAnim )
					m_actor->m_Model->SetAnimation( m_actor->m_Object->m_WalkAnim );
				break;
			}
		case CEntityOrder::ORDER_PATROL:
			{
				CEntityOrder this_segment;
				CEntityOrder repeat_patrol;
				this_segment.m_type = CEntityOrder::ORDER_GOTO;
				this_segment.m_data[0] = current->m_data[0];
				repeat_patrol.m_type = CEntityOrder::ORDER_PATROL;
				repeat_patrol.m_data[0] = current->m_data[0];
				m_orderQueue.pop_front();
				m_orderQueue.push_front( this_segment );
				m_orderQueue.push_back( repeat_patrol );
				break;
			}
		default:
			assert( 0 && "Invalid entity order" );
		}
	}
	if( m_actor->m_Model->GetAnimation() != m_actor->m_Object->m_IdleAnim )
		m_actor->m_Model->SetAnimation( m_actor->m_Object->m_IdleAnim );
}

void CEntity::dispatch( CMessage* msg )
{
	switch( msg->type )
	{
	case CMessage::EMSG_TICK:
		break;
	case CMessage::EMSG_INIT:
		if( m_base->m_name == CStr( "Prometheus Dude" ) )
		{
			std::vector<HEntity>* waypoints = g_EntityManager.matches( isWaypoint );
			while( !waypoints->empty() )
			{
				CEntityOrder patrol;
				size_t id = rand() % waypoints->size();
				std::vector<HEntity>::iterator it = waypoints->begin();
				it += id;
				HEntity waypoint = *it;
				patrol.m_type = CEntityOrder::ORDER_PATROL;
				patrol.m_data[0].location.x = waypoint->m_position.X;
				patrol.m_data[0].location.y = waypoint->m_position.Z;
				pushOrder( patrol );
				waypoints->erase( it );
			}
		}
		break;
	}
}
			
void CEntity::pushOrder( CEntityOrder& order )
{
	m_orderQueue.push_back( order );
}

void PASAPScenario()
{
	// Entity demo. I don't know where this information is going to come from for pre-PASAP.
	// So, it's hardcoded here. *shrug*

	/*
	float waypoints[4][2] = { { 12.0f, 64.0f },
							  { 28.0f, 64.0f }, 
							  { 56.0f, 52.0f },
							  { 24.0f, 28.0f } };
	*/

	/*

	for( int i = 0; i < 4; i++ )
	{
		HEntity bob = g_EntityManager.create( "Prometheus Dude", CVector3D( waypoints[i][0], 0.0f, waypoints[i][1] ), 0.0f );

		for( int t = 0; t < 4; t++ )
		{
			CEntityOrder march_of_bob;
			march_of_bob.m_type = CEntityOrder::ORDER_PATROL ;
			march_of_bob.m_data[0].location.x = waypoints[(i+t+1)%4][0];
			march_of_bob.m_data[0].location.y = waypoints[(i+t+1)%4][1];

			bob->pushOrder( march_of_bob );
		}
	}

	*/
}