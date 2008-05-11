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
#include "ps/Interact.h"
#include "ps/Loader.h"
#include "ps/LoaderThunks.h"
#include "ps/Profile.h"
#include "ps/Pyrogenesis.h"
#include "renderer/Renderer.h"
#include "renderer/Renderer.h"
#include "renderer/SkyManager.h"
#include "renderer/WaterManager.h"
#include "scripting/ScriptableObject.h"
#include "simulation/Entity.h"
#include "simulation/EntityOrders.h"
#include "simulation/LOSManager.h"
#include "simulation/Projectile.h"

float g_MaxZoomHeight=350.0f;	//note:  Max terrain height is this minus YMinOffset
float g_YMinOffset=15.0f;


extern int g_xres, g_yres;

static CVector3D cameraBookmarks[10];
static bool bookmarkInUse[10] = { false, false, false, false, false, false, false, false, false, false };
static i8 currentBookmark = -1;

const float CGameView::defaultFOV = DEGTORAD(20.f);
const float CGameView::defaultNear = 4.f;
const float CGameView::defaultFar = 4096.f;

class CGameViewImpl : public CJSObject<CGameViewImpl>, boost::noncopyable
{
public:
	CGameViewImpl(CGame* game)
		: Game(game),
		ColladaManager(), MeshManager(ColladaManager), SkeletonAnimManager(ColladaManager),
		ObjectManager(MeshManager, SkeletonAnimManager),
		ViewCamera(),
		CullCamera(),
		LockCullCamera(false),
		Culling(true),
		ViewScrollSpeed(60),
		ViewRotateSensitivity(0.002f),
		ViewRotateSensitivityKeyboard(1.0f),
		ViewRotateAboutTargetSensitivity(0.010f),
		ViewRotateAboutTargetSensitivityKeyboard(2.0f),
		ViewDragSensitivity(0.5f),
		ViewZoomSensitivityWheel(16.0f),
		ViewZoomSensitivity(256.0f),
		ViewZoomSmoothness(0.02f),
		ViewSnapSmoothness(0.02f),
		CameraDelta(),
		CameraPivot(),
		ZoomDelta(0)
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
	 * Cache global lighting environment. This is used  to check whether the
	 * environment has changed during the last frame, so that vertex data can be updated etc.
	 */
	CLightEnv CachedLightEnv;

	CCinemaManager TrackManager;

	////////////////////////////////////////
	// Settings
	float ViewScrollSpeed;
	float ViewRotateSensitivity;
	float ViewRotateSensitivityKeyboard;
	float ViewRotateAboutTargetSensitivity;
	float ViewRotateAboutTargetSensitivityKeyboard;
	float ViewDragSensitivity;
	float ViewZoomSensitivityWheel;
	float ViewZoomSensitivity;
	float ViewZoomSmoothness; // 0.0 = instantaneous zooming, 1.0 = so slow it never moves
	float ViewSnapSmoothness; // Just the same.


	////////////////////////////////////////
	// Camera Controls State
	CVector3D CameraDelta;
	CVector3D CameraPivot;

	CEntity* UnitView;
	CModel* UnitViewProp;
	CEntity* UnitAttach;
	//float m_CameraZoom;
	std::vector<CVector3D> CameraTargets;

	// Accumulate zooming changes across frames for smoothness
	float ZoomDelta;

	// JS Interface
	bool JSI_StartCustomSelection(JSContext *cx, uintN argc, jsval *argv);
	bool JSI_EndCustomSelection(JSContext *cx, uintN argc, jsval *argv);

	static void ScriptingInit();
};

CGameView::CGameView(CGame *pGame):
	m(new CGameViewImpl(pGame))
{
	SViewPort vp;
	vp.m_X=0;
	vp.m_Y=0;
	vp.m_Width=g_xres;
	vp.m_Height=g_yres;
	m->ViewCamera.SetViewPort(&vp);

	m->ViewCamera.SetProjection (defaultNear, defaultFar, defaultFOV);
	m->ViewCamera.m_Orientation.SetXRotation(DEGTORAD(30));
	m->ViewCamera.m_Orientation.RotateY(DEGTORAD(0));
	m->ViewCamera.m_Orientation.Translate (100, 150, -100);
	m->CullCamera = m->ViewCamera;
	g_Renderer.SetSceneCamera(m->ViewCamera, m->CullCamera);

	m->UnitView=NULL;
	m->UnitAttach=NULL;
}

CGameView::~CGameView()
{
	g_Selection.ClearSelection();
	g_Mouseover.Clear();
	g_BuildingPlacer.Deactivate();
	UnloadResources();

	delete m;
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

void CGameView::AttachToUnit(CEntity* target)
{
	m->UnitAttach = target;
}

bool CGameView::IsAttached()
{
	return (m->UnitAttach != NULL);
}

bool CGameView::IsUnitView()
{
	return (m->UnitView != NULL);
}


void CGameViewImpl::ScriptingInit()
{
	AddMethod<bool, &CGameViewImpl::JSI_StartCustomSelection>("startCustomSelection", 0);
	AddMethod<bool, &CGameViewImpl::JSI_EndCustomSelection>("endCustomSelection", 0);
	AddProperty(L"culling", &CGameViewImpl::Culling);
	AddProperty(L"lockCullCamera", &CGameViewImpl::LockCullCamera);

	CJSObject<CGameViewImpl>::ScriptingInit("GameView");
}

int CGameView::Initialize(CGameAttributes* UNUSED(pAttribs))
{
	CFG_GET_SYS_VAL( "view.scroll.speed", Float, m->ViewScrollSpeed );
	CFG_GET_SYS_VAL( "view.rotate.speed", Float, m->ViewRotateSensitivity );
	CFG_GET_SYS_VAL( "view.rotate.keyboard.speed", Float, m->ViewRotateSensitivityKeyboard );
	CFG_GET_SYS_VAL( "view.rotate.abouttarget.speed", Float, m->ViewRotateAboutTargetSensitivity );
	CFG_GET_SYS_VAL( "view.rotate.keyboard.abouttarget.speed", Float, m->ViewRotateAboutTargetSensitivityKeyboard );
	CFG_GET_SYS_VAL( "view.drag.speed", Float, m->ViewDragSensitivity );
	CFG_GET_SYS_VAL( "view.zoom.speed", Float, m->ViewZoomSensitivity );
	CFG_GET_SYS_VAL( "view.zoom.wheel.speed", Float, m->ViewZoomSensitivityWheel );
	CFG_GET_SYS_VAL( "view.zoom.smoothness", Float, m->ViewZoomSmoothness );
	CFG_GET_SYS_VAL( "view.snap.smoothness", Float, m->ViewSnapSmoothness );

	if( ( m->ViewZoomSmoothness < 0.0f ) || ( m->ViewZoomSmoothness > 1.0f ) )
		m->ViewZoomSmoothness = 0.02f;
	if( ( m->ViewSnapSmoothness < 0.0f ) || ( m->ViewSnapSmoothness > 1.0f ) )
		m->ViewSnapSmoothness = 0.02f;

	return 0;
}



void CGameView::RegisterInit(CGameAttributes *pAttribs)
{
	// CGameView init
	RegMemFun1(this, &CGameView::Initialize, pAttribs, L"CGameView init", 1);

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

		// This can be uncommented to try getting a bigger frustum.. 
		// but then it makes shadow maps too low-detail.
		//m_CullCamera.SetProjection(1.0f, 10000.0f, DEGTORAD(30));
		//m_CullCamera.UpdateFrustum();
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
			CPatch* patch=pTerrain->GetPatch(i,j);

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
			CPatch* patch=pTerrain->GetPatch(i,j);
			if(patch->getDrawState() == true)
			{
				c->Submit(patch);
				patch->setDrawState(false);
			}
		}
	}
	PROFILE_END( "submit terrain" );

	PROFILE_START( "submit models" );
	CWorld* world = m->Game->GetWorld();
	CUnitManager& unitMan = world->GetUnitManager();
	CProjectileManager& pProjectileMan = world->GetProjectileManager();
	CLOSManager* losMgr = world->GetLOSManager();

	const std::vector<CUnit*>& units = unitMan.GetUnits();
	for (size_t i=0;i<units.size();++i)
	{
		ogl_WarnIfError();

		CEntity* ent = units[i]->GetEntity();
		if( ent && !ent->m_visible )
			continue;

		int status = losMgr->GetUnitStatus(units[i], g_Game->GetLocalPlayer());
		CModel* model = units[i]->GetModel();

		model->ValidatePosition();
		
		if (status != UNIT_HIDDEN &&
			(!m->Culling || frustum.IsBoxVisible(CVector3D(0,0,0), model->GetBounds())))
		{
			if(units[i] != g_BuildingPlacer.m_actor)
			{
				CColor color;
				if(status == UNIT_VISIBLE)
				{
					color = CColor(1.0f, 1.0f, 1.0f, 1.0f);
				}
				else	// status == UNIT_REMEMBERED
				{
					color = CColor(0.7f, 0.7f, 0.7f, 1.0f);
				}
				model->SetShadingColor(color);
			}

			PROFILE( "submit models" );
			c->SubmitRecursive(model);
		}
	}

	const std::list<CProjectile*>& projectiles = pProjectileMan.GetProjectiles();
	std::list<CProjectile*>::const_iterator it = projectiles.begin();
	for (; it != projectiles.end(); ++it)
	{
		CModel* model = (*it)->GetModel();

		model->ValidatePosition();

		const CBound& bound = model->GetBounds();
		CVector3D centre;
		bound.GetCentre(centre);

		if ((!m->Culling || frustum.IsBoxVisible(CVector3D(0,0,0), bound))
			&& losMgr->GetStatus(centre.X, centre.Z, g_Game->GetLocalPlayer()) & LOS_VISIBLE)
		{
			PROFILE( "submit projectiles" );
			c->SubmitRecursive((*it)->GetModel());
		}
	}
	PROFILE_END( "submit models" );
}


//locks the camera in place
void CGameView::CameraLock(const CVector3D& Trans, bool smooth)
{
	CameraLock(Trans.X, Trans.Y, Trans.Z, smooth);
}

void CGameView::CameraLock(float x, float y, float z, bool smooth)
{
	CTerrain* pTerrain = m->Game->GetWorld()->GetTerrain();
	float height = pTerrain->GetExactGroundLevel(
			m->ViewCamera.m_Orientation._14 + x, m->ViewCamera.m_Orientation._34 + z) +
			g_YMinOffset;
	//is requested position within limits?
	if (m->ViewCamera.m_Orientation._24 + y <= g_MaxZoomHeight)
	{
		if( m->ViewCamera.m_Orientation._24 + y >= height)
		{
			m->ViewCamera.m_Orientation.Translate(x, y, z);
		}
		else if (m->ViewCamera.m_Orientation._24 + y < height && smooth == true)
		{
			m->ViewCamera.m_Orientation.Translate(x, y, z);
			m->ViewCamera.m_Orientation._24=height;
		}
	}
}


static void MarkUpdateColorRecursive(CModel* model)
{
	model->SetDirty(RENDERDATA_UPDATE_COLOR);

	const std::vector<CModel::Prop>& props = model->GetProps();
	for(size_t i = 0; i < props.size(); ++i) {
		MarkUpdateColorRecursive(props[i].m_Model);
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


void CGameView::ResetCamera()
{
	// quick hack to return camera home, for screenshots (after alt+tabbing)
	m->ViewCamera.SetProjection (defaultNear, defaultFar, defaultFOV);
	m->ViewCamera.m_Orientation.SetXRotation(DEGTORAD(30));
	m->ViewCamera.m_Orientation.RotateY(DEGTORAD(-45));
	m->ViewCamera.m_Orientation.Translate (100, 150, -100);
}

void CGameView::ResetCameraOrientation()
{

	CVector3D origin = m->ViewCamera.m_Orientation.GetTranslation();
	CVector3D dir = m->ViewCamera.m_Orientation.GetIn();

	CVector3D target = origin + dir * ( ( 50.0f - origin.Y ) / dir.Y );

	target -= CVector3D( -22.474480f, 50.0f, 22.474480f );

	m->ViewCamera.SetProjection (defaultNear, defaultFar, defaultFOV);
	m->ViewCamera.m_Orientation.SetXRotation(DEGTORAD(30));
	m->ViewCamera.m_Orientation.RotateY(DEGTORAD(-45));

	target += CVector3D( 100.0f, 150.0f, -100.0f );

	m->ViewCamera.m_Orientation.Translate( target );
}

void CGameView::RotateAboutTarget()
{
	m->CameraPivot = m->ViewCamera.GetWorldCoordinates(true);
}

void CGameView::Update(float DeltaTime)
{
	if (!g_app_has_focus)
		return;

	if (m->UnitView)
	{
		m->ViewCamera.m_Orientation.SetYRotation(m->UnitView->m_orientation.Y);
		m->ViewCamera.m_Orientation.Translate(m->UnitViewProp->GetTransform().GetTranslation());
		m->ViewCamera.UpdateFrustum();
		return;
	}

	if (m->UnitAttach)
	{
		CVector3D ToMove = m->UnitAttach->m_position - m->ViewCamera.GetFocus();
		m->ViewCamera.m_Orientation._14 += ToMove.X;
		m->ViewCamera.m_Orientation._34 += ToMove.Z;
		m->ViewCamera.UpdateFrustum();
		return;
	}

	if (m->TrackManager.IsActive() && m->TrackManager.IsPlaying())
	{
		if (! m->TrackManager.Update(DeltaTime))
			ResetCamera();
		return;
	}

	float delta = powf( m->ViewSnapSmoothness, DeltaTime );
	m->ViewCamera.m_Orientation.Translate( m->CameraDelta * ( 1.0f - delta ) );
	m->CameraDelta *= delta;


	// This could be rewritten much more reliably, so it doesn't e.g. accidentally tilt
	// the camera, assuming we know exactly what limits the camera should have.


	// Calculate mouse movement
	static int mouse_last_x = 0;
	static int mouse_last_y = 0;
	int mouse_dx = g_mouse_x - mouse_last_x;
	int mouse_dy = g_mouse_y - mouse_last_y;
	mouse_last_x = g_mouse_x;
	mouse_last_y = g_mouse_y;

	// Miscellaneous vectors
	CVector3D forwards = m->ViewCamera.m_Orientation.GetIn();
	CVector3D rightwards = m->ViewCamera.m_Orientation.GetLeft() * -1.0f; // upwards.Cross(forwards);
	CVector3D upwards( 0.0f, 1.0f, 0.0f );
	// rightwards.Normalize();

	CVector3D forwards_horizontal = forwards;
	forwards_horizontal.Y = 0.0f;
	forwards_horizontal.Normalize();

	if( hotkeys[HOTKEY_CAMERA_ROTATE] || hotkeys[HOTKEY_CAMERA_ROTATE_KEYBOARD] )
	{
		// Ctrl + middle-drag or left-and-right-drag to rotate view

		// Untranslate the camera, so it rotates around the correct point
		CVector3D position = m->ViewCamera.m_Orientation.GetTranslation();
		m->ViewCamera.m_Orientation.Translate(position*-1);

		// Sideways rotation

		float rightways = 0.0f;
		if( hotkeys[HOTKEY_CAMERA_ROTATE] )
			rightways = (float)mouse_dx * m->ViewRotateSensitivity;
		if( hotkeys[HOTKEY_CAMERA_ROTATE_KEYBOARD] )
		{
			if( hotkeys[HOTKEY_CAMERA_LEFT] )
				rightways -= m->ViewRotateSensitivityKeyboard * DeltaTime;
			if( hotkeys[HOTKEY_CAMERA_RIGHT] )
				rightways += m->ViewRotateSensitivityKeyboard * DeltaTime;
		}

		m->ViewCamera.m_Orientation.RotateY( rightways );

		// Up/down rotation

		float upways = 0.0f;
		if( hotkeys[HOTKEY_CAMERA_ROTATE] )
			upways = (float)mouse_dy * m->ViewRotateSensitivity;
		if( hotkeys[HOTKEY_CAMERA_ROTATE_KEYBOARD] )
		{
			if( hotkeys[HOTKEY_CAMERA_UP] )
				upways -= m->ViewRotateSensitivityKeyboard * DeltaTime;
			if( hotkeys[HOTKEY_CAMERA_DOWN] )
				upways += m->ViewRotateSensitivityKeyboard * DeltaTime;
		}

		CQuaternion temp;
		temp.FromAxisAngle(rightwards, upways);

		m->ViewCamera.m_Orientation.Rotate(temp);

		// Retranslate back to the right position
		m->ViewCamera.m_Orientation.Translate(position);

	}
	else if( hotkeys[HOTKEY_CAMERA_ROTATE_ABOUT_TARGET] )
	{
		CVector3D origin = m->ViewCamera.m_Orientation.GetTranslation();
		CVector3D delta = origin - m->CameraPivot;

		CQuaternion rotateH, rotateV; CMatrix3D rotateM;

		// Sideways rotation

		float rightways = (float)mouse_dx * m->ViewRotateAboutTargetSensitivity;

		rotateH.FromAxisAngle( upwards, rightways );

		// Up/down rotation

		float upways = (float)mouse_dy * m->ViewRotateAboutTargetSensitivity;
		rotateV.FromAxisAngle( rightwards, upways );

		rotateH *= rotateV;
		rotateH.ToMatrix( rotateM );

		delta = rotateM.Rotate( delta );

		// Lock the inclination to a rather arbitrary values (for the sake of graphical decency)

		float scan = sqrt( delta.X * delta.X + delta.Z * delta.Z ) / delta.Y;
		if( ( scan >= 0.5f ) )
		{
			// Move the camera to the origin (in preparation for rotation )
			m->ViewCamera.m_Orientation.Translate( origin * -1.0f );

			m->ViewCamera.m_Orientation.Rotate( rotateH );

			// Move the camera back to where it belongs
			m->ViewCamera.m_Orientation.Translate( m->CameraPivot + delta );
		}

	}
	else if( hotkeys[HOTKEY_CAMERA_ROTATE_ABOUT_TARGET_KEYBOARD] )
	{
		// Split up because the keyboard controls use the centre of the screen, not the mouse position.
		CVector3D origin = m->ViewCamera.m_Orientation.GetTranslation();
		CVector3D pivot = m->ViewCamera.GetFocus();
		CVector3D delta = origin - pivot;

		CQuaternion rotateH, rotateV; CMatrix3D rotateM;

		// Sideways rotation

		float rightways = 0.0f;
		if( hotkeys[HOTKEY_CAMERA_LEFT] )
			rightways -= m->ViewRotateAboutTargetSensitivityKeyboard * DeltaTime;
		if( hotkeys[HOTKEY_CAMERA_RIGHT] )
			rightways += m->ViewRotateAboutTargetSensitivityKeyboard * DeltaTime;

		rotateH.FromAxisAngle( upwards, rightways );

		// Up/down rotation

		float upways = 0.0f;
		if( hotkeys[HOTKEY_CAMERA_UP] )
			upways -= m->ViewRotateAboutTargetSensitivityKeyboard * DeltaTime;
		if( hotkeys[HOTKEY_CAMERA_DOWN] )
			upways += m->ViewRotateAboutTargetSensitivityKeyboard * DeltaTime;

		rotateV.FromAxisAngle( rightwards, upways );

		rotateH *= rotateV;
		rotateH.ToMatrix( rotateM );

		delta = rotateM.Rotate( delta );

		// Lock the inclination to a rather arbitrary values (for the sake of graphical decency)

		float scan = sqrt( delta.X * delta.X + delta.Z * delta.Z ) / delta.Y;
		if( ( scan >= 0.5f ) )
		{
			// Move the camera to the origin (in preparation for rotation )
			m->ViewCamera.m_Orientation.Translate( origin * -1.0f );

			m->ViewCamera.m_Orientation.Rotate( rotateH );

			// Move the camera back to where it belongs
			m->ViewCamera.m_Orientation.Translate( pivot + delta );
		}

	}
	else if( hotkeys[HOTKEY_CAMERA_PAN] )
	{
		// Middle-drag to pan
		//keep camera in bounds
			CameraLock(rightwards * (m->ViewDragSensitivity * mouse_dx));
			CameraLock(forwards_horizontal * (-m->ViewDragSensitivity * mouse_dy));
	}

	// Mouse movement
	if( !hotkeys[HOTKEY_CAMERA_ROTATE] && !hotkeys[HOTKEY_CAMERA_ROTATE_ABOUT_TARGET] )
	{
		if (g_mouse_x >= g_xres-2 && g_mouse_x < g_xres)
			CameraLock(rightwards * (m->ViewScrollSpeed * DeltaTime));
		else if (g_mouse_x <= 3 && g_mouse_x >= 0)
			CameraLock(-rightwards * (m->ViewScrollSpeed * DeltaTime));

		if (g_mouse_y >= g_yres-2 && g_mouse_y < g_yres)
			CameraLock(-forwards_horizontal * (m->ViewScrollSpeed * DeltaTime));
		else if (g_mouse_y <= 3 && g_mouse_y >= 0)
			CameraLock(forwards_horizontal * (m->ViewScrollSpeed * DeltaTime));

	}

	// Keyboard movement (added to mouse movement, so you can go faster if you want)

	if( hotkeys[HOTKEY_CAMERA_PAN_KEYBOARD] )
	{
		if( hotkeys[HOTKEY_CAMERA_RIGHT] )
			CameraLock(rightwards * (m->ViewScrollSpeed * DeltaTime));
		if( hotkeys[HOTKEY_CAMERA_LEFT] )
			CameraLock(-rightwards * (m->ViewScrollSpeed * DeltaTime));

		if( hotkeys[HOTKEY_CAMERA_DOWN] )
			CameraLock(-forwards_horizontal * (m->ViewScrollSpeed * DeltaTime));
		if( hotkeys[HOTKEY_CAMERA_UP] )
			CameraLock(forwards_horizontal * (m->ViewScrollSpeed * DeltaTime));

	}

	// Smoothed zooming (move a certain percentage towards the desired zoom distance every frame)
	// Note that scroll wheel zooming is event-based and handled in game_view_handler

	if( hotkeys[HOTKEY_CAMERA_ZOOM_IN] )
		m->ZoomDelta += m->ViewZoomSensitivity*DeltaTime;
	else if( hotkeys[HOTKEY_CAMERA_ZOOM_OUT] )
		m->ZoomDelta -= m->ViewZoomSensitivity*DeltaTime;

	if (fabsf(m->ZoomDelta) > 0.1f) // use a fairly high limit to avoid nasty flickering when zooming
	{
		float zoom_proportion = powf(m->ViewZoomSmoothness, DeltaTime);
		CameraLock(forwards * (m->ZoomDelta * (1.0f-zoom_proportion)), false);
		m->ZoomDelta *= zoom_proportion;
	}

	m->ViewCamera.UpdateFrustum ();
}

void CGameView::ToUnitView(CEntity* target, CModel* prop)
{
	if( !target )
	{
		//prevent previous zooming
		m->ZoomDelta = 0.0f;
		ResetCamera();
		SetCameraTarget( m->UnitView->m_position );
	}
	m->UnitView = target;
	m->UnitViewProp = prop;

}
void CGameView::PushCameraTarget( const CVector3D& target )
{
	// Save the current position
	m->CameraTargets.push_back( m->ViewCamera.m_Orientation.GetTranslation() );
	// And set the camera
	SetCameraTarget( target );
}

void CGameView::SetCameraTarget( const CVector3D& target )
{
	// Maintain the same orientation and level of zoom, if we can
	// (do this by working out the point the camera is looking at, saving
	//  the difference between that position and the camera point, and restoring
	//  that difference to our new target)

	CVector3D CurrentTarget = m->ViewCamera.GetFocus();
	m->CameraDelta = target - CurrentTarget;
}

void CGameView::PopCameraTarget()
{
	m->CameraDelta = m->CameraTargets.back() - m->ViewCamera.m_Orientation.GetTranslation();
	m->CameraTargets.pop_back();
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
			return( IN_HANDLED );

		case HOTKEY_CAMERA_RESET_ORIGIN:
			ResetCamera();
			return( IN_HANDLED );

		case HOTKEY_CAMERA_RESET:
			ResetCameraOrientation();
			return( IN_HANDLED );

		case HOTKEY_CAMERA_ROTATE_ABOUT_TARGET:
			RotateAboutTarget();
			return( IN_HANDLED );

		// Mouse wheel must be treated using events instead of polling,
		// because SDL auto-generates a sequence of mousedown/mouseup events
		// and we never get to see the "down" state inside Update().
		case HOTKEY_CAMERA_ZOOM_WHEEL_IN:
			m->ZoomDelta += m->ViewZoomSensitivityWheel;
			return( IN_HANDLED );

		case HOTKEY_CAMERA_ZOOM_WHEEL_OUT:
			m->ZoomDelta -= m->ViewZoomSensitivityWheel;
			return( IN_HANDLED );

		default:

			if( ( ev->ev.user.code >= HOTKEY_CAMERA_BOOKMARK_0 ) && ( ev->ev.user.code <= HOTKEY_CAMERA_BOOKMARK_9 ) )
			{
				// The above test limits it to 10 bookmarks, so don't worry about overflowing
				i8 id = (i8)( ev->ev.user.code - HOTKEY_CAMERA_BOOKMARK_0 );

				if( hotkeys[HOTKEY_CAMERA_BOOKMARK_SAVE] )
				{
					// Attempt to track the ground we're looking at
					cameraBookmarks[id] = GetCamera()->GetFocus();
					bookmarkInUse[id] = true;
				}
				else if( hotkeys[HOTKEY_CAMERA_BOOKMARK_SNAP] )
				{
					if( bookmarkInUse[id] && ( currentBookmark == -1 ) )
					{
						PushCameraTarget( cameraBookmarks[id] );
						currentBookmark = id;
					}
				}
				else
				{
					if( bookmarkInUse[id] )
						SetCameraTarget( cameraBookmarks[id] );
				}
				return( IN_HANDLED );
			}
		}
	case SDL_HOTKEYUP:
		switch( ev->ev.user.code )
		{
			case HOTKEY_CAMERA_BOOKMARK_SNAP:
				if( currentBookmark != -1 )
					PopCameraTarget();
				currentBookmark = -1;
				break;

			default:
				return( IN_PASS );
		}
		return( IN_HANDLED );
	}

	return IN_PASS;
}

bool CGameViewImpl::JSI_StartCustomSelection(
	JSContext* UNUSED(context), size_t UNUSED(argc), jsval* UNUSED(argv))
{
	StartCustomSelection();
	return true;
}

bool CGameViewImpl::JSI_EndCustomSelection(
	JSContext* UNUSED(context), size_t UNUSED(argc), jsval* UNUSED(argv))
{
	ResetInteraction();
	return true;
}
