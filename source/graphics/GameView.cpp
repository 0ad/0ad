/* Copyright (C) 2010 Wildfire Games.
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
#include "graphics/Model.h"
#include "graphics/ObjectManager.h"
#include "graphics/Patch.h"
#include "graphics/SkeletonAnimManager.h"
#include "graphics/Terrain.h"
#include "graphics/TextureManager.h"
#include "graphics/Unit.h"
#include "graphics/UnitManager.h"
#include "lib/input.h"
#include "lib/timer.h"
#include "maths/Bound.h"
#include "maths/MathUtil.h"
#include "maths/Matrix3D.h"
#include "maths/Quaternion.h"
#include "ps/ConfigDB.h"
#include "ps/Game.h"
#include "ps/Globals.h"
#include "ps/Hotkey.h"
#include "ps/Loader.h"
#include "ps/LoaderThunks.h"
#include "ps/Profile.h"
#include "ps/Pyrogenesis.h"
#include "ps/World.h"
#include "renderer/Renderer.h"
#include "renderer/SkyManager.h"
#include "renderer/WaterManager.h"
#include "scripting/ScriptableObject.h"
#include "simulation/LOSManager.h"
#include "simulation2/Simulation2.h"

extern int g_xres, g_yres;

const float CGameView::defaultFOV = DEGTORAD(20.f);
const float CGameView::defaultNear = 4.f;
const float CGameView::defaultFar = 4096.f;
const float CGameView::defaultCullFOV = CGameView::defaultFOV + DEGTORAD(6.0f);	//add 6 degrees to the default FOV for use with the culling frustum

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

private:
	double m_Target; // the value which m_Current is tending towards
	double m_Current;
	// (We use double because the extra precision is worthwhile here)

	float m_MinDelta; // cutoff where we stop moving (to avoid ugly shimmering effects)
public:
	float m_Smoothness;
};

class CGameViewImpl : public CJSObject<CGameViewImpl>
{
	NONCOPYABLE(CGameViewImpl);
public:
	CGameViewImpl(CGame* game)
		: Game(game),
		ColladaManager(), MeshManager(ColladaManager), SkeletonAnimManager(ColladaManager),
		ObjectManager(MeshManager, SkeletonAnimManager, *game->GetSimulation2()),
		ViewCamera(),
		CullCamera(),
		LockCullCamera(false),
		ConstrainCamera(true),
		Culling(true),

		// Dummy values (these will be filled in by the config file)
		ViewScrollSpeed(0),
		ViewRotateXSpeed(0),
		ViewRotateXMin(0),
		ViewRotateXMax(0),
		ViewRotateXDefault(0),
		ViewRotateYSpeed(0),
		ViewRotateYSpeedWheel(0),
		ViewRotateYDefault(0),
		ViewDragSpeed(0),
		ViewZoomSpeed(0),
		ViewZoomSpeedWheel(0),
		ViewZoomMin(0),
		ViewZoomMax(0),
		ViewZoomDefault(0),

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

	////////////////////////////////////////
	// Settings
	float ViewScrollSpeed;
	float ViewRotateXSpeed;
	float ViewRotateXMin;
	float ViewRotateXMax;
	float ViewRotateXDefault;
	float ViewRotateYSpeed;
	float ViewRotateYSpeedWheel;
	float ViewRotateYDefault;
	float ViewDragSpeed;
	float ViewZoomSpeed;
	float ViewZoomSpeedWheel;
	float ViewZoomMin;
	float ViewZoomMax;
	float ViewZoomDefault;

	////////////////////////////////////////
	// Camera Controls State
	CSmoothedValue PosX;
	CSmoothedValue PosY;
	CSmoothedValue PosZ;
	CSmoothedValue Zoom;
	CSmoothedValue RotateX; // inclination around x axis (relative to camera)
	CSmoothedValue RotateY; // rotation around y (vertical) axis

	static void ScriptingInit();
};

static void SetupCameraMatrix(CGameViewImpl* m, CMatrix3D* orientation)
{
	orientation->SetIdentity();
	orientation->RotateX(m->RotateX.GetSmoothedValue());
	orientation->RotateY(m->RotateY.GetSmoothedValue());
	orientation->Translate(m->PosX.GetSmoothedValue(), m->PosY.GetSmoothedValue(), m->PosZ.GetSmoothedValue());
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

	m->ViewCamera.SetProjection(defaultNear, defaultFar, defaultFOV);
	SetupCameraMatrix(m, &m->ViewCamera.m_Orientation);
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
	m->ViewCamera.SetProjection(defaultNear, defaultFar, defaultFOV);
}

CObjectManager& CGameView::GetObjectManager() const
{
	return m->ObjectManager;
}

JSObject* CGameView::GetScript()
{
	return m->GetScript();
}

/*static*/ void CGameView::ScriptingInit()
{
	return CGameViewImpl::ScriptingInit();
}

CCamera* CGameView::GetCamera()
{
	return &m->ViewCamera;
}

CCinemaManager* CGameView::GetCinema()
{
	return &m->TrackManager;
};

/*
void CGameView::AttachToUnit(CEntity* target)
{
	m->UnitAttach = target;
}
*/


void CGameViewImpl::ScriptingInit()
{
	AddProperty(L"culling", &CGameViewImpl::Culling);
	AddProperty(L"lockCullCamera", &CGameViewImpl::LockCullCamera);
	AddProperty(L"constrainCamera", &CGameViewImpl::ConstrainCamera);

	CJSObject<CGameViewImpl>::ScriptingInit("GameView");
}

int CGameView::Initialize()
{
	CFG_GET_SYS_VAL("view.scroll.speed", Float, m->ViewScrollSpeed);
	CFG_GET_SYS_VAL("view.rotate.x.speed", Float, m->ViewRotateXSpeed);
	CFG_GET_SYS_VAL("view.rotate.x.min", Float, m->ViewRotateXMin);
	CFG_GET_SYS_VAL("view.rotate.x.max", Float, m->ViewRotateXMax);
	CFG_GET_SYS_VAL("view.rotate.x.default", Float, m->ViewRotateXDefault);
	CFG_GET_SYS_VAL("view.rotate.y.speed", Float, m->ViewRotateYSpeed);
	CFG_GET_SYS_VAL("view.rotate.y.speed.wheel", Float, m->ViewRotateYSpeedWheel);
	CFG_GET_SYS_VAL("view.rotate.y.default", Float, m->ViewRotateYDefault);
	CFG_GET_SYS_VAL("view.drag.speed", Float, m->ViewDragSpeed);
	CFG_GET_SYS_VAL("view.zoom.speed", Float, m->ViewZoomSpeed);
	CFG_GET_SYS_VAL("view.zoom.speed.wheel", Float, m->ViewZoomSpeedWheel);
	CFG_GET_SYS_VAL("view.zoom.min", Float, m->ViewZoomMin);
	CFG_GET_SYS_VAL("view.zoom.max", Float, m->ViewZoomMax);
	CFG_GET_SYS_VAL("view.zoom.default", Float, m->ViewZoomDefault);

	CFG_GET_SYS_VAL("view.pos.smoothness", Float, m->PosX.m_Smoothness);
	CFG_GET_SYS_VAL("view.pos.smoothness", Float, m->PosY.m_Smoothness);
	CFG_GET_SYS_VAL("view.pos.smoothness", Float, m->PosZ.m_Smoothness);
	CFG_GET_SYS_VAL("view.zoom.smoothness", Float, m->Zoom.m_Smoothness);
	CFG_GET_SYS_VAL("view.rotate.x.smoothness", Float, m->RotateX.m_Smoothness);
	CFG_GET_SYS_VAL("view.rotate.y.smoothness", Float, m->RotateY.m_Smoothness);

	m->RotateX.SetValue(DEGTORAD(m->ViewRotateXDefault));
	m->RotateY.SetValue(DEGTORAD(m->ViewRotateYDefault));

	return 0;
}



void CGameView::RegisterInit()
{
	// CGameView init
	RegMemFun(this, &CGameView::Initialize, L"CGameView init", 1);

	// previously done by CGameView::InitResources
	RegMemFun(g_TexMan.GetSingletonPtr(), &CTextureManager::LoadTerrainTextures, L"LoadTerrainTextures", 60);
	RegMemFun(g_Renderer.GetSingletonPtr(), &CRenderer::LoadAlphaMaps, L"LoadAlphaMaps", 5);
	RegMemFun(g_Renderer.GetSingletonPtr()->GetWaterManager(), &WaterManager::LoadWaterTextures, L"LoadWaterTextures", 80);
	RegMemFun(g_Renderer.GetSingletonPtr()->GetSkyManager(), &SkyManager::LoadSkyTextures, L"LoadSkyTextures", 15);
}


void CGameView::Render()
{
	if (m->LockCullCamera == false)
	{
		// Set up cull camera
		m->CullCamera = m->ViewCamera;

		// One way to fix shadows popping in at the edge of the screen is to widen the culling frustum so that
		// objects aren't culled as early. The downside is that objects will get rendered even though they appear
		// off screen, which is somewhat inefficient. A better solution would be to decouple shadow map rendering
		// from model rendering; as it is now, a shadow map is only rendered if its associated model is to be
		// rendered.
		// (See http://trac.wildfiregames.com/ticket/504)
		m->CullCamera.SetProjection(defaultNear, defaultFar, defaultCullFOV);
		m->CullCamera.UpdateFrustum();
	}
	g_Renderer.SetSceneCamera(m->ViewCamera, m->CullCamera);

	CheckLightEnv();

	g_Renderer.RenderScene(this);
}

///////////////////////////////////////////////////////////
// This callback is part of the Scene interface
// Submit all objects visible in the given frustum
void CGameView::EnumerateObjects(const CFrustum& frustum, SceneCollector* c)
{
	PROFILE_START( "submit terrain" );
	CTerrain* pTerrain = m->Game->GetWorld()->GetTerrain();
	const ssize_t patchesPerSide = pTerrain->GetPatchesPerSide();

	// find out which patches will be drawn
	for (ssize_t j=0; j<patchesPerSide; j++) {
		for (ssize_t i=0; i<patchesPerSide; i++) {
			CPatch* patch=pTerrain->GetPatch(i,j);	// can't fail

			// If the patch is underwater, calculate a bounding box that also contains the water plane
			CBound bounds = patch->GetBounds();
			float waterHeight = g_Renderer.GetWaterManager()->m_WaterHeight + 0.001f;
			if(bounds[1].Y < waterHeight) {
				bounds[1].Y = waterHeight;
			}
			
			if (!m->Culling || frustum.IsBoxVisible (CVector3D(0,0,0), bounds)) {
				//c->Submit(patch);

				// set the renderstate for this patch
				patch->setDrawState(true);

				// set the renderstate for the neighbors
				CPatch *nPatch;

				nPatch = pTerrain->GetPatch(i-1,j-1);
				if(nPatch) nPatch->setDrawState(true);

				nPatch = pTerrain->GetPatch(i,j-1);
				if(nPatch) nPatch->setDrawState(true);

				nPatch = pTerrain->GetPatch(i+1,j-1);
				if(nPatch) nPatch->setDrawState(true);

				nPatch = pTerrain->GetPatch(i-1,j);
				if(nPatch) nPatch->setDrawState(true);

				nPatch = pTerrain->GetPatch(i+1,j);
				if(nPatch) nPatch->setDrawState(true);

				nPatch = pTerrain->GetPatch(i-1,j+1);
				if(nPatch) nPatch->setDrawState(true);

				nPatch = pTerrain->GetPatch(i,j+1);
				if(nPatch) nPatch->setDrawState(true);

				nPatch = pTerrain->GetPatch(i+1,j+1);
				if(nPatch) nPatch->setDrawState(true);
			}
		}
	}

	// draw the patches
	for (ssize_t j=0; j<patchesPerSide; j++)
	{
		for (ssize_t i=0; i<patchesPerSide; i++)
		{
			CPatch* patch=pTerrain->GetPatch(i,j);	// can't fail
			if(patch->getDrawState() == true)
			{
				c->Submit(patch);
				patch->setDrawState(false);
			}
		}
	}
	PROFILE_END( "submit terrain" );

	PROFILE_START( "submit sim components" );
	m->Game->GetSimulation2()->RenderSubmit(*c, frustum, m->Culling);
	PROFILE_END( "submit sim components" );
}


static void MarkUpdateColorRecursive(CModel& model)
{
	model.SetDirty(RENDERDATA_UPDATE_COLOR);

	const std::vector<CModel::Prop>& props = model.GetProps();
	for(size_t i = 0; i < props.size(); ++i) {
		debug_assert(props[i].m_Model);
		MarkUpdateColorRecursive(*props[i].m_Model);
	}
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
	for(size_t i = 0; i < units.size(); ++i) {
		MarkUpdateColorRecursive(units[i]->GetModel());
	}
}


void CGameView::UnloadResources()
{
	g_TexMan.UnloadTerrainTextures();
	g_Renderer.UnloadAlphaMaps();
	g_Renderer.GetWaterManager()->UnloadWaterTextures();
}


static void ClampDistance(CGameViewImpl* m, bool smooth)
{
	if (!m->ConstrainCamera)
		return;

	CCamera targetCam = m->ViewCamera;
	targetCam.m_Orientation.SetIdentity();
	targetCam.m_Orientation.RotateX(m->RotateX.GetSmoothedValue());
	targetCam.m_Orientation.RotateY(m->RotateY.GetSmoothedValue());
	// Use non-smoothed position:
	targetCam.m_Orientation.Translate(m->PosX.GetValue(), m->PosY.GetValue(), m->PosZ.GetValue());

	CVector3D forwards = targetCam.m_Orientation.GetIn();

	CVector3D delta = targetCam.GetFocus() - targetCam.m_Orientation.GetTranslation();

	float dist = delta.Dot(forwards);
	float clampedDist = Clamp(dist, m->ViewZoomMin, m->ViewZoomMax);
	float diff = clampedDist - dist;

	if (!diff)
		return;

	if (smooth)
	{
		m->PosX.AddSmoothly(forwards.X * -diff);
		m->PosY.AddSmoothly(forwards.Y * -diff);
		m->PosZ.AddSmoothly(forwards.Z * -diff);
	}
	else
	{
		m->PosX.Add(forwards.X * -diff);
		m->PosY.Add(forwards.Y * -diff);
		m->PosZ.Add(forwards.Z * -diff);
	}
}

void CGameView::Update(float DeltaTime)
{
	if (!g_app_has_focus)
		return;

	// TODO: this is probably not an ideal place for this, it should probably go
	// in a CCmpWaterManager or some such thing (once such a thing exists)
	g_Renderer.GetWaterManager()->m_WaterTexTimer += DeltaTime;

	if (m->TrackManager.IsActive() && m->TrackManager.IsPlaying())
	{
		if (! m->TrackManager.Update(DeltaTime))
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

	if (hotkeys[HOTKEY_CAMERA_ROTATE_CW])
		m->RotateY.AddSmoothly(m->ViewRotateYSpeed * DeltaTime);
	if (hotkeys[HOTKEY_CAMERA_ROTATE_CCW])
		m->RotateY.AddSmoothly(-m->ViewRotateYSpeed * DeltaTime);
	if (hotkeys[HOTKEY_CAMERA_ROTATE_UP])
		m->RotateX.AddSmoothly(-m->ViewRotateXSpeed * DeltaTime);
	if (hotkeys[HOTKEY_CAMERA_ROTATE_DOWN])
		m->RotateX.AddSmoothly(m->ViewRotateXSpeed * DeltaTime);

	float moveRightward = 0.f;
	float moveForward = 0.f;

	if (hotkeys[HOTKEY_CAMERA_PAN])
	{
		moveRightward += m->ViewDragSpeed * mouse_dx;
		moveForward += m->ViewDragSpeed * -mouse_dy;
	}

	if (g_mouse_active)
	{
		if (g_mouse_x >= g_xres - 2 && g_mouse_x < g_xres)
			moveRightward += m->ViewScrollSpeed * DeltaTime;
		else if (g_mouse_x <= 3 && g_mouse_x >= 0)
			moveRightward -= m->ViewScrollSpeed * DeltaTime;

		if (g_mouse_y >= g_yres - 2 && g_mouse_y < g_yres)
			moveForward -= m->ViewScrollSpeed * DeltaTime;
		else if (g_mouse_y <= 3 && g_mouse_y >= 0)
			moveForward += m->ViewScrollSpeed * DeltaTime;
	}

	if (hotkeys[HOTKEY_CAMERA_PAN_KEYBOARD])
	{
		if (hotkeys[HOTKEY_CAMERA_RIGHT])
			moveRightward += m->ViewScrollSpeed * DeltaTime;
		if (hotkeys[HOTKEY_CAMERA_LEFT])
			moveRightward -= m->ViewScrollSpeed * DeltaTime;
		if (hotkeys[HOTKEY_CAMERA_UP])
			moveForward += m->ViewScrollSpeed * DeltaTime;
		if (hotkeys[HOTKEY_CAMERA_DOWN])
			moveForward -= m->ViewScrollSpeed * DeltaTime;
	}

	if (moveRightward || moveForward)
	{
		float s = sin(m->RotateY.GetSmoothedValue());
		float c = cos(m->RotateY.GetSmoothedValue());
		m->PosX.AddSmoothly(c * moveRightward);
		m->PosZ.AddSmoothly(-s * moveRightward);
		m->PosX.AddSmoothly(s * moveForward);
		m->PosZ.AddSmoothly(c * moveForward);
	}

	if (hotkeys[HOTKEY_CAMERA_ZOOM_IN])
		m->Zoom.AddSmoothly(m->ViewZoomSpeed * DeltaTime);
	if (hotkeys[HOTKEY_CAMERA_ZOOM_OUT])
		m->Zoom.AddSmoothly(-m->ViewZoomSpeed * DeltaTime);

	float zoomDelta = m->Zoom.Update(DeltaTime);
	if (zoomDelta)
	{
		CVector3D forwards = m->ViewCamera.m_Orientation.GetIn();
		m->PosX.AddSmoothly(forwards.X * zoomDelta);
		m->PosY.AddSmoothly(forwards.Y * zoomDelta);
		m->PosZ.AddSmoothly(forwards.Z * zoomDelta);
	}

	if (m->ConstrainCamera)
		m->RotateX.ClampSmoothly(DEGTORAD(m->ViewRotateXMin), DEGTORAD(m->ViewRotateXMax));

	ClampDistance(m, true);

	m->PosX.Update(DeltaTime);
	m->PosY.Update(DeltaTime);
	m->PosZ.Update(DeltaTime);

	// Handle rotation around the Y (vertical) axis
	{
		CCamera targetCam = m->ViewCamera;
		SetupCameraMatrix(m, &targetCam.m_Orientation);

		float rotateYDelta = m->RotateY.Update(DeltaTime);
		if (rotateYDelta)
		{
			// We've updated RotateY, and need to adjust Pos so that it's still
			// facing towards the original focus point (the terrain in the center
			// of the screen).

			CVector3D upwards(0.0f, 1.0f, 0.0f);

			CVector3D pivot = targetCam.GetFocus();
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
		SetupCameraMatrix(m, &targetCam.m_Orientation);

		float rotateXDelta = m->RotateX.Update(DeltaTime);
		if (rotateXDelta)
		{
			CVector3D rightwards = targetCam.m_Orientation.GetLeft() * -1.0f;

			CVector3D pivot = m->ViewCamera.GetFocus();
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

	// Update the camera matrix
	SetupCameraMatrix(m, &m->ViewCamera.m_Orientation);
	m->ViewCamera.UpdateFrustum();
}

void CGameView::MoveCameraTarget(const CVector3D& target)
{
	// Maintain the same orientation and level of zoom, if we can
	// (do this by working out the point the camera is looking at, saving
	//  the difference between that position and the camera point, and restoring
	//  that difference to our new target)

	CCamera targetCam = m->ViewCamera;
	targetCam.m_Orientation.SetIdentity();
	targetCam.m_Orientation.RotateX(m->RotateX.GetValue());
	targetCam.m_Orientation.RotateY(m->RotateY.GetValue());
	targetCam.m_Orientation.Translate(m->PosX.GetValue(), m->PosY.GetValue(), m->PosZ.GetValue());

	CVector3D pivot = targetCam.GetFocus();
	CVector3D delta = target - pivot;
	m->PosX.SetValueSmoothly(delta.X + m->PosX.GetValue());
	m->PosY.SetValueSmoothly(delta.Y + m->PosY.GetValue());
	m->PosZ.SetValueSmoothly(delta.Z + m->PosZ.GetValue());

	ClampDistance(m, false);
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

	ClampDistance(m, false);

	SetupCameraMatrix(m, &m->ViewCamera.m_Orientation);
	m->ViewCamera.UpdateFrustum();
}

InReaction game_view_handler(const SDL_Event_* ev)
{
	// put any events that must be processed even if inactive here

	if(!g_app_has_focus || !g_Game)
		return IN_PASS;

	CGameView *pView=g_Game->GetView();

	return pView->HandleEvent(ev);
}

InReaction CGameView::HandleEvent(const SDL_Event_* ev)
{
	switch(ev->ev.type)
	{

	case SDL_HOTKEYDOWN:
		switch(ev->ev.user.code)
		{
		case HOTKEY_WIREFRAME:
			if (g_Renderer.GetModelRenderMode() == SOLID)
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

		// Mouse wheel must be treated using events instead of polling,
		// because SDL auto-generates a sequence of mousedown/mouseup events
		// and we never get to see the "down" state inside Update().
		case HOTKEY_CAMERA_ZOOM_WHEEL_IN:
			m->Zoom.AddSmoothly(m->ViewZoomSpeedWheel);
			return IN_HANDLED;

		case HOTKEY_CAMERA_ZOOM_WHEEL_OUT:
			m->Zoom.AddSmoothly(-m->ViewZoomSpeedWheel);
			return IN_HANDLED;

		case HOTKEY_CAMERA_ROTATE_WHEEL_CW:
			m->RotateY.AddSmoothly(m->ViewRotateYSpeedWheel);
			return IN_HANDLED;

		case HOTKEY_CAMERA_ROTATE_WHEEL_CCW:
			m->RotateY.AddSmoothly(-m->ViewRotateYSpeedWheel);
			return IN_HANDLED;
		}
	}

	return IN_PASS;
}
