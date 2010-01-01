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

#include <math.h>

#include "MiniMap.h"

#include "graphics/GameView.h"
#include "graphics/MiniPatch.h"
#include "graphics/Terrain.h"
#include "graphics/TextureEntry.h"
#include "graphics/TextureManager.h"
#include "graphics/Unit.h"
#include "graphics/UnitManager.h"
#include "lib/ogl.h"
#include "lib/external_libraries/sdl.h"
#include "lib/bits.h"
#include "lib/timer.h"
#include "network/NetMessage.h"
#include "ps/Game.h"
#include "ps/Interact.h"
#include "ps/Player.h"
#include "ps/Profile.h"
#include "ps/World.h"
#include "renderer/Renderer.h"
#include "renderer/WaterManager.h"
#include "scripting/GameEvents.h"
#include "simulation/Entity.h"
#include "simulation/EntityTemplate.h"
#include "simulation/LOSManager.h"
#include "simulation/TerritoryManager.h"


bool g_TerrainModified = false;
bool g_GameRestarted = false;

// used by GetMapSpaceCoords (precalculated as an optimization).
// this was formerly access via inline asm, which required it to be
// static data instead of a class member. that is no longer the case,
// but we leave it because this is slightly more efficient.
static float m_scaleX, m_scaleY;


static unsigned int ScaleColor(unsigned int color, float x)
{
	unsigned int r = unsigned(float(color & 0xff) * x);
	unsigned int g = unsigned(float((color>>8) & 0xff) * x);
	unsigned int b = unsigned(float((color>>16) & 0xff) * x);
	return (0xff000000 | r | g<<8 | b<<16);
}

CMiniMap::CMiniMap()
	: m_TerrainTexture(0), m_TerrainData(0), m_MapSize(0), m_Terrain(0),
	m_LOSTexture(0), m_LOSData(0), m_UnitManager(0)
{
	AddSetting(GUIST_CColor,	"fov_wedge_color");
	AddSetting(GUIST_CStr,		"tooltip");
	AddSetting(GUIST_CStr,		"tooltip_style");
	m_Clicking = false;
}

CMiniMap::~CMiniMap()
{
	Destroy();
}

void CMiniMap::HandleMessage(const SGUIMessage &Message)
{
	switch(Message.type)
	{
	case GUIM_MOUSE_PRESS_LEFT:
		{
			SetCameraPos();
			m_Clicking = true;
			break;
		}
	case GUIM_MOUSE_RELEASE_LEFT:
		{
			if(m_Clicking)
				SetCameraPos();
			m_Clicking = false;
			break;
		}
	case GUIM_MOUSE_DBLCLICK_LEFT:
		{
			if(m_Clicking)
				SetCameraPos();
			m_Clicking = false;
			break;
		}
	case GUIM_MOUSE_ENTER:
		{
			g_Selection.m_mouseOverMM = true;
			break;
		}
	case GUIM_MOUSE_LEAVE:
		{
			g_Selection.m_mouseOverMM = false;
			m_Clicking = false;
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
			if (m_Clicking)
				SetCameraPos();
			break;
		}

	default:
		break;
	}	// switch
}

void CMiniMap::SetCameraPos()
{
	CTerrain *MMTerrain=g_Game->GetWorld()->GetTerrain();
	CVector3D CamOrient=m_Camera->m_Orientation.GetTranslation();

	//get center point of screen
	int x = g_Renderer.GetWidth()/2;
	int y = g_Renderer.GetHeight()/2;
	CVector3D ScreenMiddle=m_Camera->GetWorldCoordinates(x,y);

	//Get Vector required to go from camera position to ScreenMiddle
	CVector3D TransVector;
	TransVector.X=CamOrient.X-ScreenMiddle.X;
	TransVector.Z=CamOrient.Z-ScreenMiddle.Z;
	//world position of where mouse clicked
	CVector3D Destination;
	CPos MousePos = GetMousePos();
	//X and Z according to proportion of mouse position and minimap
	Destination.X = CELL_SIZE * m_MapSize *
		( (MousePos.x - m_CachedActualSize.left) / m_CachedActualSize.GetWidth() );
	Destination.Z = CELL_SIZE * m_MapSize * ( (m_CachedActualSize.bottom - MousePos.y) /
		m_CachedActualSize.GetHeight() );

	m_Camera->m_Orientation._14=Destination.X;
	m_Camera->m_Orientation._34=Destination.Z;
	m_Camera->m_Orientation._14+=TransVector.X;
	m_Camera->m_Orientation._34+=TransVector.Z;

	//Lock Y coord.  No risk of zoom exceeding limit-Y does not increase
	float Height=MMTerrain->GetExactGroundLevel(
		m_Camera->m_Orientation._14, m_Camera->m_Orientation._34) + g_YMinOffset;

	if (m_Camera->m_Orientation._24 < Height)
	{
		m_Camera->m_Orientation._24=Height;
	}
	m_Camera->UpdateFrustum();
}
void CMiniMap::FireWorldClickEvent(int button, int clicks)
{
	//debug_printf(L"FireWorldClickEvent: button %d, clicks %d\n", button, clicks);
	
	CPos MousePos = GetMousePos();
	CVector2D Destination;
	//X and Z according to proportion of mouse position and minimap
	Destination.x = CELL_SIZE * m_MapSize *
		( (MousePos.x - m_CachedActualSize.left) / m_CachedActualSize.GetWidth() );
	Destination.y = CELL_SIZE * m_MapSize * ( (m_CachedActualSize.bottom - MousePos.y) /
		m_CachedActualSize.GetHeight() );

	g_JSGameEvents.FireWorldClick(
		button,
		clicks,
		NMT_GOTO,
		-1,
		NMT_RUN,
		-1, 
		NULL,
		(int)Destination.x,
		(int)Destination.y);
}

// render view rect : John M. Mena
// This sets up and draws the rectangle on the mini-map
// which represents the view of the camera in the world.
void CMiniMap::DrawViewRect()
{
	// Get correct world coordinates based off corner of screen start
	// at Bottom Left and going CW
	CVector3D hitPt[4];
	hitPt[0]=m_Camera->GetWorldCoordinates(0,g_Renderer.GetHeight());
	hitPt[1]=m_Camera->GetWorldCoordinates(g_Renderer.GetWidth(),g_Renderer.GetHeight());
	hitPt[2]=m_Camera->GetWorldCoordinates(g_Renderer.GetWidth(),0);
	hitPt[3]=m_Camera->GetWorldCoordinates(0,0);

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
	glScissor((int)m_CachedActualSize.left, 0, (int)m_CachedActualSize.right, (int)m_CachedActualSize.GetHeight());
	glEnable(GL_SCISSOR_TEST);
	glEnable(GL_LINE_SMOOTH);
	glLineWidth(2.0f);
	glColor3f(1.0f, 0.3f, 0.3f);

	// Draw the viewing rectangle with the ScEd's conversion algorithm
	const float x = m_CachedActualSize.left, y = m_CachedActualSize.bottom;
	glBegin(GL_LINE_LOOP);
	glVertex2f(x+ViewRect[0][0], y-ViewRect[0][1]);
	glVertex2f(x+ViewRect[1][0], y-ViewRect[1][1]);
	glVertex2f(x+ViewRect[2][0], y-ViewRect[2][1]);
	glVertex2f(x+ViewRect[3][0], y-ViewRect[3][1]);
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

void CMiniMap::Draw()
{
	PROFILE("minimap");

	// The terrain isn't actually initialized until the map is loaded, which
	// happens when the game is started, so abort until then.
	if(!(GetGUI() && g_Game && g_Game->IsGameStarted()))
		return;
	
	glDisable(GL_DEPTH_TEST);

	// Set our globals in case they hadn't been set before
	m_Camera      = g_Game->GetView()->GetCamera();
	m_Terrain     = g_Game->GetWorld()->GetTerrain();
	m_UnitManager = &g_Game->GetWorld()->GetUnitManager();
	m_Width  = (u32)(m_CachedActualSize.right - m_CachedActualSize.left);
	m_Height = (u32)(m_CachedActualSize.bottom - m_CachedActualSize.top);
	m_MapSize = m_Terrain->GetVerticesPerSide();
	m_TextureSize = (GLsizei)round_up_to_pow2((size_t)m_MapSize);

	m_scaleX = float(m_Width) / float(m_MapSize - 1);
	m_scaleY = float(m_Height) / float(m_MapSize - 1);

	if(!m_TerrainTexture || g_GameRestarted)
		CreateTextures();

	// do not limit this as with LOS updates below - we must update
	// immediately after changes are reported because this flag will be
	// reset at the end of the frame.
	if(g_TerrainModified)
		RebuildTerrainTexture();

	// only update 10x / second
	// (note: since units only move a few pixels per second on the minimap,
	// we can get away with infrequent updates; this is slow, ~20ms)
	static double last_time;
	const double cur_time = timer_Time();
	if(cur_time - last_time > 100e-3)	// 10 updates/sec
	{
		last_time = cur_time;

		CLOSManager* losMgr = g_Game->GetWorld()->GetLOSManager();
		if(losMgr->m_LOSSetting != LOS_SETTING_ALL_VISIBLE)
			RebuildLOSTexture();
	}

	const float texCoordMax = ((float)m_MapSize - 1) / ((float)m_TextureSize);
	const float x = m_CachedActualSize.left, y = m_CachedActualSize.bottom;
	const float x2 = m_CachedActualSize.right, y2 = m_CachedActualSize.top;
	const float z = GetBufferedZ();

	// Draw the main textured quad
	g_Renderer.BindTexture(0, m_TerrainTexture);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(x, y, z);
	glTexCoord2f(texCoordMax, 0.0f);
	glVertex3f(x2, y, z);
	glTexCoord2f(texCoordMax, texCoordMax);
	glVertex3f(x2, y2, z);
	glTexCoord2f(0.0f, texCoordMax);
	glVertex3f(x, y2, z);
	glEnd();

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

	// Draw the LOS quad in black, using alpha values from the LOS texture
	g_Renderer.BindTexture(0, m_LOSTexture);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);
	glColor3f(0.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(x, y, z);
	glTexCoord2f(texCoordMax, 0.0f);
	glVertex3f(x2, y, z);
	glTexCoord2f(texCoordMax, texCoordMax);
	glVertex3f(x2, y2, z);
	glTexCoord2f(0.0f, texCoordMax);
	glVertex3f(x, y2, z);
	glEnd();
	glDisable(GL_BLEND);

	PROFILE_START("minimap units");

	// Draw unit points
	const std::vector<CUnit *> &units = m_UnitManager->GetUnits();
	std::vector<CUnit *>::const_iterator iter = units.begin();
	CUnit *unit = 0;
	CVector2D pos;
	CLOSManager* losMgr = g_Game->GetWorld()->GetLOSManager();

	std::vector<MinimapUnitVertex> vertexArray;
	// TODO: don't reallocate this after every frame (but don't waste memory
	// after the number of units decreases substantially)

	// Don't enable GL_POINT_SMOOTH because it's far too slow
	// (~70msec/frame on a GF4 rendering a thousand points)
	glPointSize(3.f);

	for(; iter != units.end(); ++iter)
	{
		unit = (CUnit *)(*iter);
		if(unit && unit->GetEntity() && losMgr->GetUnitStatus(unit, g_Game->GetLocalPlayer()) != UNIT_HIDDEN)
		{
			CEntity* entity = unit->GetEntity();
			CStrW& type = entity->m_base->m_minimapType;

			MinimapUnitVertex v;

			if(type==L"Unit" || type==L"Structure" || type==L"Hero") {
				// Use the player colour
				const SPlayerColour& colour = entity->GetPlayer()->GetColour();
				v.r = cpu_i32FromFloat(colour.r*255.f);
				v.g = cpu_i32FromFloat(colour.g*255.f);
				v.b = cpu_i32FromFloat(colour.b*255.f);
				v.a = 255;
			}
			else {
				CEntityTemplate* base = entity->m_base;
				v.r = base->m_minimapR;
				v.g = base->m_minimapG;
				v.b = base->m_minimapB;
				v.a = 255;
			}

			pos = GetMapSpaceCoords(entity->m_position);

			v.x = x + pos.x;
			v.y = y - pos.y;
			vertexArray.push_back(v);
		}
	}	

	if (vertexArray.size())
	{
		glPushMatrix();
		glTranslatef(0, 0, z);

		// Unbind any vertex buffer object, if our card supports VBO's
		if (g_Renderer.GetCapabilities().m_VBO)
		{
			pglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		}

		glInterleavedArrays(GL_C4UB_V2F, sizeof(MinimapUnitVertex), &vertexArray[0]);
		glDrawArrays(GL_POINTS, 0, (GLsizei)vertexArray.size());

		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);

		glPopMatrix();
	}

	PROFILE_END("minimap units");

	DrawViewRect();

	// Reset everything back to normal
	glPointSize(1.0f);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
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
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);

	// Create LOS texture
	glGenTextures(1, &m_LOSTexture);
	g_Renderer.BindTexture(0, m_LOSTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA8, m_TextureSize, m_TextureSize, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);
	m_LOSData = new u8[(m_MapSize - 1) * (m_MapSize - 1)];
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);

	// Rebuild and upload both of them
	RebuildTerrainTexture();
	RebuildLOSTexture();
}


void CMiniMap::RebuildTerrainTexture()
{
	u32 x = 0;
	u32 y = 0;
	u32 w = m_MapSize - 1;
	u32 h = m_MapSize - 1;
	float waterHeight = g_Renderer.GetWaterManager()->m_WaterHeight;

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
				if(mp && mp->Tex1)
				{
					CTextureEntry *tex = g_TexMan.FindTexture(mp->Tex1);
					if(tex)
						color = tex->GetBaseColor();
				}

				*dataPtr++ = ScaleColor(color, float(val) / 255.0f);
			}
		}
	}

	// Upload the texture
	g_Renderer.BindTexture(0, m_TerrainTexture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_MapSize - 1, m_MapSize - 1, GL_BGRA_EXT, GL_UNSIGNED_BYTE, m_TerrainData);
}


void CMiniMap::RebuildLOSTexture()
{
	PROFILE_START("rebuild minimap: los");

	CLOSManager* losMgr = g_Game->GetWorld()->GetLOSManager();
	CPlayer* player = g_Game->GetLocalPlayer();

	ssize_t x = 0;
	ssize_t y = 0;
	ssize_t w = m_MapSize - 1;
	ssize_t h = m_MapSize - 1;

	for(ssize_t j = 0; j < h; j++)
	{
		u8 *dataPtr = m_LOSData + ((y + j) * (m_MapSize - 1)) + x;
		for(ssize_t i = 0; i < w; i++)
		{
			ELOSStatus status = losMgr->GetStatus(i, j, player);
			if(status == LOS_UNEXPLORED)
				*dataPtr++ = 0xff;
			else if(status == LOS_EXPLORED)
				*dataPtr++ = (u8) (0xff * 0.3f);
			else
				*dataPtr++ = 0;
		}
	}

	// Upload the texture
	g_Renderer.BindTexture(0, m_LOSTexture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_MapSize - 1, m_MapSize - 1, GL_ALPHA, GL_UNSIGNED_BYTE, m_LOSData);

	PROFILE_END("rebuild minimap: los");
}

void CMiniMap::Destroy()
{
	if(m_TerrainTexture)
		glDeleteTextures(1, &m_TerrainTexture);

	if(m_LOSTexture)
		glDeleteTextures(1, &m_LOSTexture);

	delete[] m_TerrainData; m_TerrainData = 0;
	delete[] m_LOSData; m_LOSData = 0;
}

CVector2D CMiniMap::GetMapSpaceCoords(CVector3D worldPos)
{
	float x = rintf(worldPos.X / CELL_SIZE);
	float y = rintf(worldPos.Z / CELL_SIZE);
	// Entity's Z coordinate is really its longitudinal coordinate on the terrain

	// Calculate map space scale
	return CVector2D(x * m_scaleX, y * m_scaleY);
}
