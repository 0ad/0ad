/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "Interact.h"

#include "graphics/GameView.h"
#include "graphics/HFTracer.h"
#include "graphics/Model.h"
#include "graphics/ModelDef.h"
#include "graphics/Terrain.h"
#include "graphics/Unit.h"
#include "graphics/UnitManager.h"
#include "gui/CGUI.h"
#include "gui/MiniMap.h"
#include "lib/input.h"
#include "lib/res/graphics/unifont.h"
#include "lib/timer.h"
#include "maths/MathUtil.h"
#include "network/NetMessage.h"
#include "ps/Game.h"
#include "ps/Globals.h"
#include "ps/Hotkey.h"
#include "ps/Player.h"
#include "ps/Filesystem.h"
#include "ps/World.h"
#include "renderer/Renderer.h"
#include "scripting/GameEvents.h"
#include "scripting/ScriptableComplex.inl"
#include "simulation/BoundingObjects.h"
#include "simulation/Collision.h"
#include "simulation/Entity.h"
#include "simulation/EntityFormation.h"
#include "simulation/EntityManager.h"
#include "simulation/EntityTemplate.h"
#include "simulation/EntityTemplateCollection.h"
#include "simulation/EventHandlers.h"
#include "simulation/FormationManager.h"
#include "simulation/Simulation.h"
#include "simulation/TerritoryManager.h"

#include "ps/CLogger.h"
#define LOG_CATEGORY "world"

extern CStr g_CursorName;
extern float g_xres, g_yres;

static const double SELECT_DBLCLICK_RATE = 0.5;
const int ORDER_DELAY = 5;

bool customSelectionMode=false;

void CSelectedEntities::AddSelection( HEntity entity )
{
	m_group = -1;
	debug_assert( !IsSelected( entity ) );
	m_selected.push_back( entity );
	entity->m_selected = true;
	m_selectionChanged = true;
}

void CSelectedEntities::RemoveSelection( HEntity entity )
{
	m_group = -1;
	debug_assert( IsSelected( entity ) );
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

void CSelectedEntities::RenderSelectionOutlines()
{
	std::vector<HEntity>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		(*it)->RenderSelectionOutline();

	if( m_group_highlight != -1 )
	{
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glEnable( GL_BLEND );

		std::vector<HEntity>::iterator it;
		for( it = m_groups[m_group_highlight].begin(); it < m_groups[m_group_highlight].end(); it++ )
			(*it)->RenderSelectionOutline( 0.5f );

		glDisable( GL_BLEND );
	}
}

void CSelectedEntities::RenderBars()
{
	std::vector<HEntity>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		(*it)->RenderBars();

	/*if( m_group_highlight != -1 )
	{
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glEnable( GL_BLEND );

		std::vector<HEntity>::iterator it;
		for( it = m_groups[m_group_highlight].begin(); it < m_groups[m_group_highlight].end(); it++ )
			(*it)->RenderBars();


		glDisable( GL_BLEND );
	}*/
}
void CSelectedEntities::RenderAuras()
{
	std::vector<HEntity>::iterator it;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	for ( it = m_selected.begin(); it != m_selected.end(); ++it )
		(*it)->RenderAuras();
}
void CSelectedEntities::RenderHealthBars()
{
	std::vector<HEntity>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		(*it)->RenderHealthBar();

	if( m_group_highlight != -1 )
	{
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glEnable( GL_BLEND );

		std::vector<HEntity>::iterator it;
		for( it = m_groups[m_group_highlight].begin(); it < m_groups[m_group_highlight].end(); it++ )
			(*it)->RenderHealthBar();


		glDisable( GL_BLEND );
	}
}
void CSelectedEntities::RenderStaminaBars()
{
	std::vector<HEntity>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		(*it)->RenderStaminaBar();

	if( m_group_highlight != -1 )
	{
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glEnable( GL_BLEND );

		std::vector<HEntity>::iterator it;
		for( it = m_groups[m_group_highlight].begin(); it < m_groups[m_group_highlight].end(); it++ )
			(*it)->RenderStaminaBar();

		glDisable( GL_BLEND );
	}
}
void CSelectedEntities::RenderBarBorders()
{
	std::vector<HEntity>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		(*it)->RenderBarBorders();

	if( m_group_highlight != -1 )
	{
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glEnable( GL_BLEND );

		std::vector<HEntity>::iterator it;
		for( it = m_groups[m_group_highlight].begin(); it < m_groups[m_group_highlight].end(); it++ )
			(*it)->RenderBarBorders();

		glDisable( GL_BLEND );
	}
}

void CSelectedEntities::RenderRanks()
{
	std::vector<HEntity>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		(*it)->RenderRank();

	if( m_group_highlight != -1 )
	{
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glEnable( GL_BLEND );

		std::vector<HEntity>::iterator it;
		for( it = m_groups[m_group_highlight].begin(); it < m_groups[m_group_highlight].end(); it++ )
			(*it)->RenderRank();

		glDisable( GL_BLEND );
	}
}


void CSelectedEntities::RenderOverlays()
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
			labelpos.Y = pTerrain->GetExactGroundLevel( labelpos.X, labelpos.Z );
#endif
			pCamera->GetScreenCoordinates( labelpos, x, y );
			glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
			glTranslatef( x, g_Renderer.GetHeight() - y, 0.0f );
			glScalef( 1.0f, -1.0f, 1.0f );
			glwprintf( L"%d", (*it)->m_grouped );

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
			labelpos.Y = pTerrain->GetExactGroundLevel( labelpos.X, labelpos.Z );
#endif
			pCamera->GetScreenCoordinates( labelpos, x, y );
			glColor4f( 1.0f, 1.0f, 1.0f, 0.5f );
			glTranslatef( x, g_Renderer.GetHeight() - y, 0.0f );
			glScalef( 1.0f, -1.0f, 1.0f );
			glwprintf( L"%d", (*it)->m_grouped );
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
void CSelectedEntities::RenderRallyPoints()
{
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_BLEND );

	std::vector<HEntity>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		(*it)->RenderRallyPoint();

	if( m_group_highlight != -1 )
	{
		std::vector<HEntity>::iterator it;
		for( it = m_groups[m_group_highlight].begin(); it < m_groups[m_group_highlight].end(); it++ )
			(*it)->RenderRallyPoint();
	}
	glDisable( GL_BLEND );
}
void CSelectedEntities::SetSelection( HEntity entity )
{
	m_group = -1;
	ClearSelection();
	m_selected.push_back( entity );
}

void CSelectedEntities::ClearSelection()
{
	m_group = -1;
	std::vector<HEntity>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		(*it)->m_selected = false;
	m_selected.clear();
	m_selectionChanged = true;
}

void CSelectedEntities::RemoveAll( HEntity entity )
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

CVector3D CSelectedEntities::GetSelectionPosition()
{
	CVector3D avg;
	std::vector<HEntity>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		avg += (*it)->m_graphics_position;
	return( avg * ( 1.0f / m_selected.size() ) );
}

void CSelectedEntities::SaveGroup( i8 groupid )
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

void CSelectedEntities::AddToGroup( i8 groupid, HEntity entity )
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

void CSelectedEntities::LoadGroup( i8 groupid )
{
	if( m_group == groupid )
		return;

	if( groupid >= MAX_GROUPS )
	{
		debug_warn( "Invalid group id" );
		return;
	}

	ClearSelection();
	m_selected = m_groups[groupid];

	std::vector<HEntity>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		(*it)->m_selected = true;
	m_group = groupid;

	m_selectionChanged = true;
}

void CSelectedEntities::AddGroup( i8 groupid )
{
	std::vector<HEntity>::iterator it;
	for( it = m_groups[groupid].begin(); it < m_groups[groupid].end(); it++ )
	{
		if( !IsSelected( *it ) )
			AddSelection( *it );
	}
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		(*it)->m_selected = true;
}

void CSelectedEntities::ChangeGroup( HEntity entity, i8 groupid )
{
	// Remove from current group
	ssize_t current = entity->m_grouped;
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

bool CSelectedEntities::IsSelected( HEntity entity )
{
	std::vector<HEntity>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
	{
		if( (*it) == entity )
			return( true );
	}
	return( false );
}

void CSelectedEntities::HighlightGroup( i8 groupid )
{
	if( m_group_highlight != -1 )
		return;
	if( !GetGroupCount( groupid ) )
		return;
	m_group_highlight = groupid;
	g_Game->GetView()->PushCameraTarget( GetGroupPosition( groupid ) );
}

void CSelectedEntities::HighlightNone()
{

	if( m_group_highlight != -1 )
		g_Game->GetView()->PopCameraTarget();
	m_group_highlight = -1;

}

int CSelectedEntities::GetGroupCount( i8 groupid )
{
	return( (int)m_groups[groupid].size() );
}

CVector3D CSelectedEntities::GetGroupPosition( i8 groupid )
{
	CVector3D avg;
	std::vector<HEntity>::iterator it;
	for( it = m_groups[groupid].begin(); it < m_groups[groupid].end(); it++ )
		avg += (*it)->m_graphics_position;
	return( avg * ( 1.0f / m_groups[groupid].size() ) );
}

void CSelectedEntities::Update()
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
		if( !g_Game->GetWorld()->GetTerrain()->IsOnMap( g_Mouseover.m_worldposition ) )
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

		// Don't count GOTO as a majority action unless everything else has 0 votes.
		defaultPoll[NMT_GOTO - NMT_COMMAND_FIRST] = 0;

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

	if( ( m_group_highlight != -1 ) && GetGroupCount( m_group_highlight ) )
		g_Game->GetView()->SetCameraTarget( GetGroupPosition( m_group_highlight ) );

}

void CMouseoverEntities::Update( float timestep )
{
	CCamera *pCamera=g_Game->GetView()->GetCamera();
	//CTerrain *pTerrain=g_Game->GetWorld()->GetTerrain();

	CVector3D origin, dir;
	pCamera->BuildCameraRay( origin, dir );

	CUnit* hit = g_Game->GetWorld()->GetUnitManager().PickUnit( origin, dir, true );

	m_worldposition = pCamera->GetWorldCoordinates(true);

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

		std::vector<HEntity> onscreen;
		g_EntityManager.GetMatchingAsHandles( onscreen, IsOnScreen );
		for(std::vector<HEntity>::iterator it = onscreen.begin(); it < onscreen.end(); it++ )
		{
			HEntity entity = *it;
			if( entity->m_extant && ( entity->GetPlayer() == g_Game->GetLocalPlayer() ) )
				m_mouseover.push_back( SMouseoverFader( entity, m_fademaximum, false ) );
		}
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
		std::vector<HEntity> onscreen;
		g_EntityManager.GetMatchingAsHandles( onscreen, IsOnScreen );

		// Reset active flags on everything...
		for(std::vector<SMouseoverFader>::iterator it2 = m_mouseover.begin(); it2 < m_mouseover.end(); it2++ )
			it2->isActive = false;

		for(std::vector<HEntity>::iterator it = onscreen.begin(); it < onscreen.end(); it++ )
		{
			HEntity entity = *it;
			if( !entity->m_extant )
				continue;

			// Can only bandbox units the local player controls.
			if( entity->GetPlayer() != g_Game->GetLocalPlayer() )
				continue;

			CVector3D worldspace = entity->m_graphics_position;

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
				for(std::vector<SMouseoverFader>::iterator it2 = m_mouseover.begin(); it2 < m_mouseover.end(); it2++ )
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

		for(std::vector<SMouseoverFader>::iterator it2 = m_mouseover.begin(); it2 < m_mouseover.end(); )
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
		bool found = false;

		for(std::vector<SMouseoverFader>::iterator it = m_mouseover.begin(); it < m_mouseover.end(); )
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

void CMouseoverEntities::AddSelection()
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
		if( it->isActive && !g_Selection.IsSelected( it->entity ) )
			g_Selection.AddSelection( it->entity );
}

void CMouseoverEntities::RemoveSelection()
{
	std::vector<SMouseoverFader>::iterator it;
	for( it = m_mouseover.begin(); it < m_mouseover.end(); it++ )
		if( it->isActive && g_Selection.IsSelected( it->entity ) )
			g_Selection.RemoveSelection( it->entity );
}

void CMouseoverEntities::SetSelection()
{
	g_Selection.ClearSelection();
	AddSelection();
}

void CMouseoverEntities::ExpandAcrossScreen()
{
	std::vector<HEntity> activeset;
	g_EntityManager.GetMatchingAsHandles(
		activeset,
		CEntityManager::EntityPredicateLogicalAnd<IsMouseoverType,IsOnScreen>
	);
	m_mouseover.clear();
	for(std::vector<HEntity>::iterator it = activeset.begin(); it < activeset.end(); it++ )
	{
		HEntity entity = *it;
		if( entity->m_extant )
			m_mouseover.push_back( SMouseoverFader( entity ) );
	}
}

void CMouseoverEntities::ExpandAcrossWorld()
{
	std::vector<HEntity> activeset;
	g_EntityManager.GetMatchingAsHandles( activeset, IsMouseoverType );
	m_mouseover.clear();
	for(std::vector<HEntity>::iterator it = activeset.begin(); it < activeset.end(); it++ )
	{
		HEntity entity = *it;
		if( entity->m_extant )
			m_mouseover.push_back( SMouseoverFader( entity ) );
	}
}

void CMouseoverEntities::RenderSelectionOutlines()
{
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_BLEND );

	std::vector<SMouseoverFader>::iterator it;
	for( it = m_mouseover.begin(); it < m_mouseover.end(); it++ )
	{
		if( !g_Selection.IsSelected(it->entity) )
			it->entity->RenderSelectionOutline( it->fade );
	}

	glDisable( GL_BLEND );
}
void CMouseoverEntities::RenderAuras()
{
	std::vector<SMouseoverFader>::iterator it;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	for ( it = m_mouseover.begin(); it != m_mouseover.end(); ++it )
	{
		if( !g_Selection.IsSelected(it->entity) )
			it->entity->RenderAuras();
	}
}
void CMouseoverEntities::RenderBars()
{
	std::vector<SMouseoverFader>::iterator it;
	for( it = m_mouseover.begin(); it < m_mouseover.end(); it++ )
	{
		if( !g_Selection.IsSelected(it->entity) )
			it->entity->RenderBars();
	}
}

void CMouseoverEntities::RenderHealthBars()
{
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_BLEND );

	std::vector<SMouseoverFader>::iterator it;
	for( it = m_mouseover.begin(); it < m_mouseover.end(); it++ )
		it->entity->RenderHealthBar();

	glDisable( GL_BLEND );
}

void CMouseoverEntities::RenderStaminaBars()
{
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_BLEND );

	std::vector<SMouseoverFader>::iterator it;
	for( it = m_mouseover.begin(); it < m_mouseover.end(); it++ )
		it->entity->RenderStaminaBar();

	glDisable( GL_BLEND );
}
void CMouseoverEntities::RenderRanks()
{
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_BLEND );

	std::vector<SMouseoverFader>::iterator it;
	for( it = m_mouseover.begin(); it < m_mouseover.end(); it++ )
		it->entity->RenderRank();

	glDisable( GL_BLEND );
}
void CMouseoverEntities::RenderBarBorders()
{
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_BLEND );

	std::vector<SMouseoverFader>::iterator it;
	for( it = m_mouseover.begin(); it < m_mouseover.end(); it++ )
		it->entity->RenderBarBorders();

	glDisable( GL_BLEND );
}
void CMouseoverEntities::RenderRallyPoints()
{
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_BLEND );

	std::vector<SMouseoverFader>::iterator it;
	for( it = m_mouseover.begin(); it < m_mouseover.end(); it++ )
		it->entity->RenderRallyPoint();

	glDisable( GL_BLEND );
}
// Helper function for CSelectedEntities::LoadUnitUiTextures
static LibError LoadUnitUIThunk( const VfsPath& pathname, const FileInfo& UNUSED(fileInfo), uintptr_t cbData )
{
	std::map<CStr, Handle>* textures = (std::map<CStr, Handle>*)cbData;
	CStr name(pathname.string());

	if ( !tex_is_known_extension(name.c_str()) )
		return INFO::CB_CONTINUE;
	Handle tmp = ogl_tex_load(name.c_str());
	if (tmp <= 0)
	{
		LOG(CLogger::Error, LOG_CATEGORY, "Rank Textures", "loadRankTextures failed on \"%s\"", name.c_str());
		return INFO::CB_CONTINUE;
	}
	name.Remove("art/textures/ui/session/icons/");	//Names are relative to this directory
	(*textures)[name] = tmp;
	ogl_tex_upload(tmp);
	return INFO::CB_CONTINUE;
}
int CSelectedEntities::LoadUnitUiTextures()
{
	THROW_ERR( fs_ForEachFile(g_VFS, "art/textures/ui/session/icons/", LoadUnitUIThunk, (uintptr_t)&m_unitUITextures, 0, DIR_RECURSIVE));
	return 0;
}
void CSelectedEntities::DestroyUnitUiTextures()
{
	for ( std::map<CStr, Handle>::iterator it=m_unitUITextures.begin(); it != m_unitUITextures.end(); it++ )
	{
		ogl_tex_free(it->second);
		it->second = 0;
	}
}
void CMouseoverEntities::RenderOverlays()
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
			labelpos.Y = pTerrain->GetExactGroundLevel( labelpos.X, labelpos.Z );
#endif
			pCamera->GetScreenCoordinates( labelpos, x, y );
			glColor4f( 1.0f, 1.0f, 1.0f, it->fade );
			glTranslatef( x, g_Renderer.GetHeight() - y, 0.0f );
			glScalef( 1.0f, -1.0f, 1.0f );
			glwprintf( L"%d", it->entity->m_grouped );
			glDisable( GL_TEXTURE_2D );
			glPopMatrix();
		}
	}
}

void CMouseoverEntities::StartBandbox( u16 x, u16 y )
{
	m_bandbox = true;
	m_x1 = x; m_y1 = y;
}

void CMouseoverEntities::StopBandbox()
{
	m_bandbox = false;
}

void FireWorldClickEvent(int button, int clicks)
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
		g_Mouseover.m_worldposition.x,
		g_Mouseover.m_worldposition.y);

	//Reset duplication flag- after this, it isn't the same order
	std::vector<HEntity>::iterator it=g_Selection.m_selected.begin();
	for ( ; it != g_Selection.m_selected.end(); it++ )
	{
		if ( (*it)->m_formation >= 0)
		(*it)->GetFormation()->SetDuplication(false);
	}
}

void MouseButtonUpHandler(const SDL_Event_* ev, int clicks)
{
	FireWorldClickEvent(ev->ev.button.button, clicks);

	switch( ev->ev.button.button )
	{
	case SDL_BUTTON_LEFT:
		if( g_BuildingPlacer.m_active )
		{
			g_BuildingPlacer.MouseReleased();
			break;
		}

		if (customSelectionMode)
			break;

		if( g_Mouseover.m_viewall )
			break;

		if( clicks == 2 )
		{
			// Double click
			g_Mouseover.ExpandAcrossScreen();
		}
		else if( clicks == 3 )
		{
			// Triple click
			g_Mouseover.ExpandAcrossWorld();
		}

		g_Mouseover.StopBandbox();
		if( hotkeys[HOTKEY_SELECTION_ADD] )
		{
			g_Mouseover.AddSelection();
		}
		else if( hotkeys[HOTKEY_SELECTION_REMOVE] )
		{
			g_Mouseover.RemoveSelection();
		}
		else
			g_Mouseover.SetSelection();
		break;
	case SDL_BUTTON_RIGHT:
		if( g_BuildingPlacer.m_active )
		{
			g_BuildingPlacer.Deactivate();
			break;
		}
	}
}

InReaction InteractInputHandler( const SDL_Event_* ev )
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
	const int SDL_BUTTON_INDEX_COUNT = 6;
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

	if (customSelectionMode && ev->ev.type != SDL_MOUSEBUTTONUP)
		return IN_PASS;

	switch( ev->ev.type )
	{
	case SDL_HOTKEYDOWN:
		switch( ev->ev.user.code )
		{
		case HOTKEY_HIGHLIGHTALL:
			g_Mouseover.m_viewall = true;
			break;
		case HOTKEY_SELECTION_SNAP:
			if( g_Selection.m_selected.size() )
				pView->SetCameraTarget( g_Selection.GetSelectionPosition() );
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
			if( ( ev->ev.user.code >= HOTKEY_SELECTION_GROUP_0 ) && ( ev->ev.user.code <= HOTKEY_SELECTION_GROUP_19 ) )
			{
				// The above test limits it to 20 groups, so don't worry about overflowing

				i8 id = (i8)( ev->ev.user.code - HOTKEY_SELECTION_GROUP_0 );

				if( hotkeys[HOTKEY_SELECTION_GROUP_ADD] )
				{
					g_Selection.AddGroup( id );
				}
				else if( hotkeys[HOTKEY_SELECTION_GROUP_SAVE] )
				{
					g_Selection.SaveGroup( id );
				}
				else if( hotkeys[HOTKEY_SELECTION_GROUP_SNAP] )
				{
					g_Selection.HighlightGroup( id );
				}
				else
				{
					if( ( g_Selection.m_group == id ) && g_Selection.GetGroupCount( id ) )
					{
						pView->SetCameraTarget( g_Selection.GetGroupPosition( id ) );
					}
					else
						g_Selection.LoadGroup( id );
				}
				return( IN_HANDLED );
			}

			return( IN_PASS );
		}
		return( IN_HANDLED );
	case SDL_HOTKEYUP:
		switch( ev->ev.user.code )
		{
		case HOTKEY_SELECTION_GROUP_SNAP:
			if( g_Selection.m_group_highlight != -1 )
				g_Selection.HighlightNone();
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
		int button = ev->ev.button.button;
		// Only process buttons within the range for which we have button state
		// arrays above.
		if (button >= 0 && button < SDL_BUTTON_INDEX_COUNT)
		{
			double time = timer_Time();
			// Reset clicks counter if too slow or if the cursor's
			// hovering over something else now.

			if( time - lastclicktime[button] >= SELECT_DBLCLICK_RATE )
				clicks[button] = 0;
			if( g_Mouseover.m_target != lastclickobject[button] )
				clicks[button] = 0;
			clicks[button]++;

			lastclicktime[button] = time;
			lastclickobject[button] = g_Mouseover.m_target;

			if(ev->ev.button.button == SDL_BUTTON_LEFT )
				button_down = false;
			if(ev->ev.button.button == SDL_BUTTON_RIGHT )
				right_button_down = false;

			MouseButtonUpHandler(ev, clicks[button]);
		}
		break;
	}
	case SDL_MOUSEBUTTONDOWN:
		switch( ev->ev.button.button )
		{
		case SDL_BUTTON_LEFT:
			button_down = true;
			button_down_x = ev->ev.button.x;
			button_down_y = ev->ev.button.y;
			button_down_time = timer_Time();
			if( g_BuildingPlacer.m_active )
			{
				g_BuildingPlacer.MousePressed();
			}
			break;
		case SDL_BUTTON_RIGHT:
			right_button_down = true;
		}
		break;
	case SDL_MOUSEMOTION:
		if( !g_Mouseover.IsBandbox() && button_down && !g_BuildingPlacer.m_active && !right_button_down )
		{
			int deltax = ev->ev.motion.x - button_down_x;
			int deltay = ev->ev.motion.y - button_down_y;
			if( abs( deltax ) > 2 || abs( deltay ) > 2 )
				g_Mouseover.StartBandbox( button_down_x, button_down_y );
		}
		break;
	}
	return( IN_PASS );
}

bool IsOnScreen( CEntity* e, void* UNUSED(userdata) )
{
	CCamera *pCamera=g_Game->GetView()->GetCamera();

	CFrustum frustum = pCamera->GetFrustum();

	if( e->m_actor )
		return( frustum.IsBoxVisible( CVector3D(), e->m_actor->GetModel()->GetBounds() ) );
	else
		// If there's no actor, just treat the entity as a point
		return( frustum.IsBoxVisible( e->m_graphics_position, CBound() ) );
}

bool IsMouseoverType( CEntity* e, void* UNUSED(userdata) )
{
	std::vector<SMouseoverFader>::iterator it;
	for( it = g_Mouseover.m_mouseover.begin(); it < g_Mouseover.m_mouseover.end(); it++ )
	{
		if( it->isActive && ( (CEntityTemplate*)it->entity->m_base == (CEntityTemplate*)e->m_base )
				&& ( it->entity->GetPlayer() == e->GetPlayer() ) )
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

static const float angleBias = 3*PI/4;	// Atlas does the same

bool CBuildingPlacer::Activate(CStrW& templateName)
{	
	if(m_active)
	{
		return false;
	}

	m_templateName = templateName;
	m_active = true;
	m_clicked = false;
	m_dragged = false;
	m_angle = angleBias;
	m_timeSinceClick = 0;
	m_totalTime = 0;
	m_valid = false;

	m_template = g_EntityTemplateCollection.GetTemplate( m_templateName );

	if( !m_template )
	{
		Deactivate();
		return false;
	}

	CStr actorName ( m_template->m_actorName );	// convert CStrW->CStr8

	std::set<CStr8> selections;
	m_actor = g_Game->GetWorld()->GetUnitManager().CreateUnit( actorName, 0, selections );
	m_actor->SetPlayerID( g_Game->GetLocalPlayer()->GetPlayerID() );

	// m_bounds
	if( m_template->m_bound_type == CBoundingObject::BOUND_CIRCLE )
	{
 		m_bounds = new CBoundingCircle( 0, 0, m_template->m_bound_circle );
	}
	else if( m_template->m_bound_type == CBoundingObject::BOUND_OABB )
	{
		m_bounds = new CBoundingBox( 0, 0, CVector2D(1, 0), m_template->m_bound_box );
	}



	return true;
}

void CBuildingPlacer::MousePressed()
{
	CCamera &camera=*g_Game->GetView()->GetCamera();
	if( m_template->m_socket == L"" )
		clickPos = camera.GetWorldCoordinates();
	m_clicked = true;
}

void CBuildingPlacer::MouseReleased()
{
	Deactivate();	// do it first in case we fail for any reason

	m_clicked = false;

	if( m_valid )
	{
		// issue a place object command across the network
		CPlaceObjectMessage *msg = new CPlaceObjectMessage();
		msg->m_IsQueued = hotkeys[HOTKEY_ORDER_QUEUE];
		msg->m_Entities = g_Selection.m_selected;
		msg->m_Template = m_templateName;
		msg->m_X = (u32) (clickPos.X * 1000);
		msg->m_Y = (u32) (clickPos.Y * 1000);
		msg->m_Z = (u32) (clickPos.Z * 1000);
		msg->m_Angle = (u32) (m_angle * 1000);
		g_Game->GetSimulation()->QueueLocalCommand(msg);
	}

	if( hotkeys[HOTKEY_ORDER_QUEUE] )
	{
		Activate( m_templateName );		// reactivate so we can place more buildings of the same type
	}
}

void CBuildingPlacer::Deactivate()
{
	m_active = false;
	g_Game->GetWorld()->GetUnitManager().RemoveUnit( m_actor );
	delete m_actor;
	m_actor = 0;
	delete m_bounds;
	m_bounds = 0;
}

void CBuildingPlacer::Update( float timeStep )
{
	if(!m_active)
		return;

	m_totalTime += timeStep;

	if( m_clicked && m_template->m_socket == L"" )
	{
		// Rotate object
		m_timeSinceClick += timeStep;
		CCamera &camera = *g_Game->GetView()->GetCamera();
		CVector3D mousePos = camera.GetWorldCoordinates();
		CVector3D dif = mousePos - clickPos;
		float x = dif.X, z = dif.Z;
		if(x*x + z*z < 3*3)
		{
			if(m_dragged || m_timeSinceClick > 0.2f)
				m_angle += timeStep * PI;
		}
		else
		{
			m_dragged = true;
			m_angle = atan2(x, z);
		}
	}

	CVector3D pos;
	if( m_clicked )
	{
		pos = clickPos;
	}
	else
	{
		CCamera &camera = *g_Game->GetView()->GetCamera();
		pos = camera.GetWorldCoordinates();
	}

	bool onSocket = false;
	if( m_template->m_socket != L"" )
	{
		// If we're on a socket of our type, remember that and snap ourselves to it
		m_bounds->SetPosition(pos.X, pos.Z);	// first, move bounds to mouse pos
		CEntity* ent = GetCollisionEntity( m_bounds, 0 );	// now, check what we intersect
		if( ent && ent->m_classes.IsMember( m_template->m_socket ) )	// if it's a socket, snap to it
		{
			onSocket = true;
			m_angle = atan2f( ent->m_ahead.x, ent->m_ahead.y );
			pos = ent->m_position;
			clickPos = ent->m_position;
		}
	}

	// Set position and angle to the location we decided on
	CMatrix3D m;
	m.SetYRotation(m_angle + PI);
	m.Translate(pos);
	m_actor->GetModel()->SetTransform( m );
	m_bounds->SetPosition(pos.X, pos.Z);

	if( m_bounds->m_type == CBoundingObject::BOUND_OABB )
	{
		CBoundingBox* box = (CBoundingBox*) m_bounds;
		box->SetOrientation( m_angle );
	}

	// Validate placement location.
	CheckValidLocation(pos, onSocket);

	// Flash our actor red if the position is invalid.

	CColor col;
	if( m_valid )
	{
		col = CColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
	else
	{
		float add = ( sin(4*PI*m_totalTime) + 1.0f ) * 0.08f;
		col = CColor( 1.4f+add, 0.4f+add, 0.4f+add, 1.0f );
	}
	m_actor->GetModel()->SetShadingColor( col );
}

/**
 * Check whether the placement location is valid (look at whether we're
 * on the map, who owns the territory, whether we are on a socket, and 
 * whether we are colliding with anything).
 *
 * @param pos position under the cursor
 * @param onSocket whether on a socket or not
 */
void CBuildingPlacer::CheckValidLocation( CVector3D pos, bool onSocket )
{
	CTerrain *pTerrain=g_Game->GetWorld()->GetTerrain();
	if( pTerrain->IsOnMap( pos.X, pos.Z ) )
	{
		// Check that we are being placed in a valid territory; currently, m_territoryRestriction
		// can be either "Allied" for placing in allied territories, or nothing. 
		// Special case: If the territory has no centre unit, that means the map contains no
		//               Settlements; for testing purposes, let us build anywhere.
		CTerritory* territory = g_Game->GetWorld()->GetTerritoryManager()->GetTerritory( pos.X, pos.Z );
		if( m_template->m_territoryRestriction == L"Allied" && (bool)territory->centre 
			&& territory->owner != g_Game->GetLocalPlayer() )
		{
			m_valid = false;
		}
		else
		{
			// It's valid to place the object here if the position is unobstructed by
			// anything except possibly our socket (which we find out by passing an
			// ignoreClass to GetCollisionObject); also, if we are a socketed object,
			// we check that we are actually on a socket, using onSocket (set above).
			// UPDATED: Check for territorial building limit

			m_valid = ( m_template->m_socket == L"" || onSocket )
						&& ( GetCollisionObject( m_bounds, 0, &m_template->m_socket ) == 0 )
						&& IsWithinLimit(pos); // Is this building within its appropriate territorial limit?
		}
	}
	else
	{
		m_valid = false;
	}
}

/**
 * Checks whether there is space (territorial limit) in the current territory
 *
 * @param pos position under the cursor
 * 
 * @returns true if within limit, false otherwise
 */
bool CBuildingPlacer::IsWithinLimit( CVector3D pos )
{
	// Get the territorial building limit based on its category
	CStrW category = m_template->m_buildingLimitCategory;
	jsval param = ToJSVal(category);
	int limit = ToPrimitive<int>(g_ScriptingHost.CallFunction("getBuildingLimit", &param, 1));

	if( limit == 0 )
	{
		return true;
	}
	else
	{
		CTerritory* territory = g_Game->GetWorld()->GetTerritoryManager()->GetTerritory( pos.X, pos.Z );

		std::vector<CEntity*> extantEntities;
		std::vector<CEntity*>::iterator entIter;
		int buildingCount = 0; // Number of buildings in the current territory

		g_EntityManager.GetExtant(extantEntities);

		// NOTE: This loop runs continuously because the function is called in Update()
		for( entIter = extantEntities.begin(); entIter != extantEntities.end(); entIter++ )
		{
			if((*entIter)->m_buildingLimitCategory == m_template->m_buildingLimitCategory
				&& g_Game->GetWorld()->GetTerritoryManager()->GetTerritory( (*entIter)->m_position.X, (*entIter)->m_position.Z ) == territory)
			{
				buildingCount++;
			}
		}

		if(buildingCount < limit)
			return true;
		else
			return false;
	}


}
