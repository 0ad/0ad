#include "precompiled.h"

#include "View.h"

#include "ActorViewer.h"
#include "GameLoop.h"

#include "ps/Game.h"
#include "ps/GameSetup/GameSetup.h"
#include "simulation/EntityManager.h"
#include "simulation/Simulation.h"

extern void (*Atlas_GLSwapBuffers)(void* context);

extern int g_xres, g_yres;

//////////////////////////////////////////////////////////////////////////

class ViewNone : public View
{
public:
	virtual void Update(float) { }
	virtual void Render() { }
	virtual CCamera& GetCamera() { return dummyCamera; }
	virtual bool WantsHighFramerate() { return false; }
private:
	CCamera dummyCamera;
};

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
