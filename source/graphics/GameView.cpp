/* Copyright (C) 2012 Wildfire Games.
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

#include "graphics/Camera.h"
#include "graphics/CinemaTrack.h"
#include "graphics/ColladaManager.h"
#include "graphics/HFTracer.h"
#include "graphics/LightEnv.h"
#include "graphics/LOSTexture.h"
#include "graphics/Model.h"
#include "graphics/ObjectManager.h"
#include "graphics/Patch.h"
#include "graphics/SkeletonAnimManager.h"
#include "graphics/Terrain.h"
#include "graphics/TerrainTextureManager.h"
#include "graphics/TerritoryTexture.h"
#include "graphics/Unit.h"
#include "graphics/UnitManager.h"
#include "graphics/scripting/JSInterface_GameView.h"
#include "lib/input.h"
#include "lib/timer.h"
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
#include "lobby/IXmppClient.h"

extern int g_xres, g_yres;

// Maximum distance outside the edge of the map that the camera's
// focus point can be moved
static const float CAMERA_EDGE_MARGIN = 2.0f*TERRAIN_TILE_SIZE;

/**
 * A value with exponential decay towards the target value.
 */
class CSmoothedValue
{
public:
	CSmoothedValue(float value, float smoothness, float minDelta)
		: m_Target(value), m_Current(value), m_Smoothness(smoothness), m_MinDelta(minDelta)
	{
	}

	float GetSmoothedValue()
	{
		return m_Current;
	}

	void SetValueSmoothly(float value)
	{
		m_Target = value;
	}

	void AddSmoothly(float value)
	{
		m_Target += value;
	}

	void Add(float value)
	{
		m_Target += value;
		m_Current += value;
	}

	float GetValue()
	{
		return m_Target;
	}

	void SetValue(float value)
	{
		m_Target = value;
		m_Current = value;
	}

	float Update(float time)
	{
		if (fabs(m_Target - m_Current) < m_MinDelta)
			return 0.0f;

		double p = pow((double)m_Smoothness, 10.0 * (double)time);
		// (add the factor of 10 so that smoothnesses don't have to be tiny numbers)

		double delta = (m_Target - m_Current) * (1.0 - p);
		m_Current += delta;
		return (float)delta;
	}

	void ClampSmoothly(float min, float max)
	{
		m_Target = Clamp(m_Target, (double)min, (double)max);
	}

	// Wrap so 'target' is in the range [min, max]
	void Wrap(float min, float max)
	{
		double t = fmod(m_Target - min, (double)(max - min));
		if (t < 0)
			t += max - min;
		t += min;

		m_Current += t - m_Target;
		m_Target = t;
	}

private:
	double m_Target; // the value which m_Current is tending towards
	double m_Current;
	// (We use double because the extra precision is worthwhile here)

	float m_MinDelta; // cutoff where we stop moving (to avoid ugly shimmering effects)
public:
	float m_Smoothness;
};

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
		ConstrainCamera(true),
		Culling(true),
		FollowEntity(INVALID_ENTITY),
		FollowFirstPerson(false),

		// Dummy values (these will be filled in by the config file)
		ViewScrollSpeed(0),
		ViewScrollSpeedModifier(1),
		ViewRotateXSpeed(0),
		ViewRotateXMin(0),
		ViewRotateXMax(0),
		ViewRotateXDefault(0),
		ViewRotateYSpeed(0),
		ViewRotateYSpeedWheel(0),
		ViewRotateYDefault(0),
		ViewRotateSpeedModifier(1),
		ViewDragSpeed(0),
		ViewZoomSpeed(0),
		ViewZoomSpeedWheel(0),
		ViewZoomMin(0),
		ViewZoomMax(0),
		ViewZoomDefault(0),
		ViewZoomSpeedModifier(1),
		ViewFOV(DEGTORAD(45.f)),
		ViewNear(2.f),
		ViewFar(4096.f),
		JoystickPanX(-1),
		JoystickPanY(-1),
		JoystickRotateX(-1),
		JoystickRotateY(-1),
		JoystickZoomIn(-1),
		JoystickZoomOut(-1),
		HeightSmoothness(0.5f),
		HeightMin(16.f),

		PosX(0, 0, 0.01f),
		PosY(0, 0, 0.01f),
		PosZ(0, 0, 0.01f),
		Zoom(0, 0, 0.1f),
		RotateX(0, 0, 0.001f),
		RotateY(0, 0, 0.001f)
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
	 * Whether the camera movement should be constrained by min/max limits
	 * and terrain avoidance.
	 */
	bool ConstrainCamera;

	/**
	 * Cache global lighting environment. This is used  to check whether the
	 * environment has changed during the last frame, so that vertex data can be updated etc.
	 */
	CLightEnv CachedLightEnv;

	CCinemaManager TrackManager;

	/**
	 * Entity for the camera to follow, or INVALID_ENTITY if none.
	 */
	entity_id_t FollowEntity;

	/**
	 * Whether to follow FollowEntity in first-person mode.
	 */
	bool FollowFirstPerson;

	////////////////////////////////////////
	// Settings
	float ViewScrollSpeed;
	float ViewScrollSpeedModifier;
	float ViewRotateXSpeed;
	float ViewRotateXMin;
	float ViewRotateXMax;
	float ViewRotateXDefault;
	float ViewRotateYSpeed;
	float ViewRotateYSpeedWheel;
	float ViewRotateYDefault;
	float ViewRotateSpeedModifier;
	float ViewDragSpeed;
	float ViewZoomSpeed;
	float ViewZoomSpeedWheel;
	float ViewZoomMin;
	float ViewZoomMax;
	float ViewZoomDefault;
	float ViewZoomSpeedModifier;
	float ViewFOV;
	float ViewNear;
	float ViewFar;
	int JoystickPanX;
	int JoystickPanY;
	int JoystickRotateX;
	int JoystickRotateY;
	int JoystickZoomIn;
	int JoystickZoomOut;
	float HeightSmoothness;
	float HeightMin;

	////////////////////////////////////////
	// Camera Controls State
	CSmoothedValue PosX;
	CSmoothedValue PosY;
	CSmoothedValue PosZ;
	CSmoothedValue Zoom;
	CSmoothedValue RotateX; // inclination around x axis (relative to camera)
	CSmoothedValue RotateY; // rotation around y (vertical) axis
};

#define IMPLEMENT_BOOLEAN_SETTING(NAME) \
bool CGameView::Get##NAME##Enabled() \
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
IMPLEMENT_BOOLEAN_SETTING(ConstrainCamera);

#undef IMPLEMENT_BOOLEAN_SETTING

static void SetupCameraMatrixSmooth(CGameViewImpl* m, CMatrix3D* orientation)
{
	orientation->SetIdentity();
	orientation->RotateX(m->RotateX.GetSmoothedValue());
	orientation->RotateY(m->RotateY.GetSmoothedValue());
	orientation->Translate(m->PosX.GetSmoothedValue(), m->PosY.GetSmoothedValue(), m->PosZ.GetSmoothedValue());
}

static void SetupCameraMatrixSmoothRot(CGameViewImpl* m, CMatrix3D* orientation)
{
	orientation->SetIdentity();
	orientation->RotateX(m->RotateX.GetSmoothedValue());
	orientation->RotateY(m->RotateY.GetSmoothedValue());
	orientation->Translate(m->PosX.GetValue(), m->PosY.GetValue(), m->PosZ.GetValue());
}

static void SetupCameraMatrixNonSmooth(CGameViewImpl* m, CMatrix3D* orientation)
{
	orientation->SetIdentity();
	orientation->RotateX(m->RotateX.GetValue());
	orientation->RotateY(m->RotateY.GetValue());
	orientation->Translate(m->PosX.GetValue(), m->PosY.GetValue(), m->PosZ.GetValue());
}

CGameView::CGameView(CGame *pGame):
	m(new CGameViewImpl(pGame))
{
	SViewPort vp;
	vp.m_X=0;
	vp.m_Y=0;
	vp.m_Width=g_xres;
	vp.m_Height=g_yres;
	m->ViewCamera.SetViewPort(vp);

	m->ViewCamera.SetProjection(m->ViewNear, m->ViewFar, m->ViewFOV);
	SetupCameraMatrixSmooth(m, &m->ViewCamera.m_Orientation);
	m->ViewCamera.UpdateFrustum();

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
	m->ViewCamera.SetViewPort(vp);
	m->ViewCamera.SetProjection(m->ViewNear, m->ViewFar, m->ViewFOV);
}

CObjectManager& CGameView::GetObjectManager() const
{
	return m->ObjectManager;
}

CCamera* CGameView::GetCamera()
{
	return &m->ViewCamera;
}

CCinemaManager* CGameView::GetCinema()
{
	return &m->TrackManager;
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
	CFG_GET_VAL("view.scroll.speed", Float, m->ViewScrollSpeed);
	CFG_GET_VAL("view.scroll.speed.modifier", Float, m->ViewScrollSpeedModifier);
	CFG_GET_VAL("view.rotate.x.speed", Float, m->ViewRotateXSpeed);
	CFG_GET_VAL("view.rotate.x.min", Float, m->ViewRotateXMin);
	CFG_GET_VAL("view.rotate.x.max", Float, m->ViewRotateXMax);
	CFG_GET_VAL("view.rotate.x.default", Float, m->ViewRotateXDefault);
	CFG_GET_VAL("view.rotate.y.speed", Float, m->ViewRotateYSpeed);
	CFG_GET_VAL("view.rotate.y.speed.wheel", Float, m->ViewRotateYSpeedWheel);
	CFG_GET_VAL("view.rotate.y.default", Float, m->ViewRotateYDefault);
	CFG_GET_VAL("view.rotate.speed.modifier", Float, m->ViewRotateSpeedModifier);
	CFG_GET_VAL("view.drag.speed", Float, m->ViewDragSpeed);
	CFG_GET_VAL("view.zoom.speed", Float, m->ViewZoomSpeed);
	CFG_GET_VAL("view.zoom.speed.wheel", Float, m->ViewZoomSpeedWheel);
	CFG_GET_VAL("view.zoom.min", Float, m->ViewZoomMin);
	CFG_GET_VAL("view.zoom.max", Float, m->ViewZoomMax);
	CFG_GET_VAL("view.zoom.default", Float, m->ViewZoomDefault);
	CFG_GET_VAL("view.zoom.speed.modifier", Float, m->ViewZoomSpeedModifier);

	CFG_GET_VAL("joystick.camera.pan.x", Int, m->JoystickPanX);
	CFG_GET_VAL("joystick.camera.pan.y", Int, m->JoystickPanY);
	CFG_GET_VAL("joystick.camera.rotate.x", Int, m->JoystickRotateX);
	CFG_GET_VAL("joystick.camera.rotate.y", Int, m->JoystickRotateY);
	CFG_GET_VAL("joystick.camera.zoom.in", Int, m->JoystickZoomIn);
	CFG_GET_VAL("joystick.camera.zoom.out", Int, m->JoystickZoomOut);

	CFG_GET_VAL("view.height.smoothness", Float, m->HeightSmoothness);
	CFG_GET_VAL("view.height.min", Float, m->HeightMin);

	CFG_GET_VAL("view.pos.smoothness", Float, m->PosX.m_Smoothness);
	CFG_GET_VAL("view.pos.smoothness", Float, m->PosY.m_Smoothness);
	CFG_GET_VAL("view.pos.smoothness", Float, m->PosZ.m_Smoothness);
	CFG_GET_VAL("view.zoom.smoothness", Float, m->Zoom.m_Smoothness);
	CFG_GET_VAL("view.rotate.x.smoothness", Float, m->RotateX.m_Smoothness);
	CFG_GET_VAL("view.rotate.y.smoothness", Float, m->RotateY.m_Smoothness);

	CFG_GET_VAL("view.near", Float, m->ViewNear);
	CFG_GET_VAL("view.far", Float, m->ViewFar);
	CFG_GET_VAL("view.fov", Float, m->ViewFOV);

	// Convert to radians
	m->RotateX.SetValue(DEGTORAD(m->ViewRotateXDefault));
	m->RotateY.SetValue(DEGTORAD(m->ViewRotateYDefault));
	m->ViewFOV = DEGTORAD(m->ViewFOV);

	return 0;
}



void CGameView::RegisterInit()
{
	// CGameView init
	RegMemFun(this, &CGameView::Initialize, L"CGameView init", 1);

	// previously done by CGameView::InitResources
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

	m->Game->CachePlayerColours();
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
	const ssize_t patchesPerSide = pTerrain->GetPatchesPerSide();

	// find out which patches will be drawn
	for (ssize_t j=0; j<patchesPerSide; j++) {
		for (ssize_t i=0; i<patchesPerSide; i++) {
			CPatch* patch=pTerrain->GetPatch(i,j);	// can't fail

			// If the patch is underwater, calculate a bounding box that also contains the water plane
			CBoundingBoxAligned bounds = patch->GetWorldBounds();
			float waterHeight = g_Renderer.GetWaterManager()->m_WaterHeight + 0.001f;
			if(bounds[1].Y < waterHeight) {
				bounds[1].Y = waterHeight;
			}

			if (!m->Culling || frustum.IsBoxVisible (CVector3D(0,0,0), bounds)) {
				c->Submit(patch);
			}
		}
	}
	}

	m->Game->GetSimulation2()->RenderSubmit(*c, frustum, m->Culling);
}


void CGameView::CheckLightEnv()
{
	if (m->CachedLightEnv == g_LightEnv)
		return;

	if (m->CachedLightEnv.GetLightingModel() != g_LightEnv.GetLightingModel())
		g_Renderer.MakeShadersDirty();

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

static void FocusHeight(CGameViewImpl* m, bool smooth)
{
	/*
		The camera pivot height is moved towards ground level.
		To prevent excessive zoom when looking over a cliff,
		the target ground level is the maximum of the ground level at the camera's near and pivot points.
		The ground levels are filtered to achieve smooth camera movement.
		The filter radius is proportional to the zoom level.
		The camera height is clamped to prevent map penetration.
	*/

	if (!m->ConstrainCamera)
		return;

	CCamera targetCam = m->ViewCamera;
	SetupCameraMatrixSmoothRot(m, &targetCam.m_Orientation);

	const CVector3D position = targetCam.m_Orientation.GetTranslation();
	const CVector3D forwards = targetCam.m_Orientation.GetIn();

	// horizontal view radius
	const float radius = sqrtf(forwards.X * forwards.X + forwards.Z * forwards.Z) * m->Zoom.GetSmoothedValue();
	const float near_radius = radius * m->HeightSmoothness;
	const float pivot_radius = radius * m->HeightSmoothness;

	const CVector3D nearPoint = position + forwards * m->ViewNear;
	const CVector3D pivotPoint = position + forwards * m->Zoom.GetSmoothedValue();

	const float ground = m->Game->GetWorld()->GetTerrain()->GetExactGroundLevel(nearPoint.X, nearPoint.Z);

	// filter ground levels for smooth camera movement
	const float filtered_near_ground = m->Game->GetWorld()->GetTerrain()->GetFilteredGroundLevel(nearPoint.X, nearPoint.Z, near_radius);
	const float filtered_pivot_ground = m->Game->GetWorld()->GetTerrain()->GetFilteredGroundLevel(pivotPoint.X, pivotPoint.Z, pivot_radius);

	// filtered maximum visible ground level in view
	const float filtered_ground = std::max(filtered_near_ground, filtered_pivot_ground);

	// target camera height above pivot point
	const float pivot_height = -forwards.Y * (m->Zoom.GetSmoothedValue() - m->ViewNear);
	// minimum camera height above filtered ground level
	const float min_height = (m->HeightMin + ground - filtered_ground);

	const float target_height = std::max(pivot_height, min_height);
	const float height = (nearPoint.Y - filtered_ground);
	const float diff = target_height - height;
	if (fabsf(diff) < 0.0001f)
		return;

	if (smooth)
	{
		m->PosY.AddSmoothly(diff);
	}
	else
	{
		m->PosY.Add(diff);
	}
}

CVector3D CGameView::GetSmoothPivot(CCamera& camera) const
{
	return camera.m_Orientation.GetTranslation() + camera.m_Orientation.GetIn() * m->Zoom.GetSmoothedValue();
}

void CGameView::Update(const float deltaRealTime)
{
	// If camera movement is being handled by the touch-input system,
	// then we should stop to avoid conflicting with it
	if (g_TouchInput.IsEnabled())
		return;

	if (!g_app_has_focus)
		return;

	if (m->TrackManager.IsActive() && m->TrackManager.IsPlaying())
	{
		if (! m->TrackManager.Update(deltaRealTime))
		{
//			ResetCamera();
		}
		return;
	}

	// Calculate mouse movement
	static int mouse_last_x = 0;
	static int mouse_last_y = 0;
	int mouse_dx = g_mouse_x - mouse_last_x;
	int mouse_dy = g_mouse_y - mouse_last_y;
	mouse_last_x = g_mouse_x;
	mouse_last_y = g_mouse_y;

	if (HotkeyIsPressed("camera.rotate.cw"))
		m->RotateY.AddSmoothly(m->ViewRotateYSpeed * deltaRealTime);
	if (HotkeyIsPressed("camera.rotate.ccw"))
		m->RotateY.AddSmoothly(-m->ViewRotateYSpeed * deltaRealTime);
	if (HotkeyIsPressed("camera.rotate.up"))
		m->RotateX.AddSmoothly(-m->ViewRotateXSpeed * deltaRealTime);
	if (HotkeyIsPressed("camera.rotate.down"))
		m->RotateX.AddSmoothly(m->ViewRotateXSpeed * deltaRealTime);

	float moveRightward = 0.f;
	float moveForward = 0.f;

	if (HotkeyIsPressed("camera.pan"))
	{
		moveRightward += m->ViewDragSpeed * mouse_dx;
		moveForward += m->ViewDragSpeed * -mouse_dy;
	}

	if (g_mouse_active)
	{
		if (g_mouse_x >= g_xres - 2 && g_mouse_x < g_xres)
			moveRightward += m->ViewScrollSpeed * deltaRealTime;
		else if (g_mouse_x <= 3 && g_mouse_x >= 0)
			moveRightward -= m->ViewScrollSpeed * deltaRealTime;

		if (g_mouse_y >= g_yres - 2 && g_mouse_y < g_yres)
			moveForward -= m->ViewScrollSpeed * deltaRealTime;
		else if (g_mouse_y <= 3 && g_mouse_y >= 0)
			moveForward += m->ViewScrollSpeed * deltaRealTime;
	}

	if (HotkeyIsPressed("camera.right"))
		moveRightward += m->ViewScrollSpeed * deltaRealTime;
	if (HotkeyIsPressed("camera.left"))
		moveRightward -= m->ViewScrollSpeed * deltaRealTime;
	if (HotkeyIsPressed("camera.up"))
		moveForward += m->ViewScrollSpeed * deltaRealTime;
	if (HotkeyIsPressed("camera.down"))
		moveForward -= m->ViewScrollSpeed * deltaRealTime;

	if (g_Joystick.IsEnabled())
	{
		// This could all be improved with extra speed and sensitivity settings
		// (maybe use pow to allow finer control?), and inversion settings

		moveRightward += g_Joystick.GetAxisValue(m->JoystickPanX) * m->ViewScrollSpeed * deltaRealTime;
		moveForward -= g_Joystick.GetAxisValue(m->JoystickPanY) * m->ViewScrollSpeed * deltaRealTime;

		m->RotateX.AddSmoothly(g_Joystick.GetAxisValue(m->JoystickRotateX) * m->ViewRotateXSpeed * deltaRealTime);
		m->RotateY.AddSmoothly(-g_Joystick.GetAxisValue(m->JoystickRotateY) * m->ViewRotateYSpeed * deltaRealTime);

		// Use a +1 bias for zoom because I want this to work with trigger buttons that default to -1
		m->Zoom.AddSmoothly((g_Joystick.GetAxisValue(m->JoystickZoomIn) + 1.0f) / 2.0f * m->ViewZoomSpeed * deltaRealTime);
		m->Zoom.AddSmoothly(-(g_Joystick.GetAxisValue(m->JoystickZoomOut) + 1.0f) / 2.0f * m->ViewZoomSpeed * deltaRealTime);
	}

	if (moveRightward || moveForward)
	{
		// Break out of following mode when the user starts scrolling
		m->FollowEntity = INVALID_ENTITY;

		float s = sin(m->RotateY.GetSmoothedValue());
		float c = cos(m->RotateY.GetSmoothedValue());
		m->PosX.AddSmoothly(c * moveRightward);
		m->PosZ.AddSmoothly(-s * moveRightward);
		m->PosX.AddSmoothly(s * moveForward);
		m->PosZ.AddSmoothly(c * moveForward);
	}

	if (m->FollowEntity)
	{
		CmpPtr<ICmpPosition> cmpPosition(*(m->Game->GetSimulation2()), m->FollowEntity);
		CmpPtr<ICmpRangeManager> cmpRangeManager(*(m->Game->GetSimulation2()), SYSTEM_ENTITY);
		if (cmpPosition && cmpPosition->IsInWorld() &&
		    cmpRangeManager && cmpRangeManager->GetLosVisibility(m->FollowEntity, m->Game->GetPlayerID(), false) == ICmpRangeManager::VIS_VISIBLE)
		{
			// Get the most recent interpolated position
			float frameOffset = m->Game->GetSimulation2()->GetLastFrameOffset();
			CMatrix3D transform = cmpPosition->GetInterpolatedTransform(frameOffset);
			CVector3D pos = transform.GetTranslation();

			if (m->FollowFirstPerson)
			{
				float x, z, angle;
				cmpPosition->GetInterpolatedPosition2D(frameOffset, x, z, angle);
				float height = 4.f;
				m->ViewCamera.m_Orientation.SetIdentity();
				m->ViewCamera.m_Orientation.RotateX((float)M_PI/24.f);
				m->ViewCamera.m_Orientation.RotateY(angle);
				m->ViewCamera.m_Orientation.Translate(pos.X, pos.Y + height, pos.Z);

				m->ViewCamera.UpdateFrustum();
				return;
			}
			else
			{
				// Move the camera to match the unit
				CCamera targetCam = m->ViewCamera;
				SetupCameraMatrixSmoothRot(m, &targetCam.m_Orientation);

				CVector3D pivot = GetSmoothPivot(targetCam);
				CVector3D delta = pos - pivot;
				m->PosX.AddSmoothly(delta.X);
				m->PosY.AddSmoothly(delta.Y);
				m->PosZ.AddSmoothly(delta.Z);
			}
		}
		else
		{
			// The unit disappeared (died or garrisoned etc), so stop following it
			m->FollowEntity = INVALID_ENTITY;
		}
	}

	if (HotkeyIsPressed("camera.zoom.in"))
		m->Zoom.AddSmoothly(-m->ViewZoomSpeed * deltaRealTime);
	if (HotkeyIsPressed("camera.zoom.out"))
		m->Zoom.AddSmoothly(m->ViewZoomSpeed * deltaRealTime);

	if (m->ConstrainCamera)
		m->Zoom.ClampSmoothly(m->ViewZoomMin, m->ViewZoomMax);

	float zoomDelta = -m->Zoom.Update(deltaRealTime);
	if (zoomDelta)
	{
		CVector3D forwards = m->ViewCamera.m_Orientation.GetIn();
		m->PosX.AddSmoothly(forwards.X * zoomDelta);
		m->PosY.AddSmoothly(forwards.Y * zoomDelta);
		m->PosZ.AddSmoothly(forwards.Z * zoomDelta);
	}

	if (m->ConstrainCamera)
		m->RotateX.ClampSmoothly(DEGTORAD(m->ViewRotateXMin), DEGTORAD(m->ViewRotateXMax));

	FocusHeight(m, true);

	// Ensure the ViewCamera focus is inside the map with the chosen margins
	// if not so - apply margins to the camera
	if (m->ConstrainCamera)
	{
		CCamera targetCam = m->ViewCamera;
		SetupCameraMatrixSmoothRot(m, &targetCam.m_Orientation);

		CTerrain* pTerrain = m->Game->GetWorld()->GetTerrain();

		CVector3D pivot = GetSmoothPivot(targetCam);
		CVector3D delta = targetCam.m_Orientation.GetTranslation() - pivot;

		CVector3D desiredPivot = pivot;

		CmpPtr<ICmpRangeManager> cmpRangeManager(*(m->Game->GetSimulation2()), SYSTEM_ENTITY);
		if (cmpRangeManager && cmpRangeManager->GetLosCircular())
		{
			// Clamp to a circular region around the center of the map
			float r = pTerrain->GetMaxX() / 2;
			CVector3D center(r, desiredPivot.Y, r);
			float dist = (desiredPivot - center).Length();
			if (dist > r - CAMERA_EDGE_MARGIN)
				desiredPivot = center + (desiredPivot - center).Normalized() * (r - CAMERA_EDGE_MARGIN);
		}
		else
		{
			// Clamp to the square edges of the map
			desiredPivot.X = Clamp(desiredPivot.X, pTerrain->GetMinX() + CAMERA_EDGE_MARGIN, pTerrain->GetMaxX() - CAMERA_EDGE_MARGIN);
			desiredPivot.Z = Clamp(desiredPivot.Z, pTerrain->GetMinZ() + CAMERA_EDGE_MARGIN, pTerrain->GetMaxZ() - CAMERA_EDGE_MARGIN);
		}

		// Update the position so that pivot is within the margin
		m->PosX.SetValueSmoothly(desiredPivot.X + delta.X);
		m->PosZ.SetValueSmoothly(desiredPivot.Z + delta.Z);
	}

	m->PosX.Update(deltaRealTime);
	m->PosY.Update(deltaRealTime);
	m->PosZ.Update(deltaRealTime);

	// Handle rotation around the Y (vertical) axis
	{
		CCamera targetCam = m->ViewCamera;
		SetupCameraMatrixSmooth(m, &targetCam.m_Orientation);

		float rotateYDelta = m->RotateY.Update(deltaRealTime);
		if (rotateYDelta)
		{
			// We've updated RotateY, and need to adjust Pos so that it's still
			// facing towards the original focus point (the terrain in the center
			// of the screen).

			CVector3D upwards(0.0f, 1.0f, 0.0f);

			CVector3D pivot = GetSmoothPivot(targetCam);
			CVector3D delta = targetCam.m_Orientation.GetTranslation() - pivot;

			CQuaternion q;
			q.FromAxisAngle(upwards, rotateYDelta);
			CVector3D d = q.Rotate(delta) - delta;

			m->PosX.Add(d.X);
			m->PosY.Add(d.Y);
			m->PosZ.Add(d.Z);
		}
	}

	// Handle rotation around the X (sideways, relative to camera) axis
	{
		CCamera targetCam = m->ViewCamera;
		SetupCameraMatrixSmooth(m, &targetCam.m_Orientation);

		float rotateXDelta = m->RotateX.Update(deltaRealTime);
		if (rotateXDelta)
		{
			CVector3D rightwards = targetCam.m_Orientation.GetLeft() * -1.0f;

			CVector3D pivot = GetSmoothPivot(targetCam);
			CVector3D delta = targetCam.m_Orientation.GetTranslation() - pivot;

			CQuaternion q;
			q.FromAxisAngle(rightwards, rotateXDelta);
			CVector3D d = q.Rotate(delta) - delta;

			m->PosX.Add(d.X);
			m->PosY.Add(d.Y);
			m->PosZ.Add(d.Z);
		}
	}

	/* This is disabled since it doesn't seem necessary:

	// Ensure the camera's near point is never inside the terrain
	if (m->ConstrainCamera)
	{
		CMatrix3D target;
		target.SetIdentity();
		target.RotateX(m->RotateX.GetValue());
		target.RotateY(m->RotateY.GetValue());
		target.Translate(m->PosX.GetValue(), m->PosY.GetValue(), m->PosZ.GetValue());

		CVector3D nearPoint = target.GetTranslation() + target.GetIn() * defaultNear;
		float ground = m->Game->GetWorld()->GetTerrain()->GetExactGroundLevel(nearPoint.X, nearPoint.Z);
		float limit = ground + 16.f;
		if (nearPoint.Y < limit)
			m->PosY.AddSmoothly(limit - nearPoint.Y);
	}
	*/

	m->RotateY.Wrap(-(float)M_PI, (float)M_PI);

	// Update the camera matrix
	m->ViewCamera.SetProjection(m->ViewNear, m->ViewFar, m->ViewFOV);
	SetupCameraMatrixSmooth(m, &m->ViewCamera.m_Orientation);
	m->ViewCamera.UpdateFrustum();
}

float CGameView::GetCameraX()
{
	CCamera targetCam = m->ViewCamera;
	CVector3D pivot = GetSmoothPivot(targetCam);
	return pivot.X;
}

float CGameView::GetCameraZ()
{
	CCamera targetCam = m->ViewCamera;
	CVector3D pivot = GetSmoothPivot(targetCam);
	return pivot.Z;
}

float CGameView::GetCameraPosX()
{
	return m->PosX.GetValue();
}

float CGameView::GetCameraPosY()
{
	return m->PosY.GetValue();
}

float CGameView::GetCameraPosZ()
{
	return m->PosZ.GetValue();
}

float CGameView::GetCameraRotX()
{
	return m->RotateX.GetValue();
}

float CGameView::GetCameraRotY()
{
	return m->RotateY.GetValue();
}

float CGameView::GetCameraZoom()
{
	return m->Zoom.GetValue();
}

void CGameView::SetCamera(CVector3D Pos, float RotX, float RotY, float zoom)
{
	m->PosX.SetValue(Pos.X);
	m->PosY.SetValue(Pos.Y);
	m->PosZ.SetValue(Pos.Z);
	m->RotateX.SetValue(RotX);
	m->RotateY.SetValue(RotY);
	m->Zoom.SetValue(zoom);

	FocusHeight(m, false);

	SetupCameraMatrixNonSmooth(m, &m->ViewCamera.m_Orientation);
	m->ViewCamera.UpdateFrustum();

	// Break out of following mode so the camera really moves to the target
	m->FollowEntity = INVALID_ENTITY;
}

void CGameView::MoveCameraTarget(const CVector3D& target)
{
	// Maintain the same orientation and level of zoom, if we can
	// (do this by working out the point the camera is looking at, saving
	//  the difference between that position and the camera point, and restoring
	//  that difference to our new target)

	CCamera targetCam = m->ViewCamera;
	SetupCameraMatrixNonSmooth(m, &targetCam.m_Orientation);

	CVector3D pivot = GetSmoothPivot(targetCam);
	CVector3D delta = target - pivot;

	m->PosX.SetValueSmoothly(delta.X + m->PosX.GetValue());
	m->PosZ.SetValueSmoothly(delta.Z + m->PosZ.GetValue());

	FocusHeight(m, false);

	// Break out of following mode so the camera really moves to the target
	m->FollowEntity = INVALID_ENTITY;
}

void CGameView::ResetCameraTarget(const CVector3D& target)
{
	CMatrix3D orientation;
	orientation.SetIdentity();
	orientation.RotateX(DEGTORAD(m->ViewRotateXDefault));
	orientation.RotateY(DEGTORAD(m->ViewRotateYDefault));

	CVector3D delta = orientation.GetIn() * m->ViewZoomDefault;
	m->PosX.SetValue(target.X - delta.X);
	m->PosY.SetValue(target.Y - delta.Y);
	m->PosZ.SetValue(target.Z - delta.Z);
	m->RotateX.SetValue(DEGTORAD(m->ViewRotateXDefault));
	m->RotateY.SetValue(DEGTORAD(m->ViewRotateYDefault));
	m->Zoom.SetValue(m->ViewZoomDefault);

	FocusHeight(m, false);

	SetupCameraMatrixSmooth(m, &m->ViewCamera.m_Orientation);
	m->ViewCamera.UpdateFrustum();

	// Break out of following mode so the camera really moves to the target
	m->FollowEntity = INVALID_ENTITY;
}

void CGameView::ResetCameraAngleZoom()
{
	CCamera targetCam = m->ViewCamera;
	SetupCameraMatrixNonSmooth(m, &targetCam.m_Orientation);

	// Compute the zoom adjustment to get us back to the default
	CVector3D forwards = targetCam.m_Orientation.GetIn();

	CVector3D pivot = GetSmoothPivot(targetCam);
	CVector3D delta = pivot - targetCam.m_Orientation.GetTranslation();
	float dist = delta.Dot(forwards);
	m->Zoom.AddSmoothly(m->ViewZoomDefault - dist);

	// Reset orientations to default
	m->RotateX.SetValueSmoothly(DEGTORAD(m->ViewRotateXDefault));
	m->RotateY.SetValueSmoothly(DEGTORAD(m->ViewRotateYDefault));
}

void CGameView::CameraFollow(entity_id_t entity, bool firstPerson)
{
	m->FollowEntity = entity;
	m->FollowFirstPerson = firstPerson;
}

entity_id_t CGameView::GetFollowedEntity()
{
	return m->FollowEntity;
}

float CGameView::GetNear() const
{
	return m->ViewNear;
}

float CGameView::GetFar() const
{
	return m->ViewFar;
}

float CGameView::GetFOV() const
{
	return m->ViewFOV;
}

void CGameView::SetCameraProjection()
{
	m->ViewCamera.SetProjection(m->ViewNear, m->ViewFar, m->ViewFOV);
}

InReaction game_view_handler(const SDL_Event_* ev)
{
	// put any events that must be processed even if inactive here
	if(!g_app_has_focus || !g_Game || !g_Game->IsGameStarted())
		return IN_PASS;

	CGameView *pView=g_Game->GetView();

	return pView->HandleEvent(ev);
}

InReaction CGameView::HandleEvent(const SDL_Event_* ev)
{
	switch(ev->ev.type)
	{

	case SDL_HOTKEYDOWN:
		std::string hotkey = static_cast<const char*>(ev->ev.user.data1);

		if (hotkey == "wireframe")
		{
			if (g_XmppClient && g_rankedGame == true)
				break;
			else if (g_Renderer.GetModelRenderMode() == SOLID)
			{
				g_Renderer.SetTerrainRenderMode(EDGED_FACES);
				g_Renderer.SetModelRenderMode(EDGED_FACES);
			}
			else if (g_Renderer.GetModelRenderMode() == EDGED_FACES)
			{
				g_Renderer.SetTerrainRenderMode(WIREFRAME);
				g_Renderer.SetModelRenderMode(WIREFRAME);
			}
			else
			{
				g_Renderer.SetTerrainRenderMode(SOLID);
				g_Renderer.SetModelRenderMode(SOLID);
			}
			return IN_HANDLED;
		}
		// Mouse wheel must be treated using events instead of polling,
		// because SDL auto-generates a sequence of mousedown/mouseup events
		// and we never get to see the "down" state inside Update().
		else if (hotkey == "camera.zoom.wheel.in")
		{
			m->Zoom.AddSmoothly(-m->ViewZoomSpeedWheel);
			return IN_HANDLED;
		}
		else if (hotkey == "camera.zoom.wheel.out")
		{
			m->Zoom.AddSmoothly(m->ViewZoomSpeedWheel);
			return IN_HANDLED;
		}
		else if (hotkey == "camera.rotate.wheel.cw")
		{
			m->RotateY.AddSmoothly(m->ViewRotateYSpeedWheel);
			return IN_HANDLED;
		}
		else if (hotkey == "camera.rotate.wheel.ccw")
		{
			m->RotateY.AddSmoothly(-m->ViewRotateYSpeedWheel);
			return IN_HANDLED;
		}
		else if (hotkey == "camera.scroll.speed.increase")
		{
			m->ViewScrollSpeed *= m->ViewScrollSpeedModifier;
			return IN_HANDLED;
		}
		else if (hotkey == "camera.scroll.speed.decrease")
		{
			m->ViewScrollSpeed /= m->ViewScrollSpeedModifier;
			return IN_HANDLED;
		}
		else if (hotkey == "camera.rotate.speed.increase")
		{
			m->ViewRotateXSpeed *= m->ViewRotateSpeedModifier;
			m->ViewRotateYSpeed *= m->ViewRotateSpeedModifier;
			return IN_HANDLED;
		}
		else if (hotkey == "camera.rotate.speed.decrease")
		{
			m->ViewRotateXSpeed /= m->ViewRotateSpeedModifier;
			m->ViewRotateYSpeed /= m->ViewRotateSpeedModifier;
			return IN_HANDLED;
		}
		else if (hotkey == "camera.zoom.speed.increase")
		{
			m->ViewZoomSpeed *= m->ViewZoomSpeedModifier;
			return IN_HANDLED;
		}
		else if (hotkey == "camera.zoom.speed.decrease")
		{
			m->ViewZoomSpeed /= m->ViewZoomSpeedModifier;
			return IN_HANDLED;
		}
		else if (hotkey == "camera.reset")
		{
			ResetCameraAngleZoom();
			return IN_HANDLED;
		}
	}

	return IN_PASS;
}
