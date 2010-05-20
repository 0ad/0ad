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

#include "graphics/GameView.h"
#include "graphics/Model.h"
#include "graphics/Sprite.h"
#include "graphics/Terrain.h"
#include "graphics/Unit.h"
#include "graphics/UnitManager.h"
#include "lib/res/graphics/ogl_tex.h"
#include "maths/MathUtil.h"
#include "maths/scripting/JSInterface_Vector3D.h"
#include "ps/Game.h"
#include "ps/Profile.h"
#include "ps/Player.h"
#include "renderer/Renderer.h"
#include "renderer/WaterManager.h"
#include "scripting/ScriptableComplex.inl"

#include "Aura.h"
#include "Collision.h"
#include "Entity.h"
#include "EntityFormation.h"
#include "EntityTemplate.h"
#include "EntityTemplateCollection.h"
#include "EventHandlers.h"
#include "Formation.h"
#include "FormationManager.h"
#include "ProductionQueue.h"
#include "TechnologyCollection.h"
#include "TerritoryManager.h"

#include <algorithm>

extern int g_xres, g_yres;

void CEntity::Render()
{
	if( !m_visible ) return;

	if( !m_orderQueue.empty() )
	{
		std::deque<CEntityOrder>::iterator it;
		CBoundingObject* destinationCollisionObject;
		float x0, y0, x, y;

		x = m_orderQueue.front().m_target_location.x;
		y = m_orderQueue.front().m_target_location.y;

		for( it = m_orderQueue.begin(); it < m_orderQueue.end(); it++ )
		{
			if( it->m_type == CEntityOrder::ORDER_PATROL )
				break;
			x = it->m_target_location.x;
			y = it->m_target_location.y;
		}
		destinationCollisionObject = GetContainingObject( CVector2D( x, y ) );

		glShadeModel( GL_FLAT );
		glBegin( GL_LINE_STRIP );

		glVertex3f( m_position.X, m_position.Y + 0.25f, m_position.Z );

		x = m_position.X;
		y = m_position.Z;

		for( it = m_orderQueue.begin(); it < m_orderQueue.end(); it++ )
		{
			x0 = x;
			y0 = y;
			x = it->m_target_location.x;
			y = it->m_target_location.y;
			rayIntersectionResults r;
			CVector2D fwd( x - x0, y - y0 );
			float l = fwd.Length();
			fwd = fwd.Normalize();
			CVector2D rgt = fwd.beta();
			if( GetRayIntersection( CVector2D( x0, y0 ), fwd, rgt, l, m_bounds->m_radius, destinationCollisionObject, &r ) )
			{
				glEnd();
				glBegin( GL_LINES );
				glColor3f( 1.0f, 0.0f, 0.0f );
				glVertex3f( x0 + fwd.x * r.distance, GetAnchorLevel( x0 + fwd.x * r.distance, y0 + fwd.y * r.distance ) + 0.25f, y0 + fwd.y * r.distance );
				glVertex3f( r.position.x, GetAnchorLevel( r.position.x, r.position.y ) + 0.25f, r.position.y );
				glEnd();
				glBegin( GL_LINE_STRIP );
				glVertex3f( x0, GetAnchorLevel( x0, y0 ), y0 );
			}
			switch( it->m_type )
			{
				case CEntityOrder::ORDER_GOTO:
				glColor3f( 1.0f, 0.0f, 0.0f );
				break;
				case CEntityOrder::ORDER_GOTO_COLLISION:
				glColor3f( 1.0f, 0.5f, 0.5f );
				break;
				case CEntityOrder::ORDER_GOTO_NOPATHING:
				case CEntityOrder::ORDER_GOTO_SMOOTHED:
				glColor3f( 0.5f, 0.5f, 0.5f );
				break;
				case CEntityOrder::ORDER_PATROL:
				glColor3f( 0.0f, 1.0f, 0.0f );
				break;
				default:
				continue;
			}

			glVertex3f( x, GetAnchorLevel( x, y ) + 0.25f, y );
		}

		glEnd();
		glShadeModel( GL_SMOOTH );
	}

	glColor3f( 1.0f, 1.0f, 1.0f );
	if( GetCollisionObject( this ) )
		glColor3f( 0.5f, 0.5f, 1.0f );
	m_bounds->Render( GetAnchorLevel( m_position.X, m_position.Z ) + 0.25f ); //m_position.Y + 0.25f );
}

void CEntity::RenderSelectionOutline( float alpha )
{
	if( !m_bounds || !m_visible )
		return;

	if( GetCollisionObject( m_bounds, m_player, &m_base->m_socket ) )
	{
		glColor4f( 1.0f, 0.5f, 0.5f, alpha );	// We're colliding with another unit; colour outline pink
	}
	else
	{
		const SPlayerColour& col = m_player->GetColour();
		glColor3f( col.r, col.g, col.b );		// Colour outline with player colour
	}

	glBegin( GL_LINE_LOOP );

	CVector3D pos = m_graphics_position;

	switch( m_bounds->m_type )
	{
		case CBoundingObject::BOUND_CIRCLE:
		{
			float radius = ((CBoundingCircle*)m_bounds)->m_radius;
			for( int i = 0; i < SELECTION_CIRCLE_POINTS; i++ )
			{
				float ang = i * 2 * (float)M_PI / SELECTION_CIRCLE_POINTS;
				float x = pos.X + radius * sin( ang );
				float y = pos.Z + radius * cos( ang );
#ifdef SELECTION_TERRAIN_CONFORMANCE

				glVertex3f( x, GetAnchorLevel( x, y ) + 0.25f, y );
#else

				glVertex3f( x, pos.Y + 0.25f, y );
#endif

			}
			break;
		}
		case CBoundingObject::BOUND_OABB:
		{
			CVector2D p, q;
			CVector2D u, v;
			q.x = pos.X;
			q.y = pos.Z;
			float d = ((CBoundingBox*)m_bounds)->m_d;
			float w = ((CBoundingBox*)m_bounds)->m_w;

			u.x = sin( m_graphics_orientation.Y );
			u.y = cos( m_graphics_orientation.Y );
			v.x = u.y;
			v.y = -u.x;

#ifdef SELECTION_TERRAIN_CONFORMANCE

			for( int i = SELECTION_BOX_POINTS; i > -SELECTION_BOX_POINTS; i-- )
			{
				p = q + u * d + v * ( w * (float)i / (float)SELECTION_BOX_POINTS );
				glVertex3f( p.x, GetAnchorLevel( p.x, p.y ) + 0.25f, p.y );
			}

			for( int i = SELECTION_BOX_POINTS; i > -SELECTION_BOX_POINTS; i-- )
			{
				p = q + u * ( d * (float)i / (float)SELECTION_BOX_POINTS ) - v * w;
				glVertex3f( p.x, GetAnchorLevel( p.x, p.y ) + 0.25f, p.y );
			}

			for( int i = -SELECTION_BOX_POINTS; i < SELECTION_BOX_POINTS; i++ )
			{
				p = q - u * d + v * ( w * (float)i / (float)SELECTION_BOX_POINTS );
				glVertex3f( p.x, GetAnchorLevel( p.x, p.y ) + 0.25f, p.y );
			}

			for( int i = -SELECTION_BOX_POINTS; i < SELECTION_BOX_POINTS; i++ )
			{
				p = q + u * ( d * (float)i / (float)SELECTION_BOX_POINTS ) + v * w;
				glVertex3f( p.x, GetAnchorLevel( p.x, p.y ) + 0.25f, p.y );
			}
#else
			p = q + u * h + v * w;
			glVertex3f( p.x, GetAnchorLevel( p.x, p.y ) + 0.25f, p.y );

			p = q + u * h - v * w;
			glVertex3f( p.x, GetAnchorLevel( p.x, p.y ) + 0.25f, p.y );

			p = q - u * h + v * w;
			glVertex3f( p.x, GetAnchorLevel( p.x, p.y ) + 0.25f, p.y );

			p = q + u * h + v * w;
			glVertex3f( p.x, GetAnchorLevel( p.x, p.y ) + 0.25f, p.y );
#endif


			break;
		}
	}

	glEnd();
}

void CEntity::RenderAuras()
{
	if( !(m_bounds && m_visible && !m_auras.empty()) )
		return;
	
	const SPlayerColour& playerCol = m_player->GetColour();
	glPushMatrix();
	glTranslatef(m_graphics_position.X, m_graphics_position.Y, 
										m_graphics_position.Z);

	size_t i=0;

	for ( AuraTable::iterator it=m_auras.begin(); it!=m_auras.end(); ++it, ++i )
	{
		CVector4D color = it->second->m_color;
		glColor4f(color.m_X, color.m_Y, color.m_Z, color.m_W);

#ifdef SELECTION_TERRAIN_CONFORMANCE
		//This starts to break when the radius is bigger
		if ( it->second->m_radius < 15.0f )
		{
			glBegin(GL_TRIANGLE_FAN);
			glVertex3f(0.0f, GetAnchorLevel(m_graphics_position.X, 
					m_graphics_position.Z)-m_graphics_position.Y+.5f, 0.0f);
			for ( int j=0; j<AURA_CIRCLE_POINTS; ++j )
			{
				CVector2D ypos( m_unsnappedPoints[i][j].x+m_graphics_position.X,
							m_unsnappedPoints[i][j].y+m_graphics_position.Z );
				CVector3D pos( m_unsnappedPoints[i][j].x, GetAnchorLevel(ypos.x, ypos.y)-
					m_graphics_position.Y+.5f, m_unsnappedPoints[i][j].y );
				glVertex3f(pos.X, pos.Y, pos.Z);
			}
			//Loop around
			CVector3D pos( m_unsnappedPoints[i][0].x, 
					GetAnchorLevel(m_unsnappedPoints[i][0].x+m_graphics_position.X, 
					m_unsnappedPoints[i][0].y+m_graphics_position.Z)-
					m_graphics_position.Y+.5f, m_unsnappedPoints[i][0].y );
			glVertex3f(pos.X, pos.Y, pos.Z);
			glEnd();	// GL_TRIANGLE_FAN
		}

		//Draw edges
		glEnable(GL_LINE_SMOOTH);
		glLineWidth(1.0f);
		glBegin(GL_LINE_LOOP);
		glColor3f( playerCol.r, playerCol.g, playerCol.b );
		for ( int j=0; j<AURA_CIRCLE_POINTS; ++j )
		{
			CVector2D ypos( m_unsnappedPoints[i][j].x+m_graphics_position.X,
						m_unsnappedPoints[i][j].y+m_graphics_position.Z );
			CVector3D pos( m_unsnappedPoints[i][j].x, GetAnchorLevel(ypos.x, ypos.y)-
				m_graphics_position.Y+.5f, m_unsnappedPoints[i][j].y );
			glVertex3f(pos.X, pos.Y, pos.Z);
		}
		glEnd();
		glDisable(GL_LINE_SMOOTH);
#else
		if ( it->second->m_radius < 15.0f )
		{
			for ( int j=0; j<SELECTION_CIRLCE_POINTS; ++j )
				glVertex3f(m_unsnappedPoints[i][j].x, .25f, m_unsnappedPoints[i][j].y);
			glVertex3f(m_unsnappedPoints[i][0].x, .25f, m_unsnappedPoints[i][0].y);
		}
		glEnd();
		
		//Draw edges
		glBegin(GL_LINE_LOOP);
		glColor3f( col.r, col.g, col.b );
		for ( int j=0; j<SELECTION_CIRLCE_POINTS; ++j )
			glVertex3f(unsnappedPoints[i][j].x, .25f, m_unsnappedPoints[i][j].y);
		glEnd();
#endif 
	}
	glPopMatrix();
}

CVector2D CEntity::GetScreenCoords( float height )
{
	CCamera &camera = *g_Game->GetView()->GetCamera();

	float sx, sy;
	CVector3D above;
	above.X = m_position.X;
	above.Z = m_position.Z;
	above.Y = GetAnchorLevel(m_position.X, m_position.Z) + height;
	camera.GetScreenCoordinates(above, sx, sy);
	return CVector2D( sx, sy );
}

void CEntity::DrawRect( CVector3D& centre, CVector3D& up, CVector3D& right, float x1, float y1, float x2, float y2 )
{
	glBegin(GL_QUADS);
	const int X[] = {1,1,0,0};	// which X and Y to choose at each vertex
	const int Y[] = {0,1,1,0};
	for( int i=0; i<4; i++ ) 
	{
		CVector3D vec = centre + right * (X[i] ? x1 : x2) + up * (Y[i] ? y1 : y2);
		glTexCoord2f( X[i], Y[i] );
		glVertex3fv( &vec.X );
	}
	glEnd();
}

void CEntity::DrawBar( CVector3D& centre, CVector3D& up, CVector3D& right, 
		float x1, float y1, float x2, float y2,
		SColour col1, SColour col2, float currVal, float maxVal )
{
	// Figure out fraction that should be col1
	float fraction;
	if(maxVal == 0) fraction = 1.0f;
	else fraction = clamp( currVal / maxVal, 0.0f, 1.0f );

	/*// Draw the border at full size
	ogl_tex_bind( g_Selection.m_unitUITextures[m_base->m_barBorder] );
	DrawRect( centre, up, right, x1, y1, x2, y2 );
	ogl_tex_bind( 0 );

	// Make the bar contents slightly smaller than the border
	x1 += m_base->m_barBorderSize;
	y1 += m_base->m_barBorderSize;
	x2 -= m_base->m_barBorderSize;
	y2 -= m_base->m_barBorderSize;*/
	
	// Draw the bar contents
	float xMid = x2 * fraction + x1 * (1.0f - fraction);
	glColor3fv( &col1.r );
	DrawRect( centre, up, right, x1, y1, xMid, y2 );
	glColor3fv( &col2.r );
	DrawRect( centre, up, right, xMid, y1, x2, y2 );
}

void CEntity::RenderBars()
{
	if( !m_base->m_barsEnabled || !m_bounds || !m_visible)
		return;

	SnapToGround();
	CVector3D centre = m_graphics_position;
	centre.Y += m_base->m_barOffset;
	CVector3D up = g_Game->GetView()->GetCamera()->m_Orientation.GetUp();
	CVector3D right = -g_Game->GetView()->GetCamera()->m_Orientation.GetLeft();

	float w = m_base->m_barWidth;
	float h = m_base->m_barHeight;
	float borderSize = m_base->m_barBorderSize;

	// Draw the health and stamina bars; if the unit has no stamina, the health bar is
	// drawn centered, otherwise it's offset slightly up and the stamina bar is offset
	// slightly down so that they overlap over an area of size borderSize.

	bool hasStamina = (m_staminaMax > 0);

	float backgroundW = w+2*borderSize;
	float backgroundH = hasStamina ? 2*h+2*borderSize : h+2*borderSize;
/*
	ogl_tex_bind( g_Selection.m_unitUITextures[m_base->m_barBorder] );
	DrawRect( centre, up, right, -backgroundW/2, -backgroundH/2, backgroundW/2, backgroundH/2 );
*/
	ogl_tex_bind( 0 );

	float off = hasStamina ? h/2 : 0;
	DrawBar( centre, up, right, -w/2, off-h/2, w/2, off+h/2, 
			SColour(0,1,0), SColour(1,0,0), m_healthCurr, m_healthMax );

	if( hasStamina ) 
	{
		DrawBar( centre, up, right, -w/2, -h, w/2, 0, 
				SColour(0,0,1), SColour(0.4f,0.4f,0.1f), m_staminaCurr, m_staminaMax );
	}

	// Draw the rank icon

/*
	CSelectedEntities::MapFilenameToHandle::iterator it = g_Selection.m_unitUITextures.find( m_rankName );
	if( it != g_Selection.m_unitUITextures.end() )
	{
		float size = 2*h + borderSize;
		ogl_tex_bind( it->second );
		DrawRect( centre, up, right, w/2+borderSize, -size/2, w/2+borderSize+size, size/2 );
		ogl_tex_bind( 0 );
	}
*/
}

void CEntity::RenderBarBorders()
{ 
	if( !m_visible )
		return;

/*
	if ( m_base->m_staminaBarHeight >= 0 && 
		g_Selection.m_unitUITextures.find(m_base->m_healthBorderName) != g_Selection.m_unitUITextures.end() )
	{
		ogl_tex_bind( g_Selection.m_unitUITextures[m_base->m_healthBorderName] );
		CVector2D pos = GetScreenCoords( m_base->m_healthBarHeight );

		float left = pos.x - m_base->m_healthBorderWidth/2;
		float right = pos.x + m_base->m_healthBorderWidth/2;
		pos.y = g_yres - pos.y;
		float bottom = pos.y + m_base->m_healthBorderHeight/2;
		float top = pos.y - m_base->m_healthBorderHeight/2;

		glBegin(GL_QUADS);

		glTexCoord2f(0.0f, 0.0f); glVertex3f( left, bottom, 0 );
		glTexCoord2f(0.0f, 1.0f); glVertex3f( left, top, 0 );
		glTexCoord2f(1.0f, 1.0f); glVertex3f( right, top, 0 );
		glTexCoord2f(1.0f, 0.0f); glVertex3f( right, bottom, 0 );

		glEnd();
	}
	if ( m_base->m_staminaBarHeight >= 0 && 
		g_Selection.m_unitUITextures.find(m_base->m_staminaBorderName) != g_Selection.m_unitUITextures.end() )
	{
		ogl_tex_bind( g_Selection.m_unitUITextures[m_base->m_staminaBorderName] );

		CVector2D pos = GetScreenCoords( m_base->m_staminaBarHeight );
		float left = pos.x - m_base->m_staminaBorderWidth/2;
		float right = pos.x + m_base->m_staminaBorderWidth/2;
		pos.y = g_yres - pos.y;
		float bottom = pos.y + m_base->m_staminaBorderHeight/2;
		float top = pos.y - m_base->m_staminaBorderHeight/2;

		glBegin(GL_QUADS);

		glTexCoord2f(0.0f, 0.0f); glVertex3f( left, bottom, 0 );
		glTexCoord2f(0.0f, 1.0f); glVertex3f( left, top, 0 );
		glTexCoord2f(1.0f, 1.0f); glVertex3f( right, top, 0 );
		glTexCoord2f(1.0f, 0.0f); glVertex3f( right, bottom, 0 );

		glEnd();
	}
*/
}

void CEntity::RenderHealthBar()
{
	if( !m_bounds || !m_visible )
		return;
	if( m_base->m_healthBarHeight < 0 )
		return;  // negative bar height means don't display health bar

	float fraction;
	if(m_healthMax == 0) fraction = 1.0f;
	else fraction = clamp(m_healthCurr / m_healthMax, 0.0f, 1.0f);

	CVector2D pos = GetScreenCoords( m_base->m_healthBarHeight );
	float x1 = pos.x - m_base->m_healthBarSize/2;
	float x2 = pos.x + m_base->m_healthBarSize/2;
	float y = g_yres - pos.y;

	glLineWidth( m_base->m_healthBarWidth );
	glBegin(GL_LINES);

	// green part of bar
	glColor3f( 0, 1, 0 );
	glVertex3f( x1, y, 0 );
	glColor3f( 0, 1, 0 );
	glVertex3f( x1 + m_base->m_healthBarSize*fraction, y, 0 );

	// red part of bar
	glColor3f( 1, 0, 0 );
	glVertex3f( x1 + m_base->m_healthBarSize*fraction, y, 0 );
	glColor3f( 1, 0, 0 );
	glVertex3f( x2, y, 0 );

	glEnd();

	glLineWidth(1.0f);
}

void CEntity::RenderStaminaBar()
{
	if( !m_bounds || !m_visible )
		return;
	if( m_base->m_staminaBarHeight < 0 )
		return;  // negative bar height means don't display stamina bar

	float fraction;
	if(m_staminaMax == 0) fraction = 1.0f;
	else fraction = clamp(m_staminaCurr / m_staminaMax, 0.0f, 1.0f);

	CVector2D pos = GetScreenCoords( m_base->m_staminaBarHeight );
	float x1 = pos.x - m_base->m_staminaBarSize/2;
	float x2 = pos.x + m_base->m_staminaBarSize/2;
	float y = g_yres - pos.y;

	glLineWidth( m_base->m_staminaBarWidth );
	glBegin(GL_LINES);

	// blue part of bar
	glColor3f( 0.1f, 0.1f, 1 );
	glVertex3f( x1, y, 0 );
	glColor3f( 0.1f, 0.1f, 1 );
	glVertex3f( x1 + m_base->m_staminaBarSize*fraction, y, 0 );

	// purple part of bar
	glColor3f( 0.3f, 0, 0.3f );
	glVertex3f( x1 + m_base->m_staminaBarSize*fraction, y, 0 );
	glColor3f( 0.3f, 0, 0.3f );
	glVertex3f( x2, y, 0 );

	glEnd();
	glLineWidth(1.0f);
}

void CEntity::RenderRank()
{
	if( !m_bounds || !m_visible )
		return;
	if( m_base->m_rankHeight < 0 )
		return;  // negative height means don't display rank
	//Check for valid texture
/*
	if( g_Selection.m_unitUITextures.find( m_rankName ) == g_Selection.m_unitUITextures.end() )
		return;
*/

	CCamera *camera = g_Game->GetView()->GetCamera();

	float sx, sy;
	CVector3D above;
	above.X = m_position.X;
	above.Z = m_position.Z;
	above.Y = GetAnchorLevel(m_position.X, m_position.Z) + m_base->m_rankHeight;
	camera->GetScreenCoordinates(above, sx, sy);
	int size = m_base->m_rankWidth/2;

	float x1 = sx + m_base->m_healthBarSize/2;
	float x2 = sx + m_base->m_healthBarSize/2 + 2*size;
	float y1 = g_yres - (sy - size);	//top
	float y2 = g_yres - (sy + size);	//bottom

//	ogl_tex_bind(g_Selection.m_unitUITextures[m_rankName]);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
	glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, g_Renderer.m_Options.m_LodBias);

	glBegin(GL_QUADS);

	glTexCoord2f(1.0f, 0.0f); glVertex3f( x2, y2, 0 );
	glTexCoord2f(1.0f, 1.0f); glVertex3f( x2, y1, 0 );
	glTexCoord2f(0.0f, 1.0f); glVertex3f( x1, y1, 0 );
	glTexCoord2f(0.0f, 0.0f); glVertex3f( x1, y2, 0 );

	glEnd();
}

void CEntity::RenderRallyPoint()
{
	if( !m_visible )
		return;

/*
	if ( !entf_get(ENTF_HAS_RALLY_POINT) ||
		 g_Selection.m_unitUITextures.find(m_base->m_rallyName) == g_Selection.m_unitUITextures.end() )
	{
		return;
	}
*/
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);

	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);	
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
	glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, g_Renderer.m_Options.m_LodBias);

	CSprite sprite;
	CTexture tex;
//	tex.SetHandle( g_Selection.m_unitUITextures[m_base->m_rallyName] );
	sprite.SetTexture(&tex);
	// Place the sprite slightly above ground/water level.
	float terrainHeight = g_Game->GetWorld()->GetTerrain()->GetExactGroundLevel(m_rallyPoint);
	float height = std::max(terrainHeight, g_Renderer.GetWaterManager()->m_WaterHeight) + 0.5f;
	CVector3D rally(m_rallyPoint.x, height, m_rallyPoint.y);
	sprite.SetTranslation(rally);
	sprite.Render();
}


