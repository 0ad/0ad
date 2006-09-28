#include "precompiled.h"

#include "View.h"

#include "ActorViewer.h"
#include "GameLoop.h"
#include "Messages.h"

#include "graphics/SColor.h"
#include "graphics/UnitManager.h"
#include "renderer/Renderer.h"
#include "ps/Game.h"
#include "ps/GameSetup/GameSetup.h"
#include "simulation/EntityManager.h"
#include "simulation/Simulation.h"

extern void (*Atlas_GLSwapBuffers)(void* context);

extern int g_xres, g_yres;

//////////////////////////////////////////////////////////////////////////

void View::SetParam(const std::wstring& UNUSED(name), bool UNUSED(value))
{
}

void View::SetParam(const std::wstring& UNUSED(name), const AtlasMessage::Colour& UNUSED(value))
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
	camera.SetViewPort(&vp);
	camera.SetProjection(CGameView::defaultNear, CGameView::defaultFar, CGameView::defaultFOV);
	camera.UpdateFrustum();

	m_ActorViewer->Render();
	Atlas_GLSwapBuffers((void*)g_GameLoop->glContext);
}

CCamera& ViewActor::GetCamera()
{
	return m_Camera;
}

CUnit* ViewActor::GetUnit(AtlasMessage::ObjectID UNUSED(id))
{
	return m_ActorViewer->GetUnit();
}

bool ViewActor::WantsHighFramerate()
{
	if (m_SpeedMultiplier != 0.f && m_ActorViewer->HasAnimation())
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

namespace AtlasMessage
{
	extern void AtlasRenderSelection();
}

ViewGame::ViewGame()
{
	debug_assert(g_Game);
}

void ViewGame::Update(float frameLength)
{
	g_EntityManager.updateAll(0);
	g_Game->GetSimulation()->Update(0.0);

	if (g_Game->GetView()->GetCinema()->IsPlaying())
		g_Game->GetView()->GetCinema()->Update(frameLength);
}

void ViewGame::Render()
{
	SViewPort vp = { 0, 0, g_xres, g_yres };
	CCamera& camera = GetCamera();
	camera.SetViewPort(&vp);
	camera.SetProjection(CGameView::defaultNear, CGameView::defaultFar, CGameView::defaultFOV);
	camera.UpdateFrustum();

	::Render();
	AtlasMessage::AtlasRenderSelection();
	Atlas_GLSwapBuffers((void*)g_GameLoop->glContext);
}

CCamera& ViewGame::GetCamera()
{
	return *g_Game->GetView()->GetCamera();
}

CUnit* ViewGame::GetUnit(AtlasMessage::ObjectID id)
{
	return g_UnitMan.FindByID(id);
}

bool ViewGame::WantsHighFramerate()
{
	if (g_Game->GetView()->GetCinema()->IsPlaying())
		return true;

	return false;
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
	if      (view == AtlasMessage::eRenderView::NONE)  return View::GetView_None();
	else if (view == AtlasMessage::eRenderView::GAME)  return View::GetView_Game();
	else if (view == AtlasMessage::eRenderView::ACTOR) return View::GetView_Actor();
	else
	{
		debug_warn("Invalid view type");
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
