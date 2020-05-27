/* Copyright (C) 2020 Wildfire Games.
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

#include "GameView.h"

#include "graphics/CameraController.h"
#include "graphics/CinemaManager.h"
#include "graphics/ColladaManager.h"
#include "graphics/HFTracer.h"
#include "graphics/LOSTexture.h"
#include "graphics/LightEnv.h"
#include "graphics/Model.h"
#include "graphics/ObjectManager.h"
#include "graphics/Patch.h"
#include "graphics/SkeletonAnimManager.h"
#include "graphics/SmoothedValue.h"
#include "graphics/Terrain.h"
#include "graphics/TerrainTextureManager.h"
#include "graphics/TerritoryTexture.h"
#include "graphics/Unit.h"
#include "graphics/UnitManager.h"
#include "graphics/scripting/JSInterface_GameView.h"
#include "lib/input.h"
#include "lib/timer.h"
#include "lobby/IXmppClient.h"
#include "maths/BoundingBoxAligned.h"
#include "maths/MathUtil.h"
#include "maths/Matrix3D.h"
#include "maths/Quaternion.h"
#include "ps/ConfigDB.h"
#include "ps/Filesystem.h"
#include "ps/Game.h"
#include "ps/Globals.h"
#include "ps/Hotkey.h"
#include "ps/Joystick.h"
#include "ps/Loader.h"
#include "ps/LoaderThunks.h"
#include "ps/Profile.h"
#include "ps/Pyrogenesis.h"
#include "ps/TouchInput.h"
#include "ps/World.h"
#include "renderer/Renderer.h"
#include "renderer/WaterManager.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpPosition.h"
#include "simulation2/components/ICmpRangeManager.h"

#include <memory>

class CGameViewImpl
{
	NONCOPYABLE(CGameViewImpl);
public:
	CGameViewImpl(CGame* game)
		: Game(game),
		ColladaManager(g_VFS), MeshManager(ColladaManager), SkeletonAnimManager(ColladaManager),
		ObjectManager(MeshManager, SkeletonAnimManager, *game->GetSimulation2()),
		LOSTexture(*game->GetSimulation2()),
		TerritoryTexture(*game->GetSimulation2()),
		ViewCamera(),
		CullCamera(),
		LockCullCamera(false),
		Culling(true),
		CameraController(new CCameraController(ViewCamera))
	{
	}

	CGame* Game;
	CColladaManager ColladaManager;
	CMeshManager MeshManager;
	CSkeletonAnimManager SkeletonAnimManager;
	CObjectManager ObjectManager;
	CLOSTexture LOSTexture;
	CTerritoryTexture TerritoryTexture;

	/**
	 * this camera controls the eye position when rendering
	 */
	CCamera ViewCamera;

	/**
	 * this camera controls the frustum that is used for culling
	 * and shadow calculations
	 *
	 * Note that all code that works with camera movements should only change
	 * m_ViewCamera. The render functions automatically sync the cull camera to
	 * the view camera depending on the value of m_LockCullCamera.
	 */
	CCamera CullCamera;

	/**
	 * When @c true, the cull camera is locked in place.
	 * When @c false, the cull camera follows the view camera.
	 *
	 * Exposed to JS as gameView.lockCullCamera
	 */
	bool LockCullCamera;

	/**
	 * When @c true, culling is enabled so that only models that have a chance of
	 * being visible are sent to the renderer.
	 * Otherwise, the entire world is sent to the renderer.
	 *
	 * Exposed to JS as gameView.culling
	 */
	bool Culling;

	/**
	 * Cache global lighting environment. This is used  to check whether the
	 * environment has changed during the last frame, so that vertex data can be updated etc.
	 */
	CLightEnv CachedLightEnv;

	CCinemaManager CinemaManager;

	/**
	 * Controller of the view's camera. We use a std::unique_ptr for an easy
	 * on the fly replacement. It's guaranteed that the pointer is never nulllptr.
	 */
	std::unique_ptr<ICameraController> CameraController;
};

#define IMPLEMENT_BOOLEAN_SETTING(NAME) \
bool CGameView::Get##NAME##Enabled() const \
{ \
	return m->NAME; \
} \
\
void CGameView::Set##NAME##Enabled(bool Enabled) \
{ \
	m->NAME = Enabled; \
}

IMPLEMENT_BOOLEAN_SETTING(Culling);
IMPLEMENT_BOOLEAN_SETTING(LockCullCamera);

bool CGameView::GetConstrainCameraEnabled() const
{
	return m->CameraController->GetConstrainCamera();
}

void CGameView::SetConstrainCameraEnabled(bool enabled)
{
	m->CameraController->SetConstrainCamera(enabled);
}

#undef IMPLEMENT_BOOLEAN_SETTING

CGameView::CGameView(CGame *pGame):
	m(new CGameViewImpl(pGame))
{
	m->CullCamera = m->ViewCamera;
	g_Renderer.SetSceneCamera(m->ViewCamera, m->CullCamera);
}

CGameView::~CGameView()
{
	UnloadResources();

	delete m;
}

void CGameView::SetViewport(const SViewPort& vp)
{
	m->CameraController->SetViewport(vp);
}

CObjectManager& CGameView::GetObjectManager()
{
	return m->ObjectManager;
}

CCamera* CGameView::GetCamera()
{
	return &m->ViewCamera;
}

CCinemaManager* CGameView::GetCinema()
{
	return &m->CinemaManager;
};

CLOSTexture& CGameView::GetLOSTexture()
{
	return m->LOSTexture;
}

CTerritoryTexture& CGameView::GetTerritoryTexture()
{
	return m->TerritoryTexture;
}

int CGameView::Initialize()
{
	m->CameraController->LoadConfig();
	return 0;
}

void CGameView::RegisterInit()
{
	// CGameView init
	RegMemFun(this, &CGameView::Initialize, L"CGameView init", 1);

	RegMemFun(g_TexMan.GetSingletonPtr(), &CTerrainTextureManager::LoadTerrainTextures, L"LoadTerrainTextures", 60);
	RegMemFun(g_Renderer.GetSingletonPtr(), &CRenderer::LoadAlphaMaps, L"LoadAlphaMaps", 5);
}

void CGameView::BeginFrame()
{
	if (m->LockCullCamera == false)
	{
		// Set up cull camera
		m->CullCamera = m->ViewCamera;
	}
	g_Renderer.SetSceneCamera(m->ViewCamera, m->CullCamera);

	CheckLightEnv();

	m->Game->CachePlayerColors();
}

void CGameView::Render()
{
	g_Renderer.RenderScene(*this);
}

///////////////////////////////////////////////////////////
// This callback is part of the Scene interface
// Submit all objects visible in the given frustum
void CGameView::EnumerateObjects(const CFrustum& frustum, SceneCollector* c)
{
	{
	PROFILE3("submit terrain");

	CTerrain* pTerrain = m->Game->GetWorld()->GetTerrain();
	float waterHeight = g_Renderer.GetWaterManager()->m_WaterHeight + 0.001f;
	const ssize_t patchesPerSide = pTerrain->GetPatchesPerSide();

	// find out which patches will be drawn
	for (ssize_t j=0; j<patchesPerSide; ++j)
	{
		for (ssize_t i=0; i<patchesPerSide; ++i)
		{
			CPatch* patch=pTerrain->GetPatch(i,j);	// can't fail

			// If the patch is underwater, calculate a bounding box that also contains the water plane
			CBoundingBoxAligned bounds = patch->GetWorldBounds();
			if(bounds[1].Y < waterHeight)
				bounds[1].Y = waterHeight;

			if (!m->Culling || frustum.IsBoxVisible(bounds))
				c->Submit(patch);
		}
	}
	}

	m->Game->GetSimulation2()->RenderSubmit(*c, frustum, m->Culling);
}


void CGameView::CheckLightEnv()
{
	if (m->CachedLightEnv == g_LightEnv)
		return;

	m->CachedLightEnv = g_LightEnv;
	CTerrain* pTerrain = m->Game->GetWorld()->GetTerrain();

	if (!pTerrain)
		return;

	PROFILE("update light env");
	pTerrain->MakeDirty(RENDERDATA_UPDATE_COLOR);

	const std::vector<CUnit*>& units = m->Game->GetWorld()->GetUnitManager().GetUnits();
	for (size_t i = 0; i < units.size(); ++i)
		units[i]->GetModel().SetDirtyRec(RENDERDATA_UPDATE_COLOR);
}


void CGameView::UnloadResources()
{
	g_TexMan.UnloadTerrainTextures();
	g_Renderer.UnloadAlphaMaps();
	g_Renderer.GetWaterManager()->UnloadWaterTextures();
}

void CGameView::Update(const float deltaRealTime)
{
	// If camera movement is being handled by the touch-input system,
	// then we should stop to avoid conflicting with it
	if (g_TouchInput.IsEnabled())
		return;

	if (!g_app_has_focus)
		return;

	m->CinemaManager.Update(deltaRealTime);
	if (m->CinemaManager.IsEnabled())
		return;

	m->CameraController->Update(deltaRealTime);
}

CVector3D CGameView::GetCameraPivot() const
{
	return m->CameraController->GetCameraPivot();
}

CVector3D CGameView::GetCameraPosition() const
{
	return m->CameraController->GetCameraPosition();
}

CVector3D CGameView::GetCameraRotation() const
{
	return m->CameraController->GetCameraRotation();
}

float CGameView::GetCameraZoom() const
{
	return m->CameraController->GetCameraZoom();
}

void CGameView::SetCamera(const CVector3D& pos, float rotX, float rotY, float zoom)
{
	m->CameraController->SetCamera(pos, rotX, rotY, zoom);
}

void CGameView::MoveCameraTarget(const CVector3D& target)
{
	m->CameraController->MoveCameraTarget(target);
}

void CGameView::ResetCameraTarget(const CVector3D& target)
{
	m->CameraController->ResetCameraTarget(target);
}

void CGameView::FollowEntity(entity_id_t entity, bool firstPerson)
{
	m->CameraController->FollowEntity(entity, firstPerson);
}

entity_id_t CGameView::GetFollowedEntity()
{
	return m->CameraController->GetFollowedEntity();
}

InReaction game_view_handler(const SDL_Event_* ev)
{
	// put any events that must be processed even if inactive here
	if (!g_app_has_focus || !g_Game || !g_Game->IsGameStarted() || g_Game->GetView()->GetCinema()->IsEnabled())
		return IN_PASS;

	CGameView *pView=g_Game->GetView();

	return pView->HandleEvent(ev);
}

InReaction CGameView::HandleEvent(const SDL_Event_* ev)
{
	switch(ev->ev.type)
	{
	case SDL_HOTKEYPRESS:
	{
		std::string hotkey = static_cast<const char*>(ev->ev.user.data1);
		if (hotkey == "wireframe")
		{
			if (g_XmppClient && g_rankedGame == true)
				break;
			else if (g_Renderer.GetModelRenderMode() == SOLID)
			{
				g_Renderer.SetTerrainRenderMode(EDGED_FACES);
				g_Renderer.SetWaterRenderMode(EDGED_FACES);
				g_Renderer.SetModelRenderMode(EDGED_FACES);
			}
			else if (g_Renderer.GetModelRenderMode() == EDGED_FACES)
			{
				g_Renderer.SetTerrainRenderMode(WIREFRAME);
				g_Renderer.SetWaterRenderMode(WIREFRAME);
				g_Renderer.SetModelRenderMode(WIREFRAME);
			}
			else
			{
				g_Renderer.SetTerrainRenderMode(SOLID);
				g_Renderer.SetWaterRenderMode(SOLID);
				g_Renderer.SetModelRenderMode(SOLID);
			}
			return IN_HANDLED;
		}
	}
	}

	return m->CameraController->HandleEvent(ev);
}
