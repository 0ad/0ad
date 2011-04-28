/* Copyright (C) 2011 Wildfire Games.
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

#include <math.h>

#include "MiniMap.h"

#include "graphics/GameView.h"
#include "graphics/LOSTexture.h"
#include "graphics/MiniPatch.h"
#include "graphics/Terrain.h"
#include "graphics/TerrainTextureEntry.h"
#include "graphics/TerrainTextureManager.h"
#include "lib/ogl.h"
#include "lib/external_libraries/sdl.h"
#include "lib/bits.h"
#include "lib/timer.h"
#include "ps/Game.h"
#include "ps/Profile.h"
#include "ps/World.h"
#include "renderer/Renderer.h"
#include "renderer/WaterManager.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpMinimap.h"

bool g_GameRestarted = false;

static unsigned int ScaleColor(unsigned int color, float x)
{
	unsigned int r = unsigned(float(color & 0xff) * x);
	unsigned int g = unsigned(float((color>>8) & 0xff) * x);
	unsigned int b = unsigned(float((color>>16) & 0xff) * x);
	return (0xff000000 | r | g<<8 | b<<16);
}

CMiniMap::CMiniMap() :
	m_TerrainTexture(0), m_TerrainData(0), m_MapSize(0), m_Terrain(0), m_TerrainDirty(true)
{
	AddSetting(GUIST_CColor,	"fov_wedge_color");
	AddSetting(GUIST_bool,		"circular");
	AddSetting(GUIST_CStrW,		"tooltip");
	AddSetting(GUIST_CStr,		"tooltip_style");
	m_Clicking = false;
	m_Hovering = false;
}

CMiniMap::~CMiniMap()
{
	Destroy();
}

void CMiniMap::HandleMessage(SGUIMessage &Message)
{
	switch(Message.type)
	{
	case GUIM_MOUSE_PRESS_LEFT:
		{
			if (m_Hovering)
			{
				SetCameraPos();
				m_Clicking = true;
			}
			break;
		}
	case GUIM_MOUSE_RELEASE_LEFT:
		{
			if(m_Hovering && m_Clicking)
			{
				SetCameraPos();
			}
			m_Clicking = false;
			break;
		}
	case GUIM_MOUSE_DBLCLICK_LEFT:
		{
			if(m_Hovering && m_Clicking)
			{
				SetCameraPos();
			}
			m_Clicking = false;
			break;
		}
	case GUIM_MOUSE_ENTER:
		{
			m_Hovering = true;
			break;
		}
	case GUIM_MOUSE_LEAVE:
		{
			m_Clicking = false;
			m_Hovering = false;
			break;
		}
	case GUIM_MOUSE_RELEASE_RIGHT:
		{
			CMiniMap::FireWorldClickEvent(SDL_BUTTON_RIGHT, 1);
			break;
		}
	case GUIM_MOUSE_DBLCLICK_RIGHT:
		{
			CMiniMap::FireWorldClickEvent(SDL_BUTTON_RIGHT, 2);
			break;
		}
	case GUIM_MOUSE_MOTION:
		{
			if (m_Hovering && m_Clicking)
			{
				SetCameraPos();
			}
			break;
		}
	case GUIM_MOUSE_WHEEL_DOWN:
	case GUIM_MOUSE_WHEEL_UP:
		Message.Skip();
		break;

	default:
		break;
	}	// switch
}

void CMiniMap::GetMouseWorldCoordinates(float& x, float& z)
{
	// Determine X and Z according to proportion of mouse position and minimap

	CPos mousePos = GetMousePos();

	float px = (mousePos.x - m_CachedActualSize.left) / m_CachedActualSize.GetWidth();
	float py = (m_CachedActualSize.bottom - mousePos.y) / m_CachedActualSize.GetHeight();

	float angle = GetAngle();

	x = CELL_SIZE * m_MapSize * (cos(angle)*(px-0.5) - sin(angle)*(py-0.5) + 0.5);
	z = CELL_SIZE * m_MapSize * (cos(angle)*(py-0.5) + sin(angle)*(px-0.5) + 0.5);
}

void CMiniMap::SetCameraPos()
{
	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();

	CVector3D target;
	GetMouseWorldCoordinates(target.X, target.Z);
	target.Y = terrain->GetExactGroundLevel(target.X, target.Z);
	g_Game->GetView()->MoveCameraTarget(target, true);
}

float CMiniMap::GetAngle()
{
	bool circular;
	GUI<bool>::GetSetting(this, "circular", circular);

	// If this is a circular map, rotate it to match the camera angle
	if (circular)
	{
		CVector3D cameraIn = m_Camera->m_Orientation.GetIn();
		return -atan2(cameraIn.X, cameraIn.Z);
	}

	// Otherwise there's no rotation
	return 0.f;
}

void CMiniMap::FireWorldClickEvent(int button, int clicks)
{
	float x, z;
	GetMouseWorldCoordinates(x, z);

	CScriptValRooted coords;
	g_ScriptingHost.GetScriptInterface().Eval("({})", coords);
	g_ScriptingHost.GetScriptInterface().SetProperty(coords.get(), "x", x, false);
	g_ScriptingHost.GetScriptInterface().SetProperty(coords.get(), "z", z, false);
	ScriptEvent("worldclick", coords);

	UNUSED2(button);
	UNUSED2(clicks);
}

// render view rect : John M. Mena
// This sets up and draws the rectangle on the mini-map
// which represents the view of the camera in the world.
void CMiniMap::DrawViewRect()
{
	// Compute the camera frustum intersected with a fixed-height plane.
	// TODO: Currently we hard-code the height, so this'll be dodgy when maps aren't the
	// expected height - how can we make it better without the view rect wobbling in
	// size while the player scrolls?
	float h = 16384.f * HEIGHT_SCALE;

	CVector3D hitPt[4];
	hitPt[0]=m_Camera->GetWorldCoordinates(0, g_Renderer.GetHeight(), h);
	hitPt[1]=m_Camera->GetWorldCoordinates(g_Renderer.GetWidth(), g_Renderer.GetHeight(), h);
	hitPt[2]=m_Camera->GetWorldCoordinates(g_Renderer.GetWidth(), 0, h);
	hitPt[3]=m_Camera->GetWorldCoordinates(0, 0, h);

	float ViewRect[4][2];
	for (int i=0;i<4;i++) {
		// convert to minimap space
		float px=hitPt[i].X;
		float pz=hitPt[i].Z;
		ViewRect[i][0]=(m_CachedActualSize.GetWidth()*px/float(CELL_SIZE*m_MapSize));
		ViewRect[i][1]=(m_CachedActualSize.GetHeight()*pz/float(CELL_SIZE*m_MapSize));
	}

	// Enable Scissoring as to restrict the rectangle
	// to only the mini-map below by retrieving the mini-maps
	// screen coords.
	const float x = m_CachedActualSize.left, y = m_CachedActualSize.bottom;
	glScissor((int)x, g_Renderer.GetHeight()-(int)y, (int)m_CachedActualSize.GetWidth(), (int)m_CachedActualSize.GetHeight());
	glEnable(GL_SCISSOR_TEST);
	glEnable(GL_LINE_SMOOTH);
	glLineWidth(2.0f);
	glColor3f(1.0f, 0.3f, 0.3f);

	// Draw the viewing rectangle with the ScEd's conversion algorithm
	glBegin(GL_LINE_LOOP);
	glVertex2f(ViewRect[0][0], -ViewRect[0][1]);
	glVertex2f(ViewRect[1][0], -ViewRect[1][1]);
	glVertex2f(ViewRect[2][0], -ViewRect[2][1]);
	glVertex2f(ViewRect[3][0], -ViewRect[3][1]);
	glEnd();

	// restore state
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_LINE_SMOOTH);
	glLineWidth(1.0f);
}

struct MinimapUnitVertex
{
	u8 r, g, b, a;
	float x, y;
};

void CMiniMap::DrawTexture(float coordMax, float angle, float x, float y, float x2, float y2, float z)
{
	// Rotate the texture coordinates (0,0)-(coordMax,coordMax) around their center point (m,m)
	const float s = sin(angle);
	const float c = cos(angle);
	const float m = coordMax / 2.f;

	glBegin(GL_QUADS);
	glTexCoord2f(m*(-c + s + 1.f), m*(-c + -s + 1.f));
	glVertex3f(x, y, z);
	glTexCoord2f(m*(c + s + 1.f), m*(-c + s + 1.f));
	glVertex3f(x2, y, z);
	glTexCoord2f(m*(c + -s + 1.f), m*(c + s + 1.f));
	glVertex3f(x2, y2, z);
	glTexCoord2f(m*(-c + -s + 1.f), m*(c + -s + 1.f));
	glVertex3f(x, y2, z);
	glEnd();
}

void CMiniMap::Draw()
{
	PROFILE("minimap");

	// The terrain isn't actually initialized until the map is loaded, which
	// happens when the game is started, so abort until then.
	if(!(GetGUI() && g_Game && g_Game->IsGameStarted()))
		return;

	// Set our globals in case they hadn't been set before
	m_Camera      = g_Game->GetView()->GetCamera();
	m_Terrain     = g_Game->GetWorld()->GetTerrain();
	m_Width  = (u32)(m_CachedActualSize.right - m_CachedActualSize.left);
	m_Height = (u32)(m_CachedActualSize.bottom - m_CachedActualSize.top);
	m_MapSize = m_Terrain->GetVerticesPerSide();
	m_TextureSize = (GLsizei)round_up_to_pow2((size_t)m_MapSize);

	if(!m_TerrainTexture || g_GameRestarted)
		CreateTextures();


	// only update 2x / second
	// (note: since units only move a few pixels per second on the minimap,
	// we can get away with infrequent updates; this is slow)
	static double last_time;
	const double cur_time = timer_Time();
	if(cur_time - last_time > 0.5)
	{
		last_time = cur_time;

		if(m_TerrainDirty)
			RebuildTerrainTexture();
	}

	const float x = m_CachedActualSize.left, y = m_CachedActualSize.bottom;
	const float x2 = m_CachedActualSize.right, y2 = m_CachedActualSize.top;
	const float z = GetBufferedZ();
	const float texCoordMax = (float)(m_MapSize - 1) / (float)m_TextureSize;
	const float angle = GetAngle();

	// Draw the main textured quad
	g_Renderer.BindTexture(0, m_TerrainTexture);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	DrawTexture(texCoordMax, angle, x, y, x2, y2, z);

	/* // TODO: reimplement with new sim system
	// Shade territories by player
	CTerritoryManager* territoryMgr = g_Game->GetWorld()->GetTerritoryManager();
	std::vector<CTerritory*>& territories = territoryMgr->GetTerritories();

	PROFILE_START("minimap territory shade");
	
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	for( size_t i=0; i<territories.size(); i++ )
	{
		if( territories[i]->owner->GetPlayerID() == 0 )
			continue;
		std::vector<CVector2D>& boundary = territories[i]->boundary;
		SPlayerColour col = territories[i]->owner->GetColour();
		glColor4f(col.r, col.g, col.b, 0.25f);
		glBegin(GL_POLYGON);
		for( size_t j=0; j<boundary.size(); j++ )
		{
			float fx = boundary[j].x / (m_Terrain->GetTilesPerSide() * CELL_SIZE);
			float fy = boundary[j].y / (m_Terrain->GetTilesPerSide() * CELL_SIZE);
			glVertex3f( x*(1-fx) + x2*fx, y*(1-fy) + y2*fy, z );
		}
		glEnd();
	}
	glDisable(GL_BLEND);

	PROFILE_END("minimap territory shade");

	// Draw territory boundaries
	glEnable(GL_LINE_SMOOTH);
	glLineWidth(1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0.8f, 0.8f, 0.8f, 0.8f);
	for( size_t i=0; i<territories.size(); i++ )
	{
		std::vector<CVector2D>& boundary = territories[i]->boundary;
		glBegin(GL_LINE_LOOP);
		for( size_t j=0; j<boundary.size(); j++ )
		{
			float fx = boundary[j].x / (m_Terrain->GetTilesPerSide() * CELL_SIZE);
			float fy = boundary[j].y / (m_Terrain->GetTilesPerSide() * CELL_SIZE);
			glVertex3f( x*(1-fx) + x2*fx, y*(1-fy) + y2*fy, z );
		}
		glEnd();
	}
	glLineWidth(1.0f);
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);
	*/

	// Draw the LOS quad in black, using alpha values from the LOS texture
	CLOSTexture& losTexture = g_Game->GetView()->GetLOSTexture();
	losTexture.BindTexture(0);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor3f(0.0f, 0.0f, 0.0f);

	glMatrixMode(GL_TEXTURE);
	glLoadMatrixf(losTexture.GetMinimapTextureMatrix());
	glMatrixMode(GL_MODELVIEW);

	DrawTexture(1.0f, angle, x, y, x2, y2, z);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	glDisable(GL_BLEND);

	// Set up the matrix for drawing points and lines
	glPushMatrix();
	glTranslatef(x, y, z);
	// Rotate around the center of the map
	glTranslatef((x2-x)/2.f, (y2-y)/2.f, 0.f);
	glRotatef(angle * 180.f/M_PI, 0.f, 0.f, 1.f);
	glTranslatef(-(x2-x)/2.f, -(y2-y)/2.f, 0.f);

	PROFILE_START("minimap units");

	// Don't enable GL_POINT_SMOOTH because it's far too slow
	// (~70msec/frame on a GF4 rendering a thousand points)
	glPointSize(3.f);

	float sx = (float)m_Width / ((m_MapSize - 1) * CELL_SIZE);
	float sy = (float)m_Height / ((m_MapSize - 1) * CELL_SIZE);

	CSimulation2* sim = g_Game->GetSimulation2();
	CSimulation2::InterfaceList ents = sim->GetEntitiesWithInterface(IID_Minimap);

	std::vector<MinimapUnitVertex> vertexArray;
	vertexArray.reserve(ents.size());

	CmpPtr<ICmpRangeManager> cmpRangeManager(*sim, SYSTEM_ENTITY);
	debug_assert(!cmpRangeManager.null());

	for (CSimulation2::InterfaceList::const_iterator it = ents.begin(); it != ents.end(); ++it)
	{
		MinimapUnitVertex v;
		ICmpMinimap* cmpMinimap = static_cast<ICmpMinimap*>(it->second);
		entity_pos_t posX, posZ;
		if (cmpMinimap->GetRenderData(v.r, v.g, v.b, posX, posZ))
		{
			ICmpRangeManager::ELosVisibility vis = cmpRangeManager->GetLosVisibility(it->first, g_Game->GetPlayerID());
			if (vis != ICmpRangeManager::VIS_HIDDEN)
			{
				v.a = 255;
				v.x = posX.ToFloat()*sx;
				v.y = -posZ.ToFloat()*sy;
				vertexArray.push_back(v);
			}
		}
	}

	if (!vertexArray.empty())
	{
		glInterleavedArrays(GL_C4UB_V2F, sizeof(MinimapUnitVertex), &vertexArray[0]);
		glDrawArrays(GL_POINTS, 0, (GLsizei)vertexArray.size());

		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
	}

	PROFILE_END("minimap units");

	DrawViewRect();

	glPopMatrix();

	// Reset everything back to normal
	glPointSize(1.0f);
	glEnable(GL_TEXTURE_2D);
}

void CMiniMap::CreateTextures()
{
	Destroy();

	// Create terrain texture
	glGenTextures(1, &m_TerrainTexture);
	g_Renderer.BindTexture(0, m_TerrainTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_TextureSize, m_TextureSize, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, 0);
	m_TerrainData = new u32[(m_MapSize - 1) * (m_MapSize - 1)];
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

	// Rebuild and upload both of them
	RebuildTerrainTexture();
}


void CMiniMap::RebuildTerrainTexture()
{
	u32 x = 0;
	u32 y = 0;
	u32 w = m_MapSize - 1;
	u32 h = m_MapSize - 1;
	float waterHeight = g_Renderer.GetWaterManager()->m_WaterHeight;

	m_TerrainDirty = false;

	for(u32 j = 0; j < h; j++)
	{
		u32 *dataPtr = m_TerrainData + ((y + j) * (m_MapSize - 1)) + x;
		for(u32 i = 0; i < w; i++)
		{
			float avgHeight = ( m_Terrain->GetVertexGroundLevel((int)i, (int)j)
					+ m_Terrain->GetVertexGroundLevel((int)i+1, (int)j)
					+ m_Terrain->GetVertexGroundLevel((int)i, (int)j+1)
					+ m_Terrain->GetVertexGroundLevel((int)i+1, (int)j+1)
				) / 4.0f;

			if(avgHeight < waterHeight)
			{
				*dataPtr++ = 0xff304080;		// TODO: perhaps use the renderer's water color?
			}
			else
			{
				int hmap = ((int)m_Terrain->GetHeightMap()[(y + j) * m_MapSize + x + i]) >> 8;
				int val = (hmap / 3) + 170;

				u32 color = 0xFFFFFFFF;

				CMiniPatch *mp = m_Terrain->GetTile(x + i, y + j);
				if(mp)
				{
					CTerrainTextureEntry *tex = mp->GetTextureEntry();
					if(tex)
					{
						// If the texture can't be loaded yet, set the dirty flags
						// so we'll try regenerating the terrain texture again soon
						if(!tex->GetTexture()->TryLoad())
							m_TerrainDirty = true;

						color = tex->GetBaseColor();
					}
				}

				*dataPtr++ = ScaleColor(color, float(val) / 255.0f);
			}
		}
	}

	// Upload the texture
	g_Renderer.BindTexture(0, m_TerrainTexture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_MapSize - 1, m_MapSize - 1, GL_BGRA_EXT, GL_UNSIGNED_BYTE, m_TerrainData);
}

void CMiniMap::Destroy()
{
	if(m_TerrainTexture)
	{
		glDeleteTextures(1, &m_TerrainTexture);
		m_TerrainTexture = 0;
	}

	delete[] m_TerrainData;
	m_TerrainData = 0;
}
