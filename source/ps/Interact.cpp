#include "precompiled.h"

#include "CLogger.h"

#include "Interact.h"
#include "Renderer.h"
#include "input.h"
#include "CConsole.h"
#include "HFTracer.h"
#include "Hotkey.h"
#include "gui/CGUI.h"
#include "gui/MiniMap.h"
#include "timer.h"
#include "Game.h"
#include "ps/Globals.h"
#include "ps/VFSUtil.h"
#include "Network/NetMessage.h"
#include "BoundingObjects.h"
#include "Unit.h"
#include "Model.h"
#include "simulation/BaseEntityCollection.h"
#include "simulation/EntityFormation.h"
#include "simulation/FormationManager.h"
#include "scripting/GameEvents.h"
#include "UnitManager.h"
#include "MathUtil.h"
#include "graphics/GameView.h"

extern CConsole* g_Console;
extern CStr g_CursorName;
extern float g_xres, g_yres;

static const double SELECT_DBLCLICK_RATE = 0.5;
const int ORDER_DELAY = 5;

bool customSelectionMode=false;

void CSelectedEntities::addSelection( HEntity entity )
{
	m_group = -1;
	debug_assert( !isSelected( entity ) );
	m_selected.push_back( entity );
	entity->m_selected = true;
	m_selectionChanged = true;
}

void CSelectedEntities::removeSelection( HEntity entity )
{
	m_group = -1;
	debug_assert( isSelected( entity ) );
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

void CSelectedEntities::renderHealthBars()
{
	std::vector<HEntity>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		(*it)->renderHealthBar();

	if( m_group_highlight != -1 )
	{
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glEnable( GL_BLEND );

		std::vector<HEntity>::iterator it;
		for( it = m_groups[m_group_highlight].begin(); it < m_groups[m_group_highlight].end(); it++ )
			(*it)->renderHealthBar();


		glDisable( GL_BLEND );
	}
}
void CSelectedEntities::renderStaminaBars()
{
	std::vector<HEntity>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		(*it)->renderStaminaBar();

	if( m_group_highlight != -1 )
	{
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glEnable( GL_BLEND );

		std::vector<HEntity>::iterator it;
		for( it = m_groups[m_group_highlight].begin(); it < m_groups[m_group_highlight].end(); it++ )
			(*it)->renderStaminaBar();

		glDisable( GL_BLEND );
	}
}
void CSelectedEntities::renderRanks()
{
	std::vector<HEntity>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		(*it)->renderRank();

	if( m_group_highlight != -1 )
	{
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glEnable( GL_BLEND );

		std::vector<HEntity>::iterator it;
		for( it = m_groups[m_group_highlight].begin(); it < m_groups[m_group_highlight].end(); it++ )
			(*it)->renderRank();

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

	/*
	glLoadIdentity();
	glTranslatef( (float)( g_mouse_x + 16 ), (float)( g_Renderer.GetHeight() - g_mouse_y - 8 ), 0.0f );
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
	case CEntityOrder::ORDER_GATHER:
		glwprintf( L"Gather" );
		break;
	}
	*/

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

	if( groupid >= MAX_GROUPS )
	{
		debug_warn( "Invalid group id" );
		return;
	}

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
	static std::vector<HEntity> lastSelection;
	
	// Drop out immediately if we're in some special interaction mode
	if (customSelectionMode)
		return;

	if( !( m_selected == lastSelection ) )
	{
		g_JSGameEvents.FireSelectionChanged( m_selectionChanged );
		lastSelection = m_selected; 
	}

	if( m_selectionChanged || g_Mouseover.m_targetChanged )
	{
		// Can't order anything off the map
		if( !g_Game->GetWorld()->GetTerrain()->isOnMap( g_Mouseover.m_worldposition ) )
		{
			m_defaultCommand = -1;
			m_secondaryCommand = -1;
			return;
		}
		
		// Quick count to see which is the modal default order.

		const int numCommands=NMT_COMMAND_LAST - NMT_COMMAND_FIRST;
		int defaultPoll[numCommands];
		std::map<CStrW, int, CStrW_hash_compare> defaultCursor[numCommands];
		std::map<int, int> defaultAction[numCommands];

		int secondaryPoll[numCommands];
		std::map<CStrW, int, CStrW_hash_compare> secondaryCursor[numCommands];
		std::map<int, int> secondaryAction[numCommands];

		int t, vote, secvote;
		for( t = 0; t < numCommands; t++ )
		{
			defaultPoll[t] = 0;
			secondaryPoll[t] = 0;
		}

		std::vector<HEntity>::iterator it;
		for( it = m_selected.begin(); it < m_selected.end(); it++ )
		{
			CEventTargetChanged evt( g_Mouseover.m_target );
			(*it)->DispatchEvent( &evt );
			vote = evt.m_defaultOrder - NMT_COMMAND_FIRST;
			secvote = evt.m_secondaryOrder - NMT_COMMAND_FIRST;
			
			if( ( vote >= 0 ) && ( vote < numCommands ) )
			{
				defaultPoll[vote]++;
				defaultCursor[vote][evt.m_defaultCursor]++;
				defaultAction[vote][evt.m_defaultAction]++;
			}
			if( ( secvote >= 0 ) && ( secvote < numCommands ) )
			{
				secondaryPoll[secvote]++;
				secondaryCursor[secvote][evt.m_secondaryCursor]++;
				secondaryAction[secvote][evt.m_secondaryAction]++;
			}
		}

		vote = -1;
		secvote = -1;
		for( t = 0; t < numCommands; t++ )
		{
			if( ( vote == -1 ) || ( defaultPoll[t] > defaultPoll[vote] ) )
				vote = t;
			if( ( secvote == -1 ) || ( secondaryPoll[t] > secondaryPoll[secvote] ) )
				secvote = t;
		}

		std::map<CStrW, int, CStrW_hash_compare>::iterator itv;
		std::map<int, int>::iterator iti;
		m_defaultCommand = vote + NMT_COMMAND_FIRST;
		m_secondaryCommand = secvote + NMT_COMMAND_FIRST;

		// Now find the most appropriate cursor
		t = 0;
		for( itv = defaultCursor[vote].begin(); itv != defaultCursor[vote].end(); itv++ )
		{
			if( itv->second > t )
			{
				t = itv->second;
				g_CursorName = itv->first;
			}
		}
		
		/*
		TODO:  provide secondary cursor name?

		t = 0;
		for( itv = secondaryCursor[secvote].begin(); itv != secondaryCursor[secvote].end(); itv++ )
		{
			if( itv->second > t )
			{
				t = itv->second;
				g_CursorName = itv->first;
			}
		}*/	

		// Find the most appropriate action parameter too
		t = 0;
		for( iti = defaultAction[vote].begin(); iti != defaultAction[vote].end(); iti++ )
		{
			if( iti->second > t )
			{
				t = iti->second;
				m_defaultAction = iti->first;
			}
		}
		
		t = 0;
		for( iti = secondaryAction[secvote].begin(); iti != secondaryAction[secvote].end(); iti++ )
		{
			if( iti->second > t )
			{
				t = iti->second;
				m_secondaryAction = iti->first;
			}
		}

		m_selectionChanged = false;
		g_Mouseover.m_targetChanged = false;
	}

	if( ( m_group_highlight != -1 ) && getGroupCount( m_group_highlight ) )
		g_Game->GetView()->SetCameraTarget( getGroupPosition( m_group_highlight ) );

}

void CMouseoverEntities::update( float timestep )
{
	CCamera *pCamera=g_Game->GetView()->GetCamera();
	//CTerrain *pTerrain=g_Game->GetWorld()->GetTerrain();

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
			if( (*it)->m_extant && ( (*it)->GetPlayer() == g_Game->GetLocalPlayer() ) )
				m_mouseover.push_back( SMouseoverFader( *it, m_fademaximum, false ) );

		delete( onscreen );
	}
	else if( m_bandbox )
	{
		m_x2 = g_mouse_x;
		m_y2 = g_mouse_y;
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

			// Can only bandbox units the local player controls.
			if( (*it)->GetPlayer() != g_Game->GetLocalPlayer() )
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
	// Rules for shift-click selection:

	// If selecting a non-allied unit, you can only select one. You can't 
	// select a mix of allied and non-allied units. Therefore:
	// Forbid shift-click of enemy units unless the selection is empty
	// Forbid shift-click of allied units if the selection contains one
	//   or more enemy units.

	if( ( m_mouseover.size() != 0 ) &&
		( m_mouseover.front().entity->GetPlayer() != g_Game->GetLocalPlayer() ) && 
		( g_Selection.m_selected.size() != 0 ) )
		return;
	
	if( ( g_Selection.m_selected.size() != 0 ) &&
		( g_Selection.m_selected.front()->GetPlayer() != g_Game->GetLocalPlayer() ) )
		return;

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
	std::vector<HEntity>* activeset = g_EntityManager.matches( 
		CEntityManager::EntityPredicateLogicalAnd<isMouseoverType,isOnScreen> );
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

void CMouseoverEntities::renderHealthBars()
{
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_BLEND );

	std::vector<SMouseoverFader>::iterator it;
	for( it = m_mouseover.begin(); it < m_mouseover.end(); it++ )
		it->entity->renderHealthBar();

	glDisable( GL_BLEND );
}

void CMouseoverEntities::renderStaminaBars()
{
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_BLEND );

	std::vector<SMouseoverFader>::iterator it;
	for( it = m_mouseover.begin(); it < m_mouseover.end(); it++ )
		it->entity->renderStaminaBar();

	glDisable( GL_BLEND );
}
void CMouseoverEntities::renderRanks()
{
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_BLEND );

	std::vector<SMouseoverFader>::iterator it;
	for( it = m_mouseover.begin(); it < m_mouseover.end(); it++ )
		it->entity->renderRank();

	glDisable( GL_BLEND );
}


int CSelectedEntities::loadRankTextures()
{
	VFSUtil::FileList ranklist;
	VFSUtil::FindFiles( "art/textures/ui/session/icons", 0, ranklist );
	for ( std::vector<CStr>::iterator it = ranklist.begin(); it != ranklist.end(); it++ )
	{
		const char* filename = it->c_str();
		if ( !tex_is_known_extension(filename) )
		{
			LOG(ERROR, "Unknown rank texture extension (%s)", filename);
			continue;
		}
		Handle ht = ogl_tex_load(filename);
		if (ht <= 0)
		{
			LOG(ERROR, "Rank Textures", "loadRankTextures failed on \"%s\"", filename);
			return ht;
		}
		m_rankTextures[it->AfterLast("/")] = ht;
		RETURN_ERR(ogl_tex_upload(ht));
	}
	return 0;
}
void CSelectedEntities::destroyRankTextures()
{
	for ( std::map<CStr, Handle>::iterator it=m_rankTextures.begin(); it != m_rankTextures.end(); it++ )
	{
		ogl_tex_free(it->second);
		it->second = 0;
	}
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

void FireWorldClickEvent(uint button, int clicks)
{
	//debug_printf("FireWorldClickEvent: button %d, clicks %d\n", button, clicks);
	//If we're clicking on the minimap, use its world click handler

	if ( g_Selection.m_mouseOverMM )
		return;
	
	g_JSGameEvents.FireWorldClick(
		button,
		clicks,
		g_Selection.m_defaultCommand,
		g_Selection.m_defaultAction,
		g_Selection.m_secondaryCommand, // FIXME Secondary order, depends entity scripts etc
		g_Selection.m_secondaryAction, // FIXME Secondary action, depends entity scripts etc
		g_Mouseover.m_target,
		(uint)g_Mouseover.m_worldposition.x,
		(uint)g_Mouseover.m_worldposition.y);
	
	//Reset duplication flag- after this, it isn't the same order
	std::vector<HEntity>::iterator it=g_Selection.m_selected.begin();
	for ( ; it != g_Selection.m_selected.end(); it++ )
	{
		if ( (*it)->m_formation >= 0)
		(*it)->GetFormation()->SetDuplication(false);
	}
}

void MouseButtonUpHandler(const SDL_Event *ev, int clicks)
{
	FireWorldClickEvent(ev->button.button, clicks);
	
	switch( ev->button.button )
	{
	case SDL_BUTTON_LEFT:
		if( g_BuildingPlacer.m_active )
		{
			g_BuildingPlacer.mouseReleased();
			break;
		}

		if (customSelectionMode)
			break;
	
		if( g_Mouseover.m_viewall )
			break;

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
		if( g_BuildingPlacer.m_active )
		{
			g_BuildingPlacer.deactivate();
			break;
		}
	}
}

InReaction interactInputHandler( const SDL_Event* ev )
{
	if (!g_app_has_focus || !g_Game)
		return IN_PASS;

	CGameView *pView=g_Game->GetView();
	//CCamera *pCamera=pView->GetCamera();
	//CTerrain *pTerrain=g_Game->GetWorld()->GetTerrain();

	// One entry for each mouse button
	// note: to store these in an array, we make assumptions as to the
	// SDL_BUTTON_* values; these are verified at compile time.
	cassert(SDL_BUTTON_LEFT == 1 && SDL_BUTTON_MIDDLE == 2 && SDL_BUTTON_RIGHT == 3 && \
	        SDL_BUTTON_WHEELUP == 4 && SDL_BUTTON_WHEELDOWN == 5);
	const uint SDL_BUTTON_INDEX_COUNT = 6;
	static double lastclicktime[SDL_BUTTON_INDEX_COUNT];
	static HEntity lastclickobject[SDL_BUTTON_INDEX_COUNT];
	static u8 clicks[SDL_BUTTON_INDEX_COUNT];

	ONCE(
		for (int i=0;i<SDL_BUTTON_INDEX_COUNT;i++)
		{
			lastclicktime[i] = 0.0f;
			clicks[i] = 0;
		});
	// These refer to the left mouse button
	static u16 button_down_x, button_down_y;
	static double button_down_time;
	static bool button_down = false;
	static bool right_button_down = false;
	
	if (customSelectionMode && ev->type != SDL_MOUSEBUTTONUP)
		return IN_PASS;
	
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
		case HOTKEY_CAMERA_UNIT_VIEW:
		{
			if ( pView->IsAttached() )
				break;	//Should only exit unit view through unit view hotkey
			if ( pView->IsUnitView() )
			{
				//If already in unit view, exit it
				pView->ToUnitView( NULL, NULL );
				break;
			}
			if ( g_Selection.m_selected.empty() )
				break;

			std::vector<CModel::Prop>& Props = g_Selection.m_selected.front()->m_actor->GetModel()->GetProps();
			
			for (size_t x=0; x<Props.size(); x++)
			{
				if( Props[x].m_Point->m_Name == "head" )
				{
					pView->ToUnitView( g_Selection.m_selected.front(), Props[x].m_Model);
					break;
				}
			}
			break;
		}
		case HOTKEY_CAMERA_UNIT_ATTACH:
		{
			if ( pView->IsUnitView() )
				break;	//Should only exit unit view through unit view hotkey
			if ( pView->IsAttached() )
			{
				//If already in unit view, exit it
				pView->AttachToUnit( NULL );
				break;
			}
			if ( g_Selection.m_selected.empty() )
				break;
			
			pView->AttachToUnit( g_Selection.m_selected.front() );
			break;
		}

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
				return( IN_HANDLED );
			}

			return( IN_PASS );
		}
		return( IN_HANDLED );
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
			return( IN_PASS );
		}
		return( IN_HANDLED );
	case SDL_MOUSEBUTTONUP:
	{
		int button = ev->button.button;
		// Only process buttons within the range for which we have button state
		// arrays above.
		if (button >= 0 && button < SDL_BUTTON_INDEX_COUNT)
		{
			double time = get_time();
			// Reset clicks counter if too slow or if the cursor's
			// hovering over something else now.
			
			if( time - lastclicktime[button] >= SELECT_DBLCLICK_RATE )
				clicks[button] = 0;
			if( g_Mouseover.m_target != lastclickobject[button] )
				clicks[button] = 0;
			clicks[button]++;

			lastclicktime[button] = time;
			lastclickobject[button] = g_Mouseover.m_target;

			if(ev->button.button == SDL_BUTTON_LEFT )
				button_down = false;
			if(ev->button.button == SDL_BUTTON_RIGHT )
				right_button_down = false;

			MouseButtonUpHandler(ev, clicks[button]);
		}
		break;
	}
	case SDL_MOUSEBUTTONDOWN:
		switch( ev->button.button )
		{
		case SDL_BUTTON_LEFT:
			button_down = true;
			button_down_x = ev->button.x;
			button_down_y = ev->button.y;
			button_down_time = get_time();
			if( g_BuildingPlacer.m_active )
			{
				g_BuildingPlacer.mousePressed();
			}
			break;
		case SDL_BUTTON_RIGHT:
			right_button_down = true;
		}
		break;
	case SDL_MOUSEMOTION:
		if( !g_Mouseover.isBandbox() && button_down && !g_BuildingPlacer.m_active && !right_button_down )
		{
			int deltax = ev->motion.x - button_down_x;
			int deltay = ev->motion.y - button_down_y;
			if( ABS( deltax ) > 2 || ABS( deltay ) > 2 )
				g_Mouseover.startBandbox( button_down_x, button_down_y );
		}
		break;
	}
	return( IN_PASS );
}

bool isOnScreen( CEntity* ev, void* UNUSED(userdata) )
{
	CCamera *pCamera=g_Game->GetView()->GetCamera();

	CFrustum frustum = pCamera->GetFrustum();

	if( ev->m_actor )
		return( frustum.IsBoxVisible( CVector3D(), ev->m_actor->GetModel()->GetBounds() ) );
	else
		// If there's no actor, just treat the entity as a point
		return( frustum.IsBoxVisible( ev->m_graphics_position, CBound() ) );
}

bool isMouseoverType( CEntity* ev, void* UNUSED(userdata) )
{
	std::vector<SMouseoverFader>::iterator it;
	for( it = g_Mouseover.m_mouseover.begin(); it < g_Mouseover.m_mouseover.end(); it++ )
	{
		if( it->isActive && ( (CBaseEntity*)it->entity->m_base == (CBaseEntity*)ev->m_base )
				&& ( it->entity->GetPlayer() == ev->GetPlayer() ) )
			return( true );
	}
	return( false );
}

void StartCustomSelection()
{
	customSelectionMode = true;
}

void ResetInteraction()
{
	customSelectionMode = false;
}


bool CBuildingPlacer::activate(CStrW& templateName)
{
	if(m_active)
	{
		return false;
	}

	m_templateName = templateName;
	m_active = true;
	m_clicked = false;
	m_dragged = false;
	m_angle = 0;
	m_timeSinceClick = 0;
	m_totalTime = 0;
	m_valid = false;

	CBaseEntity* base = g_EntityTemplateCollection.getTemplate( m_templateName );

	if( !base )
	{
		deactivate();
		return false;
	}

	// m_actor
	CStr actorName ( base->m_actorName );	// convert CStrW->CStr8

	std::set<CStrW> selections;
	m_actor = g_UnitMan.CreateUnit( actorName, 0, selections );
	m_actor->SetPlayerID(g_Game->GetLocalPlayer()->GetPlayerID());
	
	// m_bounds
	if( base->m_bound_type == CBoundingObject::BOUND_CIRCLE )
	{
 		m_bounds = new CBoundingCircle( 0, 0, base->m_bound_circle );
	}
	else if( base->m_bound_type == CBoundingObject::BOUND_OABB )
	{
		m_bounds = new CBoundingBox( 0, 0, CVector2D(1, 0), base->m_bound_box );
	}

	return true;
}

void CBuildingPlacer::mousePressed()
{
	CCamera &g_Camera=*g_Game->GetView()->GetCamera();
	clickPos = g_Camera.GetWorldCoordinates();
	m_clicked = true;
}

void CBuildingPlacer::mouseReleased()
{
	deactivate();	// do it first in case we fail for any reason

	if(m_valid) 
	{
		HEntity ent = g_EntityManager.createFoundation( m_templateName, clickPos, m_angle );
		ent->SetPlayer(g_Game->GetLocalPlayer());
	}
}

void CBuildingPlacer::deactivate()
{
	m_active = false;
	g_UnitMan.RemoveUnit( m_actor );
	delete m_actor;
	m_actor = 0;
	delete m_bounds;
	m_bounds = 0;
}

void CBuildingPlacer::update( float timeStep )
{
	if(!m_active)
		return;

	m_totalTime += timeStep;

	if(m_clicked)
	{
		m_timeSinceClick += timeStep;
		CCamera &g_Camera=*g_Game->GetView()->GetCamera();
		CVector3D mousePos = g_Camera.GetWorldCoordinates();
		CVector3D dif = mousePos - clickPos;
		float x = dif.X, z = dif.Z;
		if(x*x + z*z < 3*3) {
			if(m_dragged || m_timeSinceClick > 0.2f) 
			{
				m_angle += timeStep * PI;
			}
		}
		else
		{
			m_dragged = true;
			m_angle = atan2(x, z);
		}
	}

	CVector3D pos;
	if(m_clicked) 
	{
		pos = clickPos;
	}
	else
	{
		CCamera &g_Camera=*g_Game->GetView()->GetCamera();
		pos = g_Camera.GetWorldCoordinates();
	}

	CMatrix3D m;
	float s = sin( m_angle );
	float c = cos( m_angle );
	m._11 = -c;		m._12 = 0.0f;	m._13 = -s;		m._14 = pos.X;
	m._21 = 0.0f;	m._22 = 1.0f;	m._23 = 0.0f;	m._24 = pos.Y;
	m._31 = s;		m._32 = 0.0f;	m._33 = -c;		m._34 = pos.Z;
	m._41 = 0.0f;	m._42 = 0.0f;	m._43 = 0.0f;	m._44 = 1.0f;
	m_actor->GetModel()->SetTransform( m );

	m_bounds->setPosition(pos.X, pos.Z);

	if(m_bounds->m_type == CBoundingObject::BOUND_OABB) 
	{
		CBoundingBox* box = (CBoundingBox*) m_bounds;
		box->setOrientation(m_angle);
	}

	CTerrain *pTerrain=g_Game->GetWorld()->GetTerrain();
	m_valid = pTerrain->isOnMap(pos.X, pos.Z) && getCollisionObject(m_bounds)==0;

	CColor col;
	if(m_valid)
	{
		col = CColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
	else 
	{
		float add = (sin(4*PI*m_totalTime) + 1.0f) * 0.08f;
		col = CColor(1.4f+add, 0.4f+add, 0.4f+add, 1.0f);
	}
	m_actor->GetModel()->SetShadingColor(col);
}

void CBuildingPlacer::render()
{
	// do nothing special - our CUnit will get rendered by the UnitManager

	/*if(!m_active)
		return;

	CVector3D pos;
	if(m_clicked) 
	{
		pos = clickPos;
	}
	else
	{
		CCamera &g_Camera=*g_Game->GetView()->GetCamera();
		pos = g_Camera.GetWorldCoordinates();
	}

	glBegin(GL_LINE_STRIP);
	glColor3f(1,1,1);
	glVertex3f(pos.X, pos.Y+6, pos.Z);
	glColor3f(1,1,1);
	glVertex3f(pos.X, pos.Y, pos.Z);
	glColor3f(1,1,1);
	glVertex3f(pos.X + 3*sin(m_angle), pos.Y, pos.Z + 3*cos(m_angle));
	glEnd();*/
}
