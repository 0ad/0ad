#include "precompiled.h"
#include "Interact.h"
#include "Renderer.h"
#include "input.h"
#include "CConsole.h"
#include "HFTracer.h"
#include "Hotkey.h"
#include "timer.h"
#include "Game.h"
#include "Network/NetMessage.h"

extern CConsole* g_Console;
extern int mouse_x, mouse_y;
extern bool keys[SDLK_LAST];
extern bool g_active;
extern CStr g_CursorName;

static const float SELECT_DBLCLICK_RATE = 0.5f;
const int ORDER_DELAY = 5;

void CSelectedEntities::addSelection( HEntity entity )
{
	m_group = -1;
	assert( !isSelected( entity ) );
	m_selected.push_back( entity );
	entity->m_selected = true;
	m_selectionChanged = true;
}

void CSelectedEntities::removeSelection( HEntity entity )
{
	m_group = -1;
	assert( isSelected( entity ) );
	entity->m_selected = false;
	std::vector<HEntity>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
	{
		if( (*it) == entity ) 
		{
			m_selected.erase( it );
			m_selectionChanged = true;
			break;
		}
	}
}

void CSelectedEntities::renderSelectionOutlines()
{
	std::vector<HEntity>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		(*it)->renderSelectionOutline();

	if( m_group_highlight != -1 )
	{
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glEnable( GL_BLEND );

		std::vector<HEntity>::iterator it;
		for( it = m_groups[m_group_highlight].begin(); it < m_groups[m_group_highlight].end(); it++ )
			(*it)->renderSelectionOutline( 0.5f );

		glDisable( GL_BLEND );
	}
}

void CSelectedEntities::renderOverlays()
{
	CTerrain *pTerrain=g_Game->GetWorld()->GetTerrain();
	CCamera *pCamera=g_Game->GetView()->GetCamera();

	glPushMatrix();
	glEnable( GL_TEXTURE_2D );
	std::vector<HEntity>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
	{
		if( (*it)->m_grouped != -1 )
		{
			if( !(*it)->m_bounds ) continue;
			
			glLoadIdentity();
			float x, y;
			CVector3D labelpos = (*it)->m_graphics_position - pCamera->m_Orientation.GetLeft() * (*it)->m_bounds->m_radius;
#ifdef SELECTION_TERRAIN_CONFORMANCE
			labelpos.Y = pTerrain->getExactGroundLevel( labelpos.X, labelpos.Z );
#endif
			pCamera->GetScreenCoordinates( labelpos, x, y );
			glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
			glTranslatef( x, g_Renderer.GetHeight() - y, 0.0f );
			glScalef( 1.0f, -1.0f, 1.0f );
			glwprintf( L"%d", (i32) (*it)->m_grouped );
			
		}
	}
	if( m_group_highlight != -1 )
	{
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glEnable( GL_BLEND );

		std::vector<HEntity>::iterator it;
		for( it = m_groups[m_group_highlight].begin(); it < m_groups[m_group_highlight].end(); it++ )
		{
			if( !(*it)->m_bounds ) continue;
			
			glLoadIdentity();
			float x, y;
			CVector3D labelpos = (*it)->m_graphics_position - pCamera->m_Orientation.GetLeft() * (*it)->m_bounds->m_radius;
#ifdef SELECTION_TERRAIN_CONFORMANCE
			labelpos.Y = pTerrain->getExactGroundLevel( labelpos.X, labelpos.Z );
#endif
			pCamera->GetScreenCoordinates( labelpos, x, y );
			glColor4f( 1.0f, 1.0f, 1.0f, 0.5f );
			glTranslatef( x, g_Renderer.GetHeight() - y, 0.0f );
			glScalef( 1.0f, -1.0f, 1.0f );
			glwprintf( L"%d", (i32) (*it)->m_grouped );
		}

		glDisable( GL_BLEND );
	}

	glLoadIdentity();
	glTranslatef( (float)( mouse_x + 16 ), (float)( g_Renderer.GetHeight() - mouse_y - 8 ), 0.0f );
	glScalef( 1.0f, -1.0f, 1.0f );
	glColor4f( 1.0f, 1.0f, 1.0f, 0.5f );
	switch( m_contextOrder )
	{
	case CEntityOrder::ORDER_GOTO:
		glwprintf( L"Go to" );
		break;
	case CEntityOrder::ORDER_PATROL:
		glwprintf( L"Patrol to" );
		break;
	case CEntityOrder::ORDER_ATTACK_MELEE:
		glwprintf( L"Attack" );
		break;
	}

	glDisable( GL_TEXTURE_2D );
	glPopMatrix();
}

void CSelectedEntities::setSelection( HEntity entity )
{
	m_group = -1;
	clearSelection();
	m_selected.push_back( entity );
}

void CSelectedEntities::clearSelection()
{
	m_group = -1;
	std::vector<HEntity>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		(*it)->m_selected = false;
	m_selected.clear();
	m_selectionChanged = true;
}

void CSelectedEntities::removeAll( HEntity entity )
{
	// Remove a reference to an entity from everywhere
	// (for use when said entity is being destroyed)
	std::vector<HEntity>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
	{
		if( (*it) == entity ) 
		{
			m_selected.erase( it );
			m_selectionChanged = true;
			break;
		}
	}
	for( u8 group = 0; group < MAX_GROUPS; group++ )
	{
		for( it = m_groups[group].begin(); it < m_groups[group].end(); it++ )
		{
			if( (*it) == entity ) 
			{
				m_groups[group].erase( it );
				m_selectionChanged = true;
				break;
			}
		}
	}
}

CVector3D CSelectedEntities::getSelectionPosition()
{
	CVector3D avg;
	std::vector<HEntity>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		avg += (*it)->m_graphics_position;
	return( avg * ( 1.0f / m_selected.size() ) );
}

void CSelectedEntities::saveGroup( i8 groupid )
{
	std::vector<HEntity>::iterator it;
	// Clear all entities in the group...
	for( it = m_groups[groupid].begin(); it < m_groups[groupid].end(); it++ )
		(*it)->m_grouped = -1;
	
	m_groups[groupid].clear();
	// Remove selected entities from each group they're in, and flag them as
	// members of the new group
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
	{
		if( (*it)->m_grouped != -1 )
		{
			std::vector<HEntity>& group = m_groups[(*it)->m_grouped];
			std::vector<HEntity>::iterator it2;
			for( it2 = group.begin(); it2 < group.end(); it2++ )
			{
				if( (*it2) == &(**it) )
				{
					group.erase( it2 );
					break;
				}
			}
		}
		(*it)->m_grouped = groupid;
	}
	// Copy the group across
	m_groups[groupid] = m_selected;
	// Set the group selection memory
	m_group = groupid;
}

void CSelectedEntities::addToGroup( i8 groupid, HEntity entity )
{
	std::vector<HEntity>::iterator it;

	// Remove selected entities from each group they're in, and flag them as
	// members of the new group
	if( entity->m_grouped != -1 )
	{
		std::vector<HEntity>& group = m_groups[(*it)->m_grouped];
		std::vector<HEntity>::iterator it2;
		for( it2 = group.begin(); it2 < group.end(); it2++ )
		{
			if( (*it2) == entity )
			{
				group.erase( it2 );
				break;
			}
		}
	}
	entity->m_grouped = groupid;

	m_groups[groupid].push_back( entity );
}

void CSelectedEntities::loadGroup( i8 groupid )
{
	if( m_group == groupid )
		return;

	clearSelection();
	m_selected = m_groups[groupid];

	std::vector<HEntity>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		(*it)->m_selected = true;
	m_group = groupid;

	m_selectionChanged = true;
}

void CSelectedEntities::addGroup( i8 groupid )
{
	std::vector<HEntity>::iterator it;
	for( it = m_groups[groupid].begin(); it < m_groups[groupid].end(); it++ )
	{
		if( !isSelected( *it ) )
			addSelection( *it );
	}
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		(*it)->m_selected = true;
}

void CSelectedEntities::changeGroup( HEntity entity, i8 groupid )
{
	// Remove from current group
	i32 current = entity->m_grouped;
	if( current != -1 )
	{
		std::vector<HEntity>::iterator it;
		for( it = m_groups[current].begin(); it < m_groups[current].end(); it++ )
		{
			if( (*it) == entity ) 
			{
				m_groups[current].erase( it );
				break;
			}
		}
	}
	if( groupid != -1 )
		m_groups[groupid].push_back( entity );
	entity->m_grouped = groupid;
}

bool CSelectedEntities::isSelected( HEntity entity )
{
	std::vector<HEntity>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
	{
		if( (*it) == entity ) 
			return( true );
	}
	return( false );
}

void CSelectedEntities::highlightGroup( i8 groupid )
{
	if( m_group_highlight != -1 )
		return;
	if( !getGroupCount( groupid ) )
		return;
	m_group_highlight = groupid;
	g_Game->GetView()->PushCameraTarget( getGroupPosition( groupid ) );
}

void CSelectedEntities::highlightNone()
{

	if( m_group_highlight != -1 )
		g_Game->GetView()->PopCameraTarget();
	m_group_highlight = -1;

}

int CSelectedEntities::getGroupCount( i8 groupid )
{
	return( (int)m_groups[groupid].size() );
}

CVector3D CSelectedEntities::getGroupPosition( i8 groupid )
{
	CVector3D avg;
	std::vector<HEntity>::iterator it;
	for( it = m_groups[groupid].begin(); it < m_groups[groupid].end(); it++ )
		avg += (*it)->m_graphics_position;
	return( avg * ( 1.0f / m_groups[groupid].size() ) );
}

void CSelectedEntities::update()
{
	if( m_selectionChanged || g_Mouseover.m_targetChanged )
	{
		// Can't order anything off the map
		if( !g_Game->GetWorld()->GetTerrain()->isOnMap( g_Mouseover.m_worldposition ) )
		{
			m_contextOrder = -1;
			return;
		}

		// Quick count to see which is the modal default order.

		int defaultPoll[CEntityOrder::ORDER_LAST];
		int t, vote;
		for( t = 0; t < CEntityOrder::ORDER_LAST; t++ )
			defaultPoll[t] = 0;

		std::vector<HEntity>::iterator it;
		for( it = m_selected.begin(); it < m_selected.end(); it++ )
		{
			vote = (*it)->defaultOrder( g_Mouseover.m_target );
			if( ( vote >= 0 ) && ( vote < CEntityOrder::ORDER_LAST ) )
				defaultPoll[vote]++;
		}

		vote = -1;
		for( t = 0; t < CEntityOrder::ORDER_LAST; t++ )
		{
			if( ( vote == -1 ) || ( defaultPoll[t] > defaultPoll[vote] ) )
				vote = t;
		}

		m_contextOrder = vote;
		switch( m_contextOrder )
		{
		case CEntityOrder::ORDER_ATTACK_MELEE:
			g_CursorName = "action-attack";
			break;
		default:
			g_CursorName = "arrow-default";
			break;
		}

		m_selectionChanged = false;
		g_Mouseover.m_targetChanged = false;
	}
	/*
	if( !isContextValid( m_contextOrder ) )
	{
		// This order isn't valid for the current selection and/or target.
		for( int t = 0; t < CEntityOrder::ORDER_LAST; t++ )
			if( isContextValid( t ) )
			{
				m_contextOrder = t; return;
			}
		m_contextOrder = -1;
	}
	*/

	if( ( m_group_highlight != -1 ) && getGroupCount( m_group_highlight ) )
		g_Game->GetView()->SetCameraTarget( getGroupPosition( m_group_highlight ) );

}

void CSelectedEntities::setContext( int contextOrder )
{
	assert( isContextValid( contextOrder ) );
	m_contextOrder = contextOrder;
}

bool CSelectedEntities::nextContext()
{
	// No valid orders? 
	if( m_contextOrder == -1 ) return( false );
	int t = m_contextOrder + 1;
	while( t != m_contextOrder )
	{
		if( t == CEntityOrder::ORDER_LAST ) t = 0;
		if( isContextValid( t ) ) break;
		t++;
	}
	if( m_contextOrder == t )
		return( false );
	m_contextOrder = t;
	return( true );
}

bool CSelectedEntities::previousContext()
{
	// No valid orders?
		if( m_contextOrder == -1 ) return( false );
	int t = m_contextOrder - 1;
	while( t != m_contextOrder )
	{
		if( isContextValid( t ) ) break;
		if( t == 0 ) t = CEntityOrder::ORDER_LAST;
		t--;
	}
	if( m_contextOrder == t )
		return( false );
	m_contextOrder = t;
	return( true );
}

bool CSelectedEntities::isContextValid( int contextOrder )
{
	if( contextOrder == -1 ) return( false );

	// Can't order anything off the map
	if( !g_Game->GetWorld()->GetTerrain()->isOnMap( g_Mouseover.m_worldposition ) )
		return( false );

	// Check to see if any member of the selection supports this order type.
	std::vector<HEntity>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		if( (*it)->acceptsOrder( contextOrder, g_Mouseover.m_target ) )
			return( true );
	return( false );
}

void CSelectedEntities::contextOrder( bool pushQueue )
{
	CCamera *pCamera=g_Game->GetView()->GetCamera();
	CTerrain *pTerrain=g_Game->GetWorld()->GetTerrain();

	std::vector<HEntity>::iterator it;
	CEntityOrder context, contextRandomized;
	(int&)context.m_type = m_contextOrder;

	switch( m_contextOrder )
	{
// PATROL order: temporatily disabled until we define the network command for it
	case CEntityOrder::ORDER_PATROL:
	case CEntityOrder::ORDER_GOTO:
	{
		context.m_data[0].location = g_Mouseover.m_worldposition;
		break;
/*		
		CGotoCommand *msg=new CGotoCommand();
		msg->m_Entity=m_selected[0]->me;
		msg->m_TargetX=(u32)g_Mouseover.m_worldposition.x;
		msg->m_TargetY=(u32)g_Mouseover.m_worldposition.y;
		g_Game->GetSimulation()->QueueLocalCommand(msg);
		break;
*/
	}
	case CEntityOrder::ORDER_ATTACK_MELEE:
	{
		context.m_data[0].entity = g_Mouseover.m_target;
		for( it = m_selected.begin(); it < m_selected.end(); it++ )
			if( (*it)->acceptsOrder( m_contextOrder, g_Mouseover.m_target ) )
			{
				if( !pushQueue )
					(*it)->clearOrders();
				(*it)->pushOrder( context );
			}
		return;
	}
	default:
		break;
	}

	// Location randomizer, for group orders...
	// Having the group turn up at the destination with /some/ sort of cohesion is good
	// but tasking them all to the exact same point will leave them brawling for it
	// at the other end (it shouldn't, but the PASAP pathfinder is too simplistic)
	
	// Task them all to a point within a radius of the target, radius depends upon
	// the number of units in the group.

	/* (Simon)
	Hmm. Disabled in the makeshift transition to Command Message Queueing...

	Ideally, we'd create a Group with the selected entities, then queue a
	command with the Group ID as performing entity, with the center as the
	target, then let the location randomization be done in the
	net command=>ent. order translator (for now, CSimulation::TranslateMessage)

	Unless this is a problem that'll only be around until we make the real
	pathfinder?
	*/

	float radius = 2.0f * sqrt( (float)m_selected.size() - 1 ); 

	float _x, _y;


	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		if( (*it)->acceptsOrder( m_contextOrder, g_Mouseover.m_target ) )
		{
			contextRandomized = context;
			do
			{
				_x = (float)( rand() % 20000 ) / 10000.0f - 1.0f;
				_y = (float)( rand() % 20000 ) / 10000.0f - 1.0f;
			}
			while( ( _x * _x ) + ( _y * _y ) > 1.0f );

			contextRandomized.m_data[0].location.x += _x * radius;
			contextRandomized.m_data[0].location.y += _y * radius;

			// Clamp it to within the map, just in case.
			float mapsize = (float)g_Game->GetWorld()->GetTerrain()->GetVerticesPerSide();
			if( contextRandomized.m_data[0].location.x < 0.0f )
				contextRandomized.m_data[0].location.x = 0.0f;
			if( contextRandomized.m_data[0].location.x >= mapsize )
				contextRandomized.m_data[0].location.x = mapsize;
			if( contextRandomized.m_data[0].location.y < 0.0f )
				contextRandomized.m_data[0].location.y = 0.0f;
			if( contextRandomized.m_data[0].location.y >= mapsize )
				contextRandomized.m_data[0].location.y = mapsize;

			if( !pushQueue )
				(*it)->clearOrders();
			
			(*it)->pushOrder( contextRandomized );
		}

}

void CMouseoverEntities::update( float timestep )
{
	CCamera *pCamera=g_Game->GetView()->GetCamera();
	CTerrain *pTerrain=g_Game->GetWorld()->GetTerrain();

	CVector3D origin, dir;
	pCamera->BuildCameraRay( origin, dir );

	CUnit* hit = g_UnitMan.PickUnit( origin, dir );
	
	m_worldposition = pCamera->GetWorldCoordinates();

	if( hit && hit->GetEntity() && hit->GetEntity()->m_extant )
	{
		m_target = hit->GetEntity()->me;
	}
	else
		m_target = HEntity();

	if( m_target != m_lastTarget )
	{
		m_targetChanged = true;
		m_lastTarget = m_target;
	}

	if( m_viewall )
	{
		// 'O' key. Show selection outlines for all player units on screen
		// (as if bandboxing them all).
		// These aren't selectable; clicking when 'O' is pressed won't select
		// all units on screen.

		m_mouseover.clear();

		std::vector<HEntity>* onscreen = g_EntityManager.matches( isOnScreen );
		std::vector<HEntity>::iterator it;
		
		for( it = onscreen->begin(); it < onscreen->end(); it++ )
			if( (*it)->m_extant )
				m_mouseover.push_back( SMouseoverFader( *it, m_fademaximum, false ) );

		delete( onscreen );
	}
	else if( m_bandbox )
	{
		m_x2 = mouse_x;
		m_y2 = mouse_y;
		// Here's the fun bit:
		// Get the screen-space coordinates of all onscreen entities
		// then find the ones falling within the box.
		//
		// Fade in the ones in the box at (in+out) speed, then fade everything
		// out at (out) speed.
		std::vector<HEntity>* onscreen = g_EntityManager.matches( isOnScreen );
		std::vector<HEntity>::iterator it;

		// Reset active flags on everything...

		std::vector<SMouseoverFader>::iterator it2;
		for( it2 = m_mouseover.begin(); it2 < m_mouseover.end(); it2++ )
			it2->isActive = false;

		for( it = onscreen->begin(); it < onscreen->end(); it++ )
		{
			if( !(*it)->m_extant )
				continue;

			CVector3D worldspace = (*it)->m_graphics_position;

			float x, y;

			pCamera->GetScreenCoordinates( worldspace, x, y );

			bool inBox;
			if( m_x1 < m_x2 )
			{
				inBox = ( x >= m_x1 ) && ( x < m_x2 );
			}
			else
			{
				inBox = ( x >= m_x2 ) && ( x < m_x1 );
			}
			
			if( m_y1 < m_y2 )
			{
				inBox &= ( y >= m_y1 ) && ( y < m_y2 );
			}
			else
			{
				inBox &= ( y >= m_y2 ) && ( y < m_y1 );
			}
			
			if( inBox )
			{
				bool found = false;
				for( it2 = m_mouseover.begin(); it2 < m_mouseover.end(); it2++ )
					if( it2->entity == &(**it) )
					{
						found = true;
						it2->fade += ( m_fadeinrate + m_fadeoutrate ) * timestep;
						it2->isActive = true;
					}
				if( !found )
					m_mouseover.push_back( SMouseoverFader( *it, ( m_fadeinrate + m_fadeoutrate ) * timestep ) );
			}
		}
		delete( onscreen );
		
		for( it2 = m_mouseover.begin(); it2 < m_mouseover.end(); )
		{
			it2->fade -= m_fadeoutrate * timestep;
			if( it2->fade > m_fademaximum ) it2->fade = m_fademaximum;
			if( it2->fade < 0.0f )
			{
				it2 = m_mouseover.erase( it2 );
			}
			else
				it2++;
		}
	}
	else
	{
		std::vector<SMouseoverFader>::iterator it;
		bool found = false;

		for( it = m_mouseover.begin(); it < m_mouseover.end(); )
		{
			if( it->entity == m_target )
			{
				found = true;
				it->fade += m_fadeinrate * timestep;
				if( it->fade > m_fademaximum ) it->fade = m_fademaximum;
				it->isActive = true;
				it++; continue;
			}
			else
			{
				it->fade -= m_fadeoutrate * timestep;
				if( it->fade <= 0.0f )
				{
					it = m_mouseover.erase( it ); continue;
				}
				it++; continue;
			}
		}
		if( !found && (bool)m_target )
		{
			float initial = m_fadeinrate * timestep;
			if( initial > m_fademaximum ) initial = m_fademaximum;
			m_mouseover.push_back( SMouseoverFader( m_target, initial ) );
		}
	}
}

void CMouseoverEntities::addSelection()
{
	std::vector<SMouseoverFader>::iterator it;
	for( it = m_mouseover.begin(); it < m_mouseover.end(); it++ )
		if( it->isActive && !g_Selection.isSelected( it->entity ) )
			g_Selection.addSelection( it->entity );
}

void CMouseoverEntities::removeSelection()
{
	std::vector<SMouseoverFader>::iterator it;
	for( it = m_mouseover.begin(); it < m_mouseover.end(); it++ )
		if( it->isActive && g_Selection.isSelected( it->entity ) )
			g_Selection.removeSelection( it->entity );
}

void CMouseoverEntities::setSelection()
{
	g_Selection.clearSelection();
	addSelection();
}

void CMouseoverEntities::expandAcrossScreen()
{
	std::vector<HEntity>* activeset = g_EntityManager.matches( isMouseoverType, isOnScreen );
	m_mouseover.clear();
	std::vector<HEntity>::iterator it;
	for( it = activeset->begin(); it < activeset->end(); it++ )
		if( (*it)->m_extant )
			m_mouseover.push_back( SMouseoverFader( *it ) );
	delete( activeset );
}

void CMouseoverEntities::expandAcrossWorld()
{
	std::vector<HEntity>* activeset = g_EntityManager.matches( isMouseoverType );
	m_mouseover.clear();
	std::vector<HEntity>::iterator it;
	for( it = activeset->begin(); it < activeset->end(); it++ )
		if( (*it)->m_extant )
			m_mouseover.push_back( SMouseoverFader( *it ) );
	delete( activeset );
}

void CMouseoverEntities::renderSelectionOutlines()
{
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_BLEND );

	std::vector<SMouseoverFader>::iterator it;
	for( it = m_mouseover.begin(); it < m_mouseover.end(); it++ )
		it->entity->renderSelectionOutline( it->fade );

	glDisable( GL_BLEND );
}

void CMouseoverEntities::renderOverlays()
{
	CCamera *pCamera=g_Game->GetView()->GetCamera();
	CTerrain *pTerrain=g_Game->GetWorld()->GetTerrain();

	glLoadIdentity();
	glDisable( GL_TEXTURE_2D );
	if( m_bandbox )
	{
		//glPushMatrix();
		glColor3f( 1.0f, 1.0f, 1.0f );
		glBegin( GL_LINE_LOOP );
		glVertex2i( m_x1, g_Renderer.GetHeight() - m_y1 );
		glVertex2i( m_x2, g_Renderer.GetHeight() - m_y1 );
		glVertex2i( m_x2, g_Renderer.GetHeight() - m_y2 );
		glVertex2i( m_x1, g_Renderer.GetHeight() - m_y2 );
		glEnd();
		//glPopMatrix();
	}
	glEnable( GL_TEXTURE_2D );
	
	std::vector<SMouseoverFader>::iterator it;
	for( it = m_mouseover.begin(); it < m_mouseover.end(); it++ )
	{
		if( it->entity->m_grouped != -1 )
		{
			if( !it->entity->m_bounds ) continue;
			glPushMatrix();
			glEnable( GL_TEXTURE_2D );
			glLoadIdentity();
			float x, y;
			CVector3D labelpos = it->entity->m_graphics_position - pCamera->m_Orientation.GetLeft() * it->entity->m_bounds->m_radius;
#ifdef SELECTION_TERRAIN_CONFORMANCE
			labelpos.Y = pTerrain->getExactGroundLevel( labelpos.X, labelpos.Z );
#endif
			pCamera->GetScreenCoordinates( labelpos, x, y );
			glColor4f( 1.0f, 1.0f, 1.0f, it->fade );
			glTranslatef( x, g_Renderer.GetHeight() - y, 0.0f );
			glScalef( 1.0f, -1.0f, 1.0f );
			glwprintf( L"%d", (i32) it->entity->m_grouped );
			glDisable( GL_TEXTURE_2D );
			glPopMatrix();
		}
	}
}

void CMouseoverEntities::startBandbox( u16 x, u16 y )
{
	m_bandbox = true;
	m_x1 = x; m_y1 = y;
}

void CMouseoverEntities::stopBandbox()
{
	m_bandbox = false;
}

int interactInputHandler( const SDL_Event* ev )
{
	if (!g_active || !g_Game)
		return EV_PASS;

	CGameView *pView=g_Game->GetView();
	CCamera *pCamera=pView->GetCamera();
	CTerrain *pTerrain=g_Game->GetWorld()->GetTerrain();

	static float lastclicktime = 0.0f;
	static HEntity lastclickobject;
	static u8 clicks = 0;

	static u16 button_down_x, button_down_y;
	static bool button_down = false;

	switch( ev->type )
	{	
	case SDL_HOTKEYDOWN:
		switch( ev->user.code )
		{
		case HOTKEY_HIGHLIGHTALL:
			g_Mouseover.m_viewall = true;
			break;
		case HOTKEY_SELECTION_SNAP:
			if( g_Selection.m_selected.size() )
				pView->SetCameraTarget( g_Selection.getSelectionPosition() );
			break;
		case HOTKEY_CONTEXTORDER_NEXT:
			g_Selection.nextContext();
			break;
		case HOTKEY_CONTEXTORDER_PREVIOUS:
			g_Selection.previousContext();
			break;
		default:
			if( ( ev->user.code >= HOTKEY_SELECTION_GROUP_0 ) && ( ev->user.code <= HOTKEY_SELECTION_GROUP_19 ) )
			{
				// The above test limits it to 20 groups, so don't worry about overflowing

				i8 id = (i8)( ev->user.code - HOTKEY_SELECTION_GROUP_0 );
				
				if( hotkeys[HOTKEY_SELECTION_GROUP_ADD] )
				{
					g_Selection.addGroup( id );
				}
				else if( hotkeys[HOTKEY_SELECTION_GROUP_SAVE] )
				{
					g_Selection.saveGroup( id );
				}
				else if( hotkeys[HOTKEY_SELECTION_GROUP_SNAP] )
				{
					g_Selection.highlightGroup( id );
				}
				else
				{
					if( ( g_Selection.m_group == id ) && g_Selection.getGroupCount( id ) )
					{
						pView->SetCameraTarget( g_Selection.getGroupPosition( id ) );
					}
					else
						g_Selection.loadGroup( id );
				}
				return( EV_HANDLED );
			}
		
			return( EV_PASS );
		}
		return( EV_HANDLED );
	case SDL_HOTKEYUP:
		switch( ev->user.code )
		{
		case HOTKEY_SELECTION_GROUP_SNAP:
			if( g_Selection.m_group_highlight != -1 )
				g_Selection.highlightNone();
			break;
		case HOTKEY_HIGHLIGHTALL:
			g_Mouseover.m_viewall = false;
			break;
		default:
			return( EV_PASS );
		}
		return( EV_HANDLED );
	case SDL_MOUSEBUTTONUP:
		switch( ev->button.button )
		{
		case SDL_BUTTON_LEFT:
			if( g_Mouseover.m_viewall )
				break;
			float time;
			time = (float)get_time();
			// Reset clicks counter if too slow or if the cursor's
			// hovering over something else now.
			
			if( time - lastclicktime >= SELECT_DBLCLICK_RATE )
				clicks = 0;
			if( g_Mouseover.m_target != lastclickobject )
				clicks = 0;
			clicks++;

			if( clicks == 2 )
			{
				// Double click
				g_Mouseover.expandAcrossScreen();
			}
			else if( clicks == 3 )
			{
				// Triple click
				g_Mouseover.expandAcrossWorld();
			}
			lastclicktime = time;
			lastclickobject = g_Mouseover.m_target;

			button_down = false;
			g_Mouseover.stopBandbox();
			if( hotkeys[HOTKEY_SELECTION_ADD] )
			{
				g_Mouseover.addSelection();
			}
			else if( hotkeys[HOTKEY_SELECTION_REMOVE] )
			{
				g_Mouseover.removeSelection();
			}
			else
				g_Mouseover.setSelection();
			break;
		case SDL_BUTTON_RIGHT:
			g_Selection.contextOrder( hotkeys[HOTKEY_ORDER_QUEUE] );
			break;
		}
		break;
	case SDL_MOUSEBUTTONDOWN:
		switch( ev->button.button )
		{
		case SDL_BUTTON_LEFT:
			button_down = true;
			button_down_x = ev->button.x;
			button_down_y = ev->button.y;
			break;
		}
		break;
	case SDL_MOUSEMOTION:
		if( !g_Mouseover.isBandbox() && button_down )
		{
			int deltax = ev->motion.x - button_down_x;
			int deltay = ev->motion.y - button_down_y;
			if( ABS( deltax ) > 2 || ABS( deltay ) > 2 )
				g_Mouseover.startBandbox( button_down_x, button_down_y );
		}
		break;
	}
	return( EV_PASS );
}

bool isOnScreen( CEntity* ev )
{
	CCamera *pCamera=g_Game->GetView()->GetCamera();

	CFrustum frustum = pCamera->GetFrustum();
	return( frustum.IsBoxVisible( CVector3D(), ev->m_actor->GetModel()->GetBounds() ) );
}

bool isMouseoverType( CEntity* ev )
{
	std::vector<SMouseoverFader>::iterator it;
	for( it = g_Mouseover.m_mouseover.begin(); it < g_Mouseover.m_mouseover.end(); it++ )
	{
		if( it->isActive && ( (CBaseEntity*)it->entity->m_base == (CBaseEntity*)ev->m_base ) )
			return( true );
	}
	return( false );
}

/*
void pushCameraTarget( const CVector3D& target )
{
	// Save the current position
	cameraTargets.push_back( g_Camera.m_Orientation.GetTranslation() );
	// And set the camera
	setCameraTarget( target );
}

void setCameraTarget( const CVector3D& target )
{
	// Maintain the same orientation and level of zoom, if we can
	// (do this by working out the point the camera is looking at, saving
	//  the difference beteen that position and the camera point, and restoring
	//  that difference to our new target)

	cameraDelta = target - g_Camera.GetFocus();
}

void popCameraTarget()
{
	cameraDelta = cameraTargets.back() - g_Camera.m_Orientation.GetTranslation();
	cameraTargets.pop_back();
}
*/
