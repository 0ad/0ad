#include "precompiled.h"
#include "Interact.h"
#include "Renderer.h"
#include "input.h"
#include "CConsole.h"
#include "HFTracer.h"
#include "Hotkey.h"
#include "timer.h"

extern CCamera g_Camera;
extern CConsole* g_Console;
extern int mouse_x, mouse_y;
extern bool keys[SDLK_LAST];

static const float SELECT_DBLCLICK_RATE = 0.5f;
static const int ORDER_DELAY = 5;

void CSelectedEntities::addSelection( CEntity* entity )
{
	m_group = 255;
	assert( !isSelected( entity ) );
	m_selected.push_back( entity );
	entity->m_selected = true;
}

void CSelectedEntities::removeSelection( CEntity* entity )
{
	m_group = 255;
	assert( isSelected( entity ) );
	entity->m_selected = false;
	std::vector<CEntity*>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
	{
		if( (*it) == entity ) 
		{
			m_selected.erase( it );
			break;
		}
	}
}

void CSelectedEntities::renderSelectionOutlines()
{
	std::vector<CEntity*>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		(*it)->renderSelectionOutline();

	if( m_group_highlight != 255 )
	{
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glEnable( GL_BLEND );

		std::vector<CEntity*>::iterator it;
		for( it = m_groups[m_group_highlight].begin(); it < m_groups[m_group_highlight].end(); it++ )
			(*it)->renderSelectionOutline( 0.5f );

		glDisable( GL_BLEND );
	}
}

void CSelectedEntities::renderOverlays()
{
	glPushMatrix();
	glEnable( GL_TEXTURE_2D );
	std::vector<CEntity*>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
	{
		if( (*it)->m_grouped != 255 )
		{
			assert( (*it)->m_bounds );
			
			glLoadIdentity();
			float x, y;
			CVector3D labelpos = (*it)->m_graphics_position - g_Camera.m_Orientation.GetLeft() * (*it)->m_bounds->m_radius;
			labelpos.X += (*it)->m_bounds->m_offset.x;
			labelpos.Z += (*it)->m_bounds->m_offset.y;
#ifdef SELECTION_TERRAIN_CONFORMANCE
			labelpos.Y = (*it)->getExactGroundLevel( labelpos.X, labelpos.Z );
#endif
			g_Camera.GetScreenCoordinates( labelpos, x, y );
			glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
			glTranslatef( x, g_Renderer.GetHeight() - y, 0.0f );
			glScalef( 1.0f, -1.0f, 1.0f );
			glwprintf( L"%d", (*it)->m_grouped );
			
		}
	}
	if( m_group_highlight != 255 )
	{
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glEnable( GL_BLEND );

		std::vector<CEntity*>::iterator it;
		for( it = m_groups[m_group_highlight].begin(); it < m_groups[m_group_highlight].end(); it++ )
		{
			assert( (*it)->m_bounds );
			
			glLoadIdentity();
			float x, y;
			CVector3D labelpos = (*it)->m_graphics_position - g_Camera.m_Orientation.GetLeft() * (*it)->m_bounds->m_radius;
			labelpos.X += (*it)->m_bounds->m_offset.x;
			labelpos.Z += (*it)->m_bounds->m_offset.y;
#ifdef SELECTION_TERRAIN_CONFORMANCE
			labelpos.Y = (*it)->getExactGroundLevel( labelpos.X, labelpos.Z );
#endif
			g_Camera.GetScreenCoordinates( labelpos, x, y );
			glColor4f( 1.0f, 1.0f, 1.0f, 0.5f );
			glTranslatef( x, g_Renderer.GetHeight() - y, 0.0f );
			glScalef( 1.0f, -1.0f, 1.0f );
			glwprintf( L"%d", (*it)->m_grouped );
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
	}

	glDisable( GL_TEXTURE_2D );
	glPopMatrix();
}

void CSelectedEntities::setSelection( CEntity* entity )
{
	m_group = 255;
	clearSelection();
	m_selected.push_back( entity );
}

void CSelectedEntities::clearSelection()
{
	m_group = 255;
	std::vector<CEntity*>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		(*it)->m_selected = false;
	m_selected.clear();
}

void CSelectedEntities::removeAll( CEntity* entity )
{
	// Remove a reference to an entity from everywhere
	// (for use when said entity is being destroyed)
	std::vector<CEntity*>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
	{
		if( (*it) == entity ) 
		{
			m_selected.erase( it );
			break;
		}
	}
	for( u8 group = 0; group < 10; group++ )
	{
		for( it = m_groups[group].begin(); it < m_groups[group].end(); it++ )
		{
			if( (*it) == entity ) 
			{
				m_groups[group].erase( it );
				break;
			}
		}
	}
}

CVector3D CSelectedEntities::getSelectionPosition()
{
	CVector3D avg;
	std::vector<CEntity*>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
	{
		avg += (*it)->m_graphics_position;
		avg.X += (*it)->m_bounds->m_offset.x;
		avg.Z += (*it)->m_bounds->m_offset.y;
	}
	return( avg * ( 1.0f / m_selected.size() ) );
}

void CSelectedEntities::saveGroup( u8 groupid )
{
	std::vector<CEntity*>::iterator it;
	// Clear all entities in the group...
	for( it = m_groups[groupid].begin(); it < m_groups[groupid].end(); it++ )
	{
		(*it)->m_grouped = 255;
		(*it)->m_grouped_mirror = 0;
	}
	m_groups[groupid].clear();
	// Remove selected entities from each group they're in, and flag them as
	// members of the new group
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
	{
		if( (*it)->m_grouped < 255 )
		{
			std::vector<CEntity*>& group = m_groups[(*it)->m_grouped];
			std::vector<CEntity*>::iterator it2;
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
		(*it)->m_grouped_mirror = ( groupid != 0 ) ? groupid : 10;
	}
	// Copy the group across
	m_groups[groupid] = m_selected;
	// Set the group selection memory
	m_group = groupid;
}

void CSelectedEntities::loadGroup( u8 groupid )
{
	clearSelection();
	m_selected = m_groups[groupid];
	std::vector<CEntity*>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		(*it)->m_selected = true;
	m_group = groupid;
}

void CSelectedEntities::addGroup( u8 groupid )
{
	std::vector<CEntity*>::iterator it;
	for( it = m_groups[groupid].begin(); it < m_groups[groupid].end(); it++ )
	{
		if( !isSelected( *it ) )
			addSelection( *it );
	}
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		(*it)->m_selected = true;
}

void CSelectedEntities::changeGroup( CEntity* entity, u8 groupid )
{
	// Remove from current group
	u8 current = entity->m_grouped;
	if( current != 255 )
	{
		std::vector<CEntity*>::iterator it;
		for( it = m_groups[current].begin(); it < m_groups[current].end(); it++ )
		{
			if( (*it) == entity ) 
			{
				m_groups[current].erase( it );
				break;
			}
		}
	}
	if( groupid != 255 )
		m_groups[groupid].push_back( entity );
	entity->m_grouped = groupid;
}

bool CSelectedEntities::isSelected( CEntity* entity )
{
	std::vector<CEntity*>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
	{
		if( (*it) == entity ) 
			return( true );
	}
	return( false );
}

void CSelectedEntities::highlightGroup( u8 groupid )
{
	if( m_group_highlight != 255 )
		return;
	if( !getGroupCount( groupid ) )
		return;
	m_group_highlight = groupid;
	pushCameraTarget( getGroupPosition( groupid ) );
}

void CSelectedEntities::highlightNone()
{
	if( m_group_highlight != 255 )
		popCameraTarget();
	m_group_highlight = 255;
}

int CSelectedEntities::getGroupCount( u8 groupid )
{
	return( (int)m_groups[groupid].size() );
}

CVector3D CSelectedEntities::getGroupPosition( u8 groupid )
{
	CVector3D avg;
	std::vector<CEntity*>::iterator it;
	for( it = m_groups[groupid].begin(); it < m_groups[groupid].end(); it++ )
	{
		avg += (*it)->m_graphics_position;
		avg.X += (*it)->m_bounds->m_offset.x;
		avg.Z += (*it)->m_bounds->m_offset.y;
	}
	return( avg * ( 1.0f / m_groups[groupid].size() ) );
}

void CSelectedEntities::update()
{
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
	if( ( m_group_highlight != 255 ) && getGroupCount( m_group_highlight ) )
		setCameraTarget( getGroupPosition( m_group_highlight ) );
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

	// Check to see if any member of the selection supports this order type.
	std::vector<CEntity*>::iterator it;
	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		if( (*it)->acceptsOrder( contextOrder, g_Mouseover.m_target ) )
			return( true );
	return( false );
}

void CSelectedEntities::contextOrder( bool pushQueue )
{
	std::vector<CEntity*>::iterator it;
	CEntityOrder context;
	(int&)context.m_type = m_contextOrder;
	CVector3D origin, dir;
	g_Camera.BuildCameraRay( origin, dir );

	switch( m_contextOrder )
	{
	case CEntityOrder::ORDER_GOTO:
	case CEntityOrder::ORDER_PATROL:
		{
		CHFTracer maptracer( g_Terrain.GetHeightMap(), g_Terrain.GetVerticesPerSide(), (float)CELL_SIZE, HEIGHT_SCALE );
		int x, y; CVector3D ipt;
		maptracer.RayIntersect( origin, dir, x, y, ipt );
		context.m_data[0].location.x = ipt.X;
		context.m_data[0].location.y = ipt.Z;
		}
		break;
	
	default:
		break;
	}

	for( it = m_selected.begin(); it < m_selected.end(); it++ )
		if( (*it)->acceptsOrder( m_contextOrder, g_Mouseover.m_target ) )
			g_Scheduler.pushFrame( ORDER_DELAY, (*it)->me, new CMessageOrder( context, pushQueue ) );
	
}

void CMouseoverEntities::update( float timestep )
{
	CVector3D origin, dir;
	g_Camera.BuildCameraRay( origin, dir );

	CUnit* hit = g_UnitMan.PickUnit( origin, dir );

	m_target = NULL;
	if( hit )
		m_target = hit->GetEntity();
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
			m_mouseover.push_back( SMouseoverFader( &(**it), m_fademaximum, false ) );

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
			CVector3D worldspace = (*it)->m_graphics_position;
			worldspace.X += (*it)->m_bounds->m_offset.x;
			worldspace.Z += (*it)->m_bounds->m_offset.y;

			float x, y;

			g_Camera.GetScreenCoordinates( worldspace, x, y );

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
					m_mouseover.push_back( SMouseoverFader( &(**it), ( m_fadeinrate + m_fadeoutrate ) * timestep ) );
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
		if( !found && m_target )
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
	{
		m_mouseover.push_back( SMouseoverFader( &(**it) ) );
	}
	delete( activeset );
}

void CMouseoverEntities::expandAcrossWorld()
{
	std::vector<HEntity>* activeset = g_EntityManager.matches( isMouseoverType );
	m_mouseover.clear();
	std::vector<HEntity>::iterator it;
	for( it = activeset->begin(); it < activeset->end(); it++ )
	{
		m_mouseover.push_back( SMouseoverFader( &(**it) ) );
	}
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
		if( it->entity->m_grouped != 255 )
		{
			assert( it->entity->m_bounds );
			glPushMatrix();
			glEnable( GL_TEXTURE_2D );
			glLoadIdentity();
			float x, y;
			CVector3D labelpos = it->entity->m_graphics_position - g_Camera.m_Orientation.GetLeft() * it->entity->m_bounds->m_radius;
			labelpos.X += it->entity->m_bounds->m_offset.x;
			labelpos.Z += it->entity->m_bounds->m_offset.y;
#ifdef SELECTION_TERRAIN_CONFORMANCE
			labelpos.Y = it->entity->getExactGroundLevel( labelpos.X, labelpos.Z );
#endif
			g_Camera.GetScreenCoordinates( labelpos, x, y );
			glColor4f( 1.0f, 1.0f, 1.0f, it->fade );
			glTranslatef( x, g_Renderer.GetHeight() - y, 0.0f );
			glScalef( 1.0f, -1.0f, 1.0f );
			glwprintf( L"%d", it->entity->m_grouped );
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
	
	static float lastclicktime = 0.0f;
	static CEntity* lastclickobject = NULL;
	static u8 clicks = 0;

	static u16 button_down_x, button_down_y;
	static bool button_down = false;

	switch( ev->type )
	{
	case SDL_KEYDOWN:
		if( ( ev->key.keysym.sym >= SDLK_0 ) && ( ev->key.keysym.sym <= SDLK_9 ) )
		{
			u8 groupid = ev->key.keysym.sym - SDLK_0;
			if( hotkeys[HOTKEY_SELECTION_GROUP_ADD] )
			{
				g_Selection.addGroup( groupid );
			}
			else if( hotkeys[HOTKEY_SELECTION_GROUP_SAVE] )
			{
				g_Selection.saveGroup( groupid );
			}
			else if( hotkeys[HOTKEY_SELECTION_GROUP_SNAP] )
			{
				g_Selection.highlightGroup( groupid );
			}
			else
			{
				if( ( g_Selection.m_group == groupid ) && g_Selection.getGroupCount( groupid ) )
				{
					setCameraTarget( g_Selection.getGroupPosition( groupid ) );
				}
				else
					g_Selection.loadGroup( groupid );
			}
		}
		break;
	case SDL_HOTKEYDOWN:
		switch( ev->user.code )
		{
		case HOTKEY_HIGHLIGHTALL:
			g_Mouseover.m_viewall = true;
			break;
		case HOTKEY_SELECTION_SNAP:
			if( g_Selection.m_selected.size() )
				setCameraTarget( g_Selection.getSelectionPosition() );
			break;
		case HOTKEY_CONTEXTORDER_NEXT:
			g_Selection.nextContext();
			break;
		case HOTKEY_CONTEXTORDER_PREVIOUS:
			g_Selection.previousContext();
			break;
		default:
			return( EV_PASS );
		}
		return( EV_HANDLED );
	case SDL_HOTKEYUP:
		switch( ev->user.code )
		{
		case HOTKEY_SELECTION_GROUP_SNAP:
			if( g_Selection.m_group_highlight != 255 )
				g_Selection.highlightNone();
			break;
		case HOTKEY_HIGHLIGHTALL:
			g_Mouseover.m_viewall = false;
			break;
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
			g_Selection.contextOrder( keys[SDLK_LSHIFT] || keys[SDLK_RSHIFT] );
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
	CFrustum frustum = g_Camera.GetFustum();
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

void pushCameraTarget( const CVector3D& target )
{
	cameraTargets.push_back( g_Camera.m_Orientation.GetTranslation() );
	setCameraTarget( target );
}

void setCameraTarget( const CVector3D& target )
{
	CVector3D cameraForward = g_Camera.m_Orientation.GetIn();
	cameraDelta = ( target - ( cameraForward * 160.0f ) ) - g_Camera.m_Orientation.GetTranslation();
}

void popCameraTarget()
{
	cameraDelta = cameraTargets.back() - g_Camera.m_Orientation.GetTranslation();
	cameraTargets.pop_back();
}