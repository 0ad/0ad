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

#include "View.h"

#include "ActorViewer.h"
#include "GameLoop.h"
#include "Messages.h"
#include "SimState.h"

#include "graphics/CinemaTrack.h"
#include "graphics/GameView.h"
#include "graphics/ParticleManager.h"
#include "graphics/SColor.h"
#include "graphics/UnitManager.h"
#include "lib/timer.h"
#include "lib/utf8.h"
#include "maths/MathUtil.h"
#include "ps/Game.h"
#include "ps/GameSetup/GameSetup.h"
#include "ps/World.h"
#include "renderer/Renderer.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpObstructionManager.h"
#include "simulation2/components/ICmpPathfinder.h"

extern void (*Atlas_GLSwapBuffers)(void* context);

extern int g_xres, g_yres;

//////////////////////////////////////////////////////////////////////////

void View::SetParam(const std::wstring& UNUSED(name), bool UNUSED(value))
{
}

void View::SetParam(const std::wstring& UNUSED(name), const AtlasMessage::Colour& UNUSED(value))
{
}

void View::SetParam(const std::wstring& UNUSED(name), const std::wstring& UNUSED(value))
{
}

//////////////////////////////////////////////////////////////////////////

ViewActor::ViewActor()
: m_SpeedMultiplier(1.f), m_ActorViewer(new ActorViewer())
{
}

ViewActor::~ViewActor()
{
	delete m_ActorViewer;
}

void ViewActor::Update(float frameLength)
{
	m_ActorViewer->Update(frameLength * m_SpeedMultiplier);
}

void ViewActor::Render()
{
	SViewPort vp = { 0, 0, g_xres, g_yres };
	CCamera& camera = GetCamera();
	camera.SetViewPort(vp);
	camera.SetProjection(2.f, 512.f, DEGTORAD(20.f));
	camera.UpdateFrustum();

	m_ActorViewer->Render();
	Atlas_GLSwapBuffers((void*)g_GameLoop->glCanvas);
}

CCamera& ViewActor::GetCamera()
{
	return m_Camera;
}

CSimulation2* ViewActor::GetSimulation2()
{
	return m_ActorViewer->GetSimulation2();
}

entity_id_t ViewActor::GetEntityId(AtlasMessage::ObjectID UNUSED(obj))
{
	return m_ActorViewer->GetEntity();
}

bool ViewActor::WantsHighFramerate()
{
	if (m_SpeedMultiplier != 0.f)
		return true;

	return false;
}

void ViewActor::SetSpeedMultiplier(float speed)
{
	m_SpeedMultiplier = speed;
}

ActorViewer& ViewActor::GetActorViewer()
{
	return *m_ActorViewer;
}

void ViewActor::SetParam(const std::wstring& name, bool value)
{
	if (name == L"wireframe")
		g_Renderer.SetModelRenderMode(value ? WIREFRAME : SOLID);
	else if (name == L"walk")
		m_ActorViewer->SetWalkEnabled(value);
	else if (name == L"ground")
		m_ActorViewer->SetGroundEnabled(value);
	else if (name == L"shadows")
		m_ActorViewer->SetShadowsEnabled(value);
	else if (name == L"stats")
		m_ActorViewer->SetStatsEnabled(value);
}

void ViewActor::SetParam(const std::wstring& name, const AtlasMessage::Colour& value)
{
	if (name == L"background")
	{
		m_ActorViewer->SetBackgroundColour(SColor4ub(value.r, value.g, value.b, 255));
	}
}


//////////////////////////////////////////////////////////////////////////

template<typename T, typename S>
static void delete_pair_2nd(std::pair<T,S> v)
{
	delete v.second;
}

ViewGame::ViewGame()
: m_SpeedMultiplier(0.f)
{
	ENSURE(g_Game);
}

ViewGame::~ViewGame()
{
	std::for_each(m_SavedStates.begin(), m_SavedStates.end(), delete_pair_2nd<std::wstring, SimState*>);
}

CSimulation2* ViewGame::GetSimulation2()
{
	return g_Game->GetSimulation2();
}

void ViewGame::Update(float frameLength)
{
	float actualFrameLength = frameLength * m_SpeedMultiplier;

	// Clean up any entities destroyed during UI message processing
	g_Game->GetSimulation2()->FlushDestroyedEntities();

	if (m_SpeedMultiplier == 0.f)
	{
		// Update unit interpolation
		g_Game->Interpolate(0.0);

		// Update particles even when the game is paused, so people can see
		// what they look like. (TODO: maybe it'd be nice if this only applied in
		// the not-testing-game editor state, not the testing-game-but-currently-paused
		// state. Or maybe display a static snapshot of the particles (at some time
		// later than 0 so they're actually visible) instead of animation, or something.)
		g_Renderer.GetParticleManager().Interpolate(frameLength);
	}
	else
	{
		// Update the whole world
		// (Tell the game update not to interpolate graphics - we'll do that
		// ourselves)
		bool ok = g_Game->Update(actualFrameLength, false);
		if (! ok)
		{
			// Whoops, we're trying to go faster than the simulation can manage.
			// It's probably better to run at the right sim rate, at the expense
			// of framerate, so let's try simulating a few more times.
			double t = timer_Time();
			while (!ok && timer_Time() < t + 0.1) // don't go much worse than 10fps
			{
				ok = g_Game->Update(0.0, false); // don't add on any extra sim time
			}
		}

		// Interpolate the graphics - we only want to do this once per visual frame,
		// not in every call to g_Game->Update
		g_Game->Interpolate(actualFrameLength);
	}

	// Cinematic motion should be independent of simulation update, so we can
	// preview the cinematics by themselves
	if (g_Game->GetView()->GetCinema()->IsPlaying())
		g_Game->GetView()->GetCinema()->Update(frameLength);
}

void ViewGame::Render()
{
	SViewPort vp = { 0, 0, g_xres, g_yres };
	CCamera& camera = GetCamera();
	camera.SetViewPort(vp);
	camera.SetProjection(CGameView::defaultNear, CGameView::defaultFar, CGameView::defaultFOV);
	camera.UpdateFrustum();

	// Update the pathfinder display if necessary
	if (!m_DisplayPassability.empty())
	{
		CmpPtr<ICmpObstructionManager> cmpObstructionMan(*GetSimulation2(), SYSTEM_ENTITY);
		if (!cmpObstructionMan.null())
		{
			cmpObstructionMan->SetDebugOverlay(true);
		}

		CmpPtr<ICmpPathfinder> cmpPathfinder(*GetSimulation2(), SYSTEM_ENTITY);
		if (!cmpPathfinder.null())
		{
			cmpPathfinder->SetDebugOverlay(true);
			// Kind of a hack to make it update the terrain grid
			ICmpPathfinder::Goal goal = { ICmpPathfinder::Goal::POINT, fixed::Zero(), fixed::Zero() };
			u8 passClass = cmpPathfinder->GetPassabilityClass(m_DisplayPassability);
			u8 costClass = cmpPathfinder->GetCostClass("default");
			cmpPathfinder->SetDebugPath(fixed::Zero(), fixed::Zero(), goal, passClass, costClass);
		}
	}

	::Render();
	Atlas_GLSwapBuffers((void*)g_GameLoop->glCanvas);
}

void ViewGame::SetParam(const std::wstring& name, bool value)
{
	if (name == L"priorities")
		g_Renderer.SetDisplayTerrainPriorities(value);
}

void ViewGame::SetParam(const std::wstring& name, const std::wstring& value)
{
	if (name == L"passability")
	{
		m_DisplayPassability = CStrW(value).ToUTF8();

		CmpPtr<ICmpObstructionManager> cmpObstructionMan(*GetSimulation2(), SYSTEM_ENTITY);
		if (!cmpObstructionMan.null())
			cmpObstructionMan->SetDebugOverlay(!value.empty());

		CmpPtr<ICmpPathfinder> cmpPathfinder(*GetSimulation2(), SYSTEM_ENTITY);
		if (!cmpPathfinder.null())
			cmpPathfinder->SetDebugOverlay(!value.empty());
	}
	else if (name == L"renderpath")
	{
		g_Renderer.SetRenderPath(g_Renderer.GetRenderPathByName(CStrW(value).ToUTF8()));
	}
}

CCamera& ViewGame::GetCamera()
{
	return *g_Game->GetView()->GetCamera();
}

bool ViewGame::WantsHighFramerate()
{
	if (g_Game->GetView()->GetCinema()->IsPlaying())
		return true;

	if (m_SpeedMultiplier != 0.f)
		return true;

	return false;
}

void ViewGame::SetSpeedMultiplier(float speed)
{
	m_SpeedMultiplier = speed;
}

void ViewGame::SaveState(const std::wstring& label)
{
	delete m_SavedStates[label]; // in case it already exists
	m_SavedStates[label] = SimState::Freeze();
}

void ViewGame::RestoreState(const std::wstring& label)
{
	SimState* simState = m_SavedStates[label];
	if (! simState)
		return;

	simState->Thaw();
}

std::wstring ViewGame::DumpState(bool binary)
{
	std::stringstream stream;
	if (binary)
	{
		if (! g_Game->GetSimulation2()->SerializeState(stream))
			return L"(internal error)";
		// We can't return raw binary data, because we want to handle it with wxJS which
		// doesn't like \0 bytes in strings, so return it as hex
		static const char digits[] = "0123456789abcdef";
		std::string str = stream.str();
		std::wstring ret;
		ret.reserve(str.length()*3);
		for (size_t i = 0; i < str.length(); ++i)
		{
			ret += digits[(unsigned char)str[i] >> 4];
			ret += digits[(unsigned char)str[i] & 0x0f];
			ret += ' ';
		}
		return ret;
	}
	else
	{
		if (! g_Game->GetSimulation2()->DumpDebugState(stream))
			return L"(internal error)";
		return wstring_from_utf8(stream.str());
	}
}

//////////////////////////////////////////////////////////////////////////

ViewNone* view_None = NULL;
ViewGame* view_Game = NULL;
ViewActor* view_Actor = NULL;

View::~View()
{
}

View* View::GetView(int /*eRenderView*/ view)
{
	switch (view)
	{
	case AtlasMessage::eRenderView::NONE:  return View::GetView_None();
	case AtlasMessage::eRenderView::GAME:  return View::GetView_Game();
	case AtlasMessage::eRenderView::ACTOR: return View::GetView_Actor();
	default:
		debug_warn(L"Invalid view type");
		return View::GetView_None();
	}
}

View* View::GetView_None()
{
	if (! view_None)
		view_None = new ViewNone();
	return view_None;
}

ViewGame* View::GetView_Game()
{
	if (! view_Game)
		view_Game = new ViewGame();
	return view_Game;
}

ViewActor* View::GetView_Actor()
{
	if (! view_Actor)
		view_Actor = new ViewActor();
	return view_Actor;
}

void View::DestroyViews()
{
	delete view_None; view_None = NULL;
	delete view_Game; view_Game = NULL;
	delete view_Actor; view_Actor = NULL;
}
