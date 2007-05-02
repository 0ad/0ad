#include "precompiled.h"

#include "View.h"

#include "ActorViewer.h"
#include "GameLoop.h"
#include "Messages.h"

#include "graphics/CinemaTrack.h"
#include "graphics/GameView.h"
#include "graphics/Model.h"
#include "graphics/ObjectBase.h"
#include "graphics/ObjectEntry.h"
#include "graphics/SColor.h"
#include "graphics/Unit.h"
#include "graphics/UnitManager.h"
#include "lib/timer.h"
#include "maths/MathUtil.h"
#include "ps/Game.h"
#include "ps/GameSetup/GameSetup.h"
#include "ps/Player.h"
#include "renderer/Renderer.h"
#include "simulation/Entity.h"
#include "simulation/EntityManager.h"
#include "simulation/EntityTemplate.h"
#include "simulation/EntityTemplateCollection.h"
#include "simulation/Projectile.h"
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
	Atlas_GLSwapBuffers((void*)g_GameLoop->glCanvas);
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

struct SimState
{
	struct Entity
	{
		CStrW templateName;
		int unitID;
		std::set<CStr> selections;
		int playerID;
		CVector3D position;
		float angle;
	};

	struct Nonentity
	{
		CStrW actorName;
		int unitID;
		std::set<CStr> selections;
		CVector3D position;
		float angle;
	};

	bool onlyEntities;
	std::vector<Entity> entities;
	std::vector<Nonentity> nonentities;
};

template<typename T, typename S>
static void delete_pair_2nd(std::pair<T,S> v)
{
	delete v.second;
}

ViewGame::ViewGame()
: m_SpeedMultiplier(0.f)
{
	debug_assert(g_Game);
}

ViewGame::~ViewGame()
{
	std::for_each(m_SavedStates.begin(), m_SavedStates.end(), delete_pair_2nd<std::wstring, SimState*>);
}

void ViewGame::Update(float frameLength)
{
	float actualFrameLength = frameLength * m_SpeedMultiplier;

	if (m_SpeedMultiplier == 0.f)
	{
		// Update unit interpolation
		g_Game->GetSimulation()->Interpolate(0.0);
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
			double t = get_time();
			while (!ok && get_time() < t + 0.1) // don't go much worse than 10fps
			{
				ok = g_Game->Update(0.0, false); // don't add on any extra sim time
			}
		}

		// Interpolate the graphics - we only want to do this once per visual frame,
		// not in every call to g_Game->Update
		g_Game->GetSimulation()->Interpolate(actualFrameLength);

		// If we still couldn't keep up, just drop the sim rate instead of building
		// up a (potentially very large) backlog of required updates
		g_Game->GetSimulation()->DiscardMissedUpdates();
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
	camera.SetViewPort(&vp);
	camera.SetProjection(CGameView::defaultNear, CGameView::defaultFar, CGameView::defaultFOV);
	camera.UpdateFrustum();

	::Render();
	AtlasMessage::AtlasRenderSelection();
	Atlas_GLSwapBuffers((void*)g_GameLoop->glCanvas);
}

CCamera& ViewGame::GetCamera()
{
	return *g_Game->GetView()->GetCamera();
}

CUnit* ViewGame::GetUnit(AtlasMessage::ObjectID id)
{
	return g_Game->GetWorld()->GetUnitManager().FindByID(id);
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

void ViewGame::SaveState(const std::wstring& label, bool onlyEntities)
{
	SimState* simState = new SimState();
	simState->onlyEntities = onlyEntities;

	CUnitManager& unitMan = g_Game->GetWorld()->GetUnitManager();
	const std::vector<CUnit*>& units = unitMan.GetUnits();

	for (std::vector<CUnit*>::const_iterator unit = units.begin(); unit != units.end(); ++unit)
	{
		CEntity* entity = (*unit)->GetEntity();

		// Ignore objects that aren't entities
		if (! entity)
			continue;

		SimState::Entity e;
		e.templateName = entity->m_base->m_Tag;
		e.unitID = (*unit)->GetID();
		e.selections = (*unit)->GetActorSelections();
		e.playerID = entity->GetPlayer()->GetPlayerID();
		e.position = entity->m_position;
		e.angle = entity->m_orientation.Y;

		simState->entities.push_back(e);
	}

	if (! onlyEntities)
	{
		for (std::vector<CUnit*>::const_iterator unit = units.begin(); unit != units.end(); ++unit) {

			// Ignore objects that are entities
			if ((*unit)->GetEntity())
				continue;

			SimState::Nonentity n;
			n.actorName = (*unit)->GetObject()->m_Base->m_Name;
			n.unitID = (*unit)->GetID();
			n.selections = (*unit)->GetActorSelections();
			n.position = (*unit)->GetModel()->GetTransform().GetTranslation();
			CVector3D orient = (*unit)->GetModel()->GetTransform().GetIn();
			n.angle = atan2(-orient.X, -orient.Z);

			simState->nonentities.push_back(n);
		}
	}

	delete m_SavedStates[label]; // in case it already exists
	m_SavedStates[label] = simState;
}

void ViewGame::RestoreState(const std::wstring& label)
{
	SimState* simState = m_SavedStates[label];
	if (! simState)
		return;

	CUnitManager& unitMan = g_Game->GetWorld()->GetUnitManager();

	// delete all existing entities
	g_Game->GetWorld()->GetProjectileManager().DeleteAll();
	g_EntityManager.DeleteAll();

	if (! simState->onlyEntities)
	{
		// delete all remaining non-entity units
		unitMan.DeleteAll();
		// don't reset the unitID counter - there's no need, since it'll work alright anyway
	}

	for (size_t i = 0; i < simState->entities.size(); ++i)
	{
		SimState::Entity& e = simState->entities[i];

		CEntityTemplate* base = g_EntityTemplateCollection.GetTemplate(e.templateName, g_Game->GetPlayer(e.playerID));
		if (base)
		{
			HEntity ent = g_EntityManager.Create(base, e.position, e.angle, e.selections);

			if (ent)
			{
				ent->m_actor->SetPlayerID(e.playerID);
				ent->m_actor->SetID(e.unitID);
			}
		}
	}

	g_EntityManager.InitializeAll();

	if (! simState->onlyEntities)
	{
		for (size_t i = 0; i < simState->nonentities.size(); ++i)
		{
			SimState::Nonentity& n = simState->nonentities[i];

			CUnit* unit = unitMan.CreateUnit(n.actorName, NULL, n.selections);

			if (unit)
			{
				CMatrix3D m;
				m.SetYRotation(n.angle + PI);
				m.Translate(n.position);
				unit->GetModel()->SetTransform(m);

				unit->SetID(n.unitID);
			}
		}
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
