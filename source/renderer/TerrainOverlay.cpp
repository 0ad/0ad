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

#include "TerrainOverlay.h"

#include "ps/Overlay.h"
#include "ps/Game.h"
#include "ps/World.h"
#include "graphics/Terrain.h"
#include "maths/MathUtil.h"
#include "lib/ogl.h"

#include <algorithm>

// Handy things for STL:

/// Functor for sorting pairs, using the &lt;-ordering of their second values.
struct compare2nd
{
	template<typename S, typename T> bool operator()(const std::pair<S, T>& a, const std::pair<S, T>& b) const
	{
		return a.second < b.second;
	}
};

/// Functor for comparing the firsts of pairs to a specified value.
template<typename S> struct equal1st
{
	const S& val;
	equal1st(const S& val) : val(val) {}
	template <typename T> bool operator()(const std::pair<S, T>& a) const
	{
		return a.first == val;
	}
private:
	const equal1st& operator=(const equal1st& rhs);
};

/// Functor for calling ->Render on pairs' firsts.
struct render1st
{
	template<typename S, typename T> void operator()(const std::pair<S, T>& a) const
	{
		a.first->Render();
	}
};

//////////////////////////////////////////////////////////////////////////

// Global overlay list management:

static std::vector<std::pair<TerrainOverlay*, int> > g_TerrainOverlayList;

TerrainOverlay::TerrainOverlay(int priority)
{
	// Add to global list of overlays
	g_TerrainOverlayList.push_back(std::make_pair(this, priority));
	// Sort by overlays by priority. Do stable sort so that adding/removing
	// overlays doesn't randomly disturb all the existing ones (which would
	// be noticeable if they have the same priority and overlap).
	std::stable_sort(g_TerrainOverlayList.begin(), g_TerrainOverlayList.end(),
		compare2nd());
}

TerrainOverlay::~TerrainOverlay()
{
	std::vector<std::pair<TerrainOverlay*, int> >::iterator newEnd =
		std::remove_if(g_TerrainOverlayList.begin(), g_TerrainOverlayList.end(),
			equal1st<TerrainOverlay*>(this));
	g_TerrainOverlayList.erase(newEnd, g_TerrainOverlayList.end());
}



#if 0
//initial test to draw out entity boundaries
//it shows how to retrieve object boundary postions for triangulation 
//NOTE: it's a test to see how to retrieve bounadry locations for static objects on the terrain.
//disabled
void TerrainOverlay::RenderEntityEdges()
{
	//Kai: added for line drawing
	//use a function to encapsulate all the entity boundaries
	std::vector<CEntity*> results;

			
	
	g_EntityManager.GetExtant(results);

	glColor3f( 1, 1, 1 );		// Colour outline with player colour

	
	
	for(size_t i =0 ; i < results.size(); i++)
	{
		glBegin(GL_LINE_LOOP);

		CEntity* tempHandle = results[i];
		debug_printf(L"Entity position: %f %f %f\n", tempHandle->m_position.X,tempHandle->m_position.Y,tempHandle->m_position.Z);

		CVector2D p, q;
			CVector2D u, v;
			q.x = tempHandle->m_position.X;
			q.y = tempHandle->m_position.Z;
			float d = ((CBoundingBox*)tempHandle->m_bounds)->m_d;
			float w = ((CBoundingBox*)tempHandle->m_bounds)->m_w;

			u.x = sin( tempHandle->m_graphics_orientation.Y );
			u.y = cos( tempHandle->m_graphics_orientation.Y );
			v.x = u.y;
			v.y = -u.x;

		CBoundingObject* m_bounds = tempHandle->m_bounds;

		switch( m_bounds->m_type )
		{
			case CBoundingObject::BOUND_CIRCLE:
			{
				break;
			}
			case CBoundingObject::BOUND_OABB:
			{
				//glVertex3f( tempHandle->m_position.X, tempHandle->GetAnchorLevel( tempHandle->m_position.X, tempHandle->m_position.Z ) + 10.0f, tempHandle->m_position.Z );    // lower left vertex
					
				//glVertex3f( 5, tempHandle->GetAnchorLevel( 5, 5 ) + 0.25f, 5 );     // upper vertex

				p = q + u * d + v * w;
				glVertex3f( p.x, tempHandle->GetAnchorLevel( p.x, p.y ) + + 10.0f, p.y );

				p = q - u * d + v * w ;
				glVertex3f( p.x, tempHandle->GetAnchorLevel( p.x, p.y ) + + 10.0f, p.y );

				p = q - u * d - v * w;
				glVertex3f( p.x, tempHandle->GetAnchorLevel( p.x, p.y ) + + 10.0f, p.y );

				p = q + u * d - v * w;
				glVertex3f( p.x, tempHandle->GetAnchorLevel( p.x, p.y ) + + 10.0f, p.y );

				
				break;
			}

				

		}//end switch

	glEnd();
	}//end for loop

	CTerrain* m_Terrain = g_Game->GetWorld()->GetTerrain();
	CEntity* tempHandle = results[0];
	glBegin(GL_LINE_LOOP);

	int width = m_Terrain->GetVerticesPerSide()*4;
	glVertex3f( 0, tempHandle->GetAnchorLevel( 0, 0 ) + 0.25f, 0 );
	glVertex3f( width, tempHandle->GetAnchorLevel( width, 0 ) + 0.25f, 0 );
	glVertex3f( width, tempHandle->GetAnchorLevel(width,width ) + 0.25f,width );
	glVertex3f( 0, tempHandle->GetAnchorLevel( 0, width ) + 0.25f, width );
	glEnd();

	//----------------------
}
#endif


void TerrainOverlay::RenderOverlays()
{
	if (g_TerrainOverlayList.size() == 0)
		return;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	// To ensure that outlines are drawn on top of the terrain correctly (and
	// don't Z-fight and flicker nastily), draw them as QUADS with the LINE
	// PolygonMode, and use PolygonOffset to pull them towards the camera.
	// (See e.g. http://www.opengl.org/resources/faq/technical/polygonoffset.htm)
	glPolygonOffset(-1.f, -1.f);
	glEnable(GL_POLYGON_OFFSET_LINE);

	glDisable(GL_TEXTURE_2D);

	std::for_each(g_TerrainOverlayList.begin(), g_TerrainOverlayList.end(),
		render1st());

	// Clean up state changes
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_POLYGON_OFFSET_LINE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);


	//Kai: invoking the auxiliary function to draw out entity edges
	//RenderEntityEdges();
	
}

//////////////////////////////////////////////////////////////////////////

void TerrainOverlay::StartRender()
{
}

void TerrainOverlay::EndRender()
{
}

void TerrainOverlay::GetTileExtents(
	ssize_t& min_i_inclusive, ssize_t& min_j_inclusive,
	ssize_t& max_i_inclusive, ssize_t& max_j_inclusive)
{
	// Default to whole map
	min_i_inclusive = min_j_inclusive = 0;
	max_i_inclusive = max_j_inclusive = m_Terrain->GetTilesPerSide()-1;
}

void TerrainOverlay::Render()
{
	m_Terrain = g_Game->GetWorld()->GetTerrain();

	StartRender();

	ssize_t min_i, min_j, max_i, max_j;
	GetTileExtents(min_i, min_j, max_i, max_j);
	// Clamp the min to 0, but the max to -1 - so tile -1 can never be rendered,
	// but if unclamped_max<0 then no tiles at all will be rendered. And the same
	// for the upper limit.
	min_i = clamp(min_i, ssize_t(0), m_Terrain->GetTilesPerSide());
	min_j = clamp(min_j, ssize_t(0), m_Terrain->GetTilesPerSide());
	max_i = clamp(max_i, ssize_t(-1), m_Terrain->GetTilesPerSide()-1);
	max_j = clamp(max_j, ssize_t(-1), m_Terrain->GetTilesPerSide()-1);

	for (m_j = min_j; m_j <= max_j; ++m_j)
		for (m_i = min_i; m_i <= max_i; ++m_i)
			ProcessTile(m_i, m_j);

	EndRender();
}

void TerrainOverlay::RenderTile(const CColor& colour, bool draw_hidden)
{
	RenderTile(colour, draw_hidden, m_i, m_j);
}

void TerrainOverlay::RenderTile(const CColor& colour, bool draw_hidden, ssize_t i, ssize_t j)
{
	// TODO: if this is unpleasantly slow, make it much more efficient
	// (e.g. buffering data and making a single draw call? or at least
	// far fewer calls than it makes now)

	if (draw_hidden)
	{
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
	}
	else
	{
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
	}
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	CVector3D pos;
	glBegin(GL_QUADS);
		glColor4fv(colour.FloatArray());
		m_Terrain->CalcPosition(i,   j,   pos); glVertex3fv(pos.GetFloatArray());
		m_Terrain->CalcPosition(i+1, j,   pos); glVertex3fv(pos.GetFloatArray());
		m_Terrain->CalcPosition(i+1, j+1, pos); glVertex3fv(pos.GetFloatArray());
		m_Terrain->CalcPosition(i,   j+1, pos); glVertex3fv(pos.GetFloatArray());
	glEnd();
}

void TerrainOverlay::RenderTileOutline(const CColor& colour, int line_width, bool draw_hidden)
{
	RenderTileOutline(colour, line_width, draw_hidden, m_i, m_j);
}

void TerrainOverlay::RenderTileOutline(const CColor& colour, int line_width, bool draw_hidden, ssize_t i, ssize_t j)
{
	if (draw_hidden)
	{
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
	}
	else
	{
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glLineWidth((float)line_width);

	CVector3D pos;
	glBegin(GL_QUADS);
		glColor4fv(colour.FloatArray());
		m_Terrain->CalcPosition(i,   j,   pos); glVertex3fv(pos.GetFloatArray());
		m_Terrain->CalcPosition(i+1, j,   pos); glVertex3fv(pos.GetFloatArray());
		m_Terrain->CalcPosition(i+1, j+1, pos); glVertex3fv(pos.GetFloatArray());
		m_Terrain->CalcPosition(i,   j+1, pos); glVertex3fv(pos.GetFloatArray());
	glEnd();
}
