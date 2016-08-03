/* Copyright (C) 2016 Wildfire Games.
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

#include <sstream>
#include <string>

#include "graphics/Camera.h"
#include "graphics/CinemaManager.h"
#include "graphics/GameView.h"
#include "gui/CGUI.h"
#include "gui/GUIManager.h"
#include "gui/IGUIObject.h"
#include "lib/ogl.h"
#include "maths/MathUtil.h"
#include "maths/Quaternion.h"
#include "maths/Vector3D.h"
#include "maths/Vector4D.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Game.h"
#include "ps/GameSetup/Config.h"
#include "ps/Hotkey.h"
#include "simulation2/components/ICmpOverlayRenderer.h"
#include "simulation2/components/ICmpRangeManager.h"
#include "simulation2/components/ICmpSelectable.h"
#include "simulation2/components/ICmpTerritoryManager.h"
#include "simulation2/MessageTypes.h"
#include "simulation2/system/ComponentManager.h"
#include "simulation2/Simulation2.h"
#include "renderer/Renderer.h"


CCinemaManager::CCinemaManager()
	: m_DrawPaths(false)
{
}

void CCinemaManager::AddPath(const CStrW& name, const CCinemaPath& path)
{
	if (m_CinematicSimulationData.m_Paths.find(name) != m_CinematicSimulationData.m_Paths.end())
	{
		LOGWARNING("Path with name '%s' already exists", name.ToUTF8());
		return;
	}
	m_CinematicSimulationData.m_Paths[name] = path;
}

void CCinemaManager::AddPathToQueue(const CStrW& name)
{
	if (!HasPath(name))
	{
		LOGWARNING("Path with name '%s' doesn't exist", name.ToUTF8());
		return;
	}
	m_CinematicSimulationData.m_PathQueue.push_back(m_CinematicSimulationData.m_Paths[name]);
}

void CCinemaManager::ClearQueue()
{
	m_CinematicSimulationData.m_PathQueue.clear();
}

void CCinemaManager::SetAllPaths(const std::map<CStrW, CCinemaPath>& paths)
{
	m_CinematicSimulationData.m_Paths = paths;
}

bool CCinemaManager::HasPath(const CStrW& name) const
{
	return m_CinematicSimulationData.m_Paths.find(name) != m_CinematicSimulationData.m_Paths.end();
}

void CCinemaManager::SetEnabled(bool enabled)
{
	// TODO: maybe assert?
	if (m_CinematicSimulationData.m_PathQueue.empty() && enabled)
	{
		enabled = false;
		m_CinematicSimulationData.m_Paused = true;
	}

	if (m_CinematicSimulationData.m_Enabled == enabled)
		return;

	// TODO: Enabling/Disabling does not work if the session GUI page is not the top page.
	// This can happen in various situations, for example when the player wins/looses the game
	// while the cinematic is running (a message box is the top page in this case). 
	// It might be better to disable the whole GUI during the cinematic instead of a specific 
	// GUI object.
	
	// sn - session gui object
	IGUIObject *sn = g_GUI->FindObjectByName("sn");
	CmpPtr<ICmpRangeManager> cmpRangeManager(g_Game->GetSimulation2()->GetSimContext().GetSystemEntity());
	CmpPtr<ICmpTerritoryManager> cmpTerritoryManager(g_Game->GetSimulation2()->GetSimContext().GetSystemEntity());

	// GUI visibility
	if (sn)
	{
		if (enabled)
			sn->SetSetting("hidden", L"true");
		else
			sn->SetSetting("hidden", L"false");
	}

	// Overlay visibility
	g_Renderer.SetOptionBool(CRenderer::Option::OPT_SILHOUETTES, !enabled);
	if (cmpRangeManager)
	{
		if (enabled)
			m_CinematicSimulationData.m_MapRevealed = cmpRangeManager->GetLosRevealAll(-1);
		// TODO: improve m_MapRevealed state and without fade in
		cmpRangeManager->SetLosRevealAll(-1, enabled);
	}
	if (cmpTerritoryManager)
		cmpTerritoryManager->SetVisibility(!enabled);
	ICmpSelectable::SetOverrideVisibility(!enabled);
	ICmpOverlayRenderer::SetOverrideVisibility(!enabled);

	m_CinematicSimulationData.m_Enabled = enabled;
}

void CCinemaManager::Play()
{
	m_CinematicSimulationData.m_Paused = false;
}

void CCinemaManager::Stop()
{
	m_CinematicSimulationData.m_PathQueue.clear();
}

void CCinemaManager::Update(const float deltaRealTime)
{
	if (g_Game->m_Paused != m_CinematicSimulationData.m_Paused)
	{
		m_CinematicSimulationData.m_Paused = g_Game->m_Paused;

		// sn - session gui object
		IGUIObject *sn = g_GUI->FindObjectByName("sn");

		// GUI visibility
		if (sn)
		{
			if (m_CinematicSimulationData.m_Paused)
				sn->SetSetting("hidden", L"false");
			else
				sn->SetSetting("hidden", L"true");
		}
	}

	if (m_CinematicSimulationData.m_PathQueue.empty() || !m_CinematicSimulationData.m_Enabled || m_CinematicSimulationData.m_Paused)
		return;
	
	if (HotkeyIsPressed("leave"))
	{
		// TODO: implement skip
	}
	
	m_CinematicSimulationData.m_PathQueue.front().Play(deltaRealTime);
}

void CCinemaManager::Render()
{
	if (GetEnabled())
	{
		DrawBars();
		return;
	}
	
	if (!m_DrawPaths)
		return;

	// draw all paths
	for (const std::pair<CStrW, CCinemaPath>& p : m_CinematicSimulationData.m_Paths)
		p.second.Draw();
}

void CCinemaManager::DrawBars() const
{
	int height = (float)g_xres / 2.39f;
	int shift = (g_yres - height) / 2;
	if (shift <= 0)
		return;

#if CONFIG2_GLES
	#warning TODO : implement bars for GLES
#else
	// Set up transform for GL bars
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	CMatrix3D transform;
	transform.SetOrtho(0.f, (float)g_xres, 0.f, (float)g_yres, -1.f, 1000.f);
	glLoadMatrixf(&transform._11);

	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	glBegin(GL_QUADS);
	glVertex2i(0, 0);
	glVertex2i(g_xres, 0);
	glVertex2i(g_xres, shift);
	glVertex2i(0, shift);
	glEnd();

	glBegin(GL_QUADS);
	glVertex2i(0, g_yres - shift);
	glVertex2i(g_xres, g_yres - shift);
	glVertex2i(g_xres, g_yres);
	glVertex2i(0, g_yres);
	glEnd();

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

	// Restore transform
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
#endif
}

InReaction cinema_manager_handler(const SDL_Event_* ev)
{
	// put any events that must be processed even if inactive here
	if (!g_Game || !g_Game->IsGameStarted())
		return IN_PASS;

	CCinemaManager* pCinemaManager = g_Game->GetView()->GetCinema();

	return pCinemaManager->HandleEvent(ev);
}

InReaction CCinemaManager::HandleEvent(const SDL_Event_* ev)
{
	switch (ev->ev.type)
	{
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		if (GetEnabled() && !m_CinematicSimulationData.m_Paused)
			return IN_HANDLED;
	default:
		return IN_PASS;
	}
}

bool CCinemaManager::GetEnabled() const
{
	return m_CinematicSimulationData.m_Enabled;
}

bool CCinemaManager::IsPlaying() const
{
	return !m_CinematicSimulationData.m_Paused;
}

const std::map<CStrW, CCinemaPath>& CCinemaManager::GetAllPaths()
{
	return m_CinematicSimulationData.m_Paths;
}

CinematicSimulationData* CCinemaManager::GetCinematicSimulationData()
{
	return &m_CinematicSimulationData;
}

bool CCinemaManager::GetPathsDrawing() const
{
	return m_DrawPaths;
}

void CCinemaManager::SetPathsDrawing(const bool drawPath)
{
	m_DrawPaths = drawPath;
}
