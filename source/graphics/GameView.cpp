

#include "precompiled.h"

#include "Terrain.h"
#include "Renderer.h"
#include "GameView.h"
#include "Game.h"
#include "Camera.h"
#include "Interact.h"

#include "Matrix3D.h"
#include "MathUtil.h"
#include "Renderer.h"
#include "Terrain.h"
#include "LightEnv.h"
#include "HFTracer.h"
#include "TextureManager.h"
#include "ObjectManager.h"
#include "LOSManager.h"
#include "EntityOrders.h"
#include "Bound.h"
#include "Pyrogenesis.h"
#include "Hotkey.h"
#include "ConfigDB.h"
#include "Loader.h"
#include "Profile.h"
#include "ps/LoaderThunks.h"
#include "ps/Globals.h"
#include "renderer/WaterManager.h"

#include "Quaternion.h"
#include "Unit.h"
#include "Model.h"
#include "Projectile.h"

#include "input.h"
#include "lib.h"
#include "timer.h"

float g_MaxZoomHeight=350.0f;	//note:  Max terrain height is this minus YMinOffset
float g_YMinOffset=15.0f;


extern int g_xres, g_yres;

static CVector3D cameraBookmarks[10];
static bool bookmarkInUse[10] = { false, false, false, false, false, false, false, false, false, false };
static i8 currentBookmark = -1;

const float CGameView::defaultFOV = DEGTORAD(20.f);
const float CGameView::defaultNear = 1.f;
const float CGameView::defaultFar = 5000.f;

CGameView::CGameView(CGame *pGame):
	m_pGame(pGame),
	m_pWorld(pGame->GetWorld()),
	m_ViewCamera(),
	m_CullCamera(),
	m_LockCullCamera(false),
	m_ViewScrollSpeed(60),
	m_ViewRotateSensitivity(0.002f),
	m_ViewRotateSensitivityKeyboard(1.0f),
	m_ViewRotateAboutTargetSensitivity(0.010f),
	m_ViewRotateAboutTargetSensitivityKeyboard(2.0f),
	m_ViewDragSensitivity(0.5f),
	m_ViewZoomSensitivityWheel(16.0f),
	m_ViewZoomSensitivity(256.0f),
	m_ViewZoomSmoothness(0.02f),
	m_ViewSnapSmoothness(0.02f),
	m_CameraDelta(),
	m_CameraPivot(),
	m_ZoomDelta(0)
{
	SViewPort vp;
	vp.m_X=0;
	vp.m_Y=0;
	vp.m_Width=g_xres;
	vp.m_Height=g_yres;
	m_ViewCamera.SetViewPort(&vp);

	m_ViewCamera.SetProjection (defaultNear, defaultFar, defaultFOV);
	m_ViewCamera.m_Orientation.SetXRotation(DEGTORAD(30));
	m_ViewCamera.m_Orientation.RotateY(DEGTORAD(0));
	m_ViewCamera.m_Orientation.Translate (100, 150, -100);
	m_CullCamera = m_ViewCamera;
	g_Renderer.SetCamera(m_ViewCamera, m_CullCamera);

	m_UnitView=NULL;
	m_UnitAttach=NULL;

	/* //TEST TRACK
	int Test_Nodes=15;
	int Test_Variance=10.0f;	//rand() % Test_Variance distortion of linear points
	int Test_Space=30;
	float Test_NodeTime = 0.65f;

	CVector3D Test_Direction(1.0f, 0.0f, 1.0f);
	CVector3D Test_StartPos(100, 150, -80);
	Test_Direction.Normalize();

	CCinemaData Test_Data;
	TNSpline Test_Spline;
	CCinemaTrack Test_Track;

	//linear generation with variance factor
	for (int i=0; i<Test_Nodes; i++)
	{
		CVector3D linear = Test_StartPos + Test_Direction * i * Test_Space;
		if ( rand() % 2 )
			linear.X += rand() % Test_Variance;
		else
			linear.X -= rand() % Test_Variance;

		if ( rand() % 2 )
			linear.Y += rand() % Test_Variance;
		else
			linear.Y -= rand() % Test_Variance;

		if ( rand() % 2 )
			linear.Z += rand() % Test_Variance;
		else
			linear.Z -= rand() % Test_Variance;

		Test_Spline.AddNode( linear, Test_NodeTime );
	}

	Test_Data.m_TotalDuration = Test_Spline.MaxDistance;
	Test_Data.m_TotalRotation = CVector3D( 0.0f, DEGTORAD(-150.0f), 0.0f );
	Test_Data.m_Growth = Test_Data.m_GrowthCount = 1.9f;
	Test_Data.m_Switch = .5f;
	Test_Data.m_Mode = EM_IN;
	Test_Data.m_Style = ES_DEFAULT;
	Test_Track.m_StartRotation = CVector3D( DEGTORAD(30), DEGTORAD(-45), 0.0f );

	Test_Track.AddPath( Test_Data, Test_Spline );

	Test_Data.m_Style = ES_GROWTH;
	Test_Track.AddPath( Test_Data, Test_Spline );

	Test_Data.m_Mode = EM_OUT;
	Test_Data.m_Style = ES_EXPO;
	Test_Track.AddPath( Test_Data, Test_Spline );

	Test_Data.m_Mode = EM_INOUT;
	Test_Data.m_Style = ES_GROWTH;
	Test_Track.AddPath( Test_Data, Test_Spline );

	Test_Data.m_Mode = EM_IN;
	Test_Data.m_Style = ES_SINE;
	Test_Track.AddPath( Test_Data, Test_Spline );

	m_TrackManager.AddTrack(Test_Track, "test");
	m_TrackManager.QueueTrack("test", true);
	*/

	//TEST TRACK
	CVector3D Test_NodePos[] = { CVector3D( 109.31228f, 93.302315f, -28.760098f ),
								CVector3D( 147.03696f, 87.134117f, 54.718628f ),
								CVector3D( 174.245293f, 97.134117f, 116.449940f ),
								CVector3D( 146.245293f, 107.134117f, 162.449940f ),
								CVector3D( 117.245293f, 97.134117f, 134.449940f ),
								CVector3D( 72.88226f, 87.134117f, 114.373001f ),
								CVector3D( 50.39368f, 77.71053f, 99.925789f ) };
	float Test_NodeTime = 0.85f;

	CCinemaData Test_Data;
	TNSpline Test_Spline;
	CCinemaTrack Test_Track;

	//linear generation with variance factor
	for (int i=0; i<ARRAY_SIZE(Test_NodePos); i++)
	{
		Test_Spline.AddNode( Test_NodePos[i], Test_NodeTime );
	}

	Test_Data.m_TotalDuration = Test_Spline.MaxDistance;
	Test_Data.m_TotalRotation = CVector3D( DEGTORAD(15.0f), DEGTORAD(-100.0f), 0.0f );
	Test_Data.m_Growth = Test_Data.m_GrowthCount = 1.9f;
	Test_Data.m_Switch = .5f;
	Test_Data.m_Mode = EM_IN;
	Test_Data.m_Style = ES_DEFAULT;
	Test_Track.m_StartRotation = CVector3D( DEGTORAD(30), DEGTORAD(-45), 0.0f );

	Test_Track.AddPath( Test_Data, Test_Spline );

	Test_Data.m_Style = ES_GROWTH;
	Test_Track.AddPath( Test_Data, Test_Spline );

	Test_Data.m_Mode = EM_OUT;
	Test_Data.m_Style = ES_EXPO;
	Test_Track.AddPath( Test_Data, Test_Spline );

	Test_Data.m_Mode = EM_INOUT;
	Test_Data.m_Style = ES_GROWTH;
	Test_Track.AddPath( Test_Data, Test_Spline );

	Test_Data.m_Mode = EM_IN;
	Test_Data.m_Style = ES_SINE;
	Test_Track.AddPath( Test_Data, Test_Spline );

	m_TrackManager.AddTrack(Test_Track, "test");

	m_TestTrack.m_Paths.push_back(CCinemaPath());
	ONCE( ScriptingInit(); );
}

CGameView::~CGameView()
{
	UnloadResources();
}

void CGameView::ScriptingInit()
{
	AddMethod<bool, &CGameView::JSI_StartCustomSelection>("startCustomSelection", 0);
	AddMethod<bool, &CGameView::JSI_EndCustomSelection>("endCustomSelection", 0);
	AddProperty(L"lockCullCamera", &CGameView::m_LockCullCamera);

	CJSObject<CGameView>::ScriptingInit("GameView");
}

int CGameView::Initialize(CGameAttributes* UNUSED(pAttribs))
{
	CFG_GET_SYS_VAL( "view.scroll.speed", Float, m_ViewScrollSpeed );
	CFG_GET_SYS_VAL( "view.rotate.speed", Float, m_ViewRotateSensitivity );
	CFG_GET_SYS_VAL( "view.rotate.keyboard.speed", Float, m_ViewRotateSensitivityKeyboard );
	CFG_GET_SYS_VAL( "view.rotate.abouttarget.speed", Float, m_ViewRotateAboutTargetSensitivity );
	CFG_GET_SYS_VAL( "view.rotate.keyboard.abouttarget.speed", Float, m_ViewRotateAboutTargetSensitivityKeyboard );
	CFG_GET_SYS_VAL( "view.drag.speed", Float, m_ViewDragSensitivity );
	CFG_GET_SYS_VAL( "view.zoom.speed", Float, m_ViewZoomSensitivity );
	CFG_GET_SYS_VAL( "view.zoom.wheel.speed", Float, m_ViewZoomSensitivityWheel );
	CFG_GET_SYS_VAL( "view.zoom.smoothness", Float, m_ViewZoomSmoothness );
	CFG_GET_SYS_VAL( "view.snap.smoothness", Float, m_ViewSnapSmoothness );

	if( ( m_ViewZoomSmoothness < 0.0f ) || ( m_ViewZoomSmoothness > 1.0f ) )
		m_ViewZoomSmoothness = 0.02f;
	if( ( m_ViewSnapSmoothness < 0.0f ) || ( m_ViewSnapSmoothness > 1.0f ) )
		m_ViewSnapSmoothness = 0.02f;

	return 0;
}








void CGameView::RegisterInit(CGameAttributes *pAttribs)
{
	// CGameView init
	RegMemFun1(this, &CGameView::Initialize, pAttribs, L"CGameView init", 1);

	// previously done by CGameView::InitResources
	RegMemFun(g_TexMan.GetSingletonPtr(), &CTextureManager::LoadTerrainTextures, L"LoadTerrainTextures", 60);
	RegMemFun(g_ObjMan.GetSingletonPtr(), &CObjectManager::LoadObjects, L"LoadObjects", 1);
	RegMemFun(g_Renderer.GetSingletonPtr(), &CRenderer::LoadAlphaMaps, L"LoadAlphaMaps", 5);
	RegMemFun(g_Renderer.GetSingletonPtr()->GetWaterManager(), &WaterManager::LoadWaterTextures, L"LoadWaterTextures", 80);
}


void CGameView::Render()
{
	if (m_LockCullCamera == false)
		m_CullCamera = m_ViewCamera;
	g_Renderer.SetCamera(m_ViewCamera, m_CullCamera);

	CheckLightEnv();

	MICROLOG(L"render terrain");
	PROFILE_START( "render terrain" );
	RenderTerrain(m_pWorld->GetTerrain());
	PROFILE_END( "render terrain" );
	MICROLOG(L"render models");
	PROFILE_START( "render models" );
	RenderModels(m_pWorld->GetUnitManager(), m_pWorld->GetProjectileManager());
	PROFILE_END( "render models" );
}

void CGameView::RenderTerrain(CTerrain *pTerrain)
{
	CFrustum frustum = m_CullCamera.GetFrustum();
	u32 patchesPerSide=pTerrain->GetPatchesPerSide();
	for (uint j=0; j<patchesPerSide; j++) {
		for (uint i=0; i<patchesPerSide; i++) {
			CPatch* patch=pTerrain->GetPatch(i,j);
			if (frustum.IsBoxVisible (CVector3D(0,0,0),patch->GetBounds())) {
				g_Renderer.Submit(patch);
			}
		}
	}
}

void CGameView::RenderModels(CUnitManager *pUnitMan, CProjectileManager *pProjectileMan)
{
	CFrustum frustum = m_CullCamera.GetFrustum();
	CLOSManager* losMgr = m_pWorld->GetLOSManager();

	const std::vector<CUnit*>& units=pUnitMan->GetUnits();
	for (uint i=0;i<units.size();++i)
	{
		int status = losMgr->GetUnitStatus(units[i], g_Game->GetLocalPlayer());
		CModel* model = units[i]->GetModel();

		model->ValidatePosition();

		if (frustum.IsBoxVisible(CVector3D(0,0,0), model->GetBounds())
			&& status != UNIT_HIDDEN)
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
			SubmitModelRecursive(model);
		}
	}

	const std::vector<CProjectile*>& projectiles=pProjectileMan->GetProjectiles();
	for (uint i=0;i<projectiles.size();++i)
	{
		CModel* model = projectiles[i]->GetModel();

		model->ValidatePosition();

		const CBound& bound = model->GetBounds();
		CVector3D centre;
		bound.GetCentre(centre);

		if (frustum.IsBoxVisible(CVector3D(0,0,0), bound)
			&& losMgr->GetStatus(centre.X, centre.Z, g_Game->GetLocalPlayer()) & LOS_VISIBLE)
		{
			PROFILE( "submit projectiles" );
			SubmitModelRecursive(projectiles[i]->GetModel());
		}
	}
}


//locks the camera in place
void CGameView::CameraLock(CVector3D Trans, bool smooth)
{
	CTerrain* pTerrain = m_pWorld->GetTerrain();
	float height=pTerrain->getExactGroundLevel(
			m_ViewCamera.m_Orientation._14 + Trans.X, m_ViewCamera.m_Orientation._34 + Trans.Z) +
			g_YMinOffset;
	//is requested position within limits?
	if (m_ViewCamera.m_Orientation._24 + Trans.Y <= g_MaxZoomHeight)
	{
		if( m_ViewCamera.m_Orientation._24 + Trans.Y >= height)
		{
			m_ViewCamera.m_Orientation.Translate(Trans);
		}
		else if (m_ViewCamera.m_Orientation._24 + Trans.Y < height && smooth == true)
		{
			m_ViewCamera.m_Orientation.Translate(Trans);
			m_ViewCamera.m_Orientation._24=height;
		}


	}
}

void CGameView::CameraLock(float x, float y, float z, bool smooth)
{
	CTerrain* pTerrain = m_pWorld->GetTerrain();
	float height = pTerrain->getExactGroundLevel(
			m_ViewCamera.m_Orientation._14 + x, m_ViewCamera.m_Orientation._34 + z) +
			g_YMinOffset;
	//is requested position within limits?
	if (m_ViewCamera.m_Orientation._24 + y <= g_MaxZoomHeight)
	{
		if( m_ViewCamera.m_Orientation._24 + y >= height)
		{
			m_ViewCamera.m_Orientation.Translate(x, y, z);
		}
		else if (m_ViewCamera.m_Orientation._24 + y < height && smooth == true)
		{
			m_ViewCamera.m_Orientation.Translate(x, y, z);
			m_ViewCamera.m_Orientation._24=height;
		}
	}
}


void CGameView::SubmitModelRecursive(CModel* model)
{
	g_Renderer.Submit(model);

	const std::vector<CModel::Prop>& props=model->GetProps();
	for (uint i=0;i<props.size();i++) {
		SubmitModelRecursive(props[i].m_Model);
	}
}

void CGameView::RenderNoCull()
{
	CUnitManager *pUnitMan=m_pWorld->GetUnitManager();
	CTerrain *pTerrain=m_pWorld->GetTerrain();

	if (m_LockCullCamera == false)
		m_CullCamera = m_ViewCamera;
	g_Renderer.SetCamera(m_ViewCamera, m_CullCamera);

	CheckLightEnv();

	uint i,j;
	const std::vector<CUnit*>& units=pUnitMan->GetUnits();
	for (i=0;i<units.size();++i) {
		SubmitModelRecursive(units[i]->GetModel());
	}

	u32 patchesPerSide=pTerrain->GetPatchesPerSide();
	for (j=0; j<patchesPerSide; j++) {
		for (i=0; i<patchesPerSide; i++) {
			CPatch* patch=pTerrain->GetPatch(i,j);
			g_Renderer.Submit(patch);
		}
	}
}

static void MarkUpdateColorRecursive(CModel* model)
{
	model->SetDirty(RENDERDATA_UPDATE_COLOR);

	const std::vector<CModel::Prop>& props = model->GetProps();
	for(uint i = 0; i < props.size(); ++i) {
		MarkUpdateColorRecursive(props[i].m_Model);
	}
}

void CGameView::CheckLightEnv()
{
	if (m_cachedLightEnv == g_LightEnv)
		return;

	m_cachedLightEnv = g_LightEnv;
	CTerrain* pTerrain = m_pWorld->GetTerrain();

	if (!pTerrain)
		return;

	PROFILE("update light env");
	pTerrain->MakeDirty(RENDERDATA_UPDATE_COLOR);

	const std::vector<CUnit*>& units = m_pWorld->GetUnitManager()->GetUnits();
	for(size_t i = 0; i < units.size(); ++i) {
		MarkUpdateColorRecursive(units[i]->GetModel());
	}
}


void CGameView::UnloadResources()
{
	g_ObjMan.UnloadObjects();
	g_TexMan.UnloadTerrainTextures();
	g_Renderer.UnloadAlphaMaps();
	g_Renderer.GetWaterManager()->UnloadWaterTextures();
}


void CGameView::ResetCamera()
{
	// quick hack to return camera home, for screenshots (after alt+tabbing)
	m_ViewCamera.SetProjection (1, 5000, DEGTORAD(20));
	m_ViewCamera.m_Orientation.SetXRotation(DEGTORAD(30));
	m_ViewCamera.m_Orientation.RotateY(DEGTORAD(-45));
	m_ViewCamera.m_Orientation.Translate (100, 150, -100);
}

void CGameView::ResetCameraOrientation()
{

	CVector3D origin = m_ViewCamera.m_Orientation.GetTranslation();
	CVector3D dir = m_ViewCamera.m_Orientation.GetIn();

	CVector3D target = origin + dir * ( ( 50.0f - origin.Y ) / dir.Y );

	target -= CVector3D( -22.474480f, 50.0f, 22.474480f );

	m_ViewCamera.SetProjection (1, 5000, DEGTORAD(20));
	m_ViewCamera.m_Orientation.SetXRotation(DEGTORAD(30));
	m_ViewCamera.m_Orientation.RotateY(DEGTORAD(-45));

	target += CVector3D( 100.0f, 150.0f, -100.0f );

	m_ViewCamera.m_Orientation.Translate( target );
}

void CGameView::RotateAboutTarget()
{
	m_CameraPivot = m_ViewCamera.GetWorldCoordinates();
}

void CGameView::Update(float DeltaTime)
{
	if (!g_app_has_focus)
		return;

	if (m_UnitView)
	{
		m_ViewCamera.m_Orientation.SetYRotation(m_UnitView->m_orientation);
		m_ViewCamera.m_Orientation.Translate(m_UnitViewProp->GetTransform().GetTranslation());
		m_ViewCamera.UpdateFrustum();
		return;
	}

	if (m_UnitAttach)
	{
		CVector3D ToMove = m_UnitAttach->m_position - m_ViewCamera.GetFocus();
		m_ViewCamera.m_Orientation._14 += ToMove.X;
		m_ViewCamera.m_Orientation._34 += ToMove.Z;
		m_ViewCamera.UpdateFrustum();
		return;
	}

	if (m_TrackManager.IsPlaying())
	{
		if(!m_TrackManager.Update(DeltaTime))
		{
			ResetCamera();
		}
		m_ViewCamera.UpdateFrustum();
		return;
	}

	float delta = powf( m_ViewSnapSmoothness, DeltaTime );
	m_ViewCamera.m_Orientation.Translate( m_CameraDelta * ( 1.0f - delta ) );
	m_CameraDelta *= delta;


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
	CVector3D forwards = m_ViewCamera.m_Orientation.GetIn();
	CVector3D rightwards = m_ViewCamera.m_Orientation.GetLeft() * -1.0f; // upwards.Cross(forwards);
	CVector3D upwards( 0.0f, 1.0f, 0.0f );
	// rightwards.Normalize();

	CVector3D forwards_horizontal = forwards;
	forwards_horizontal.Y = 0.0f;
	forwards_horizontal.Normalize();

	if( hotkeys[HOTKEY_CAMERA_ROTATE] || hotkeys[HOTKEY_CAMERA_ROTATE_KEYBOARD] )
	{
		// Ctrl + middle-drag or left-and-right-drag to rotate view

		// Untranslate the camera, so it rotates around the correct point
		CVector3D position = m_ViewCamera.m_Orientation.GetTranslation();
		m_ViewCamera.m_Orientation.Translate(position*-1);

		// Sideways rotation

		float rightways = 0.0f;
		if( hotkeys[HOTKEY_CAMERA_ROTATE] )
			rightways = (float)mouse_dx * m_ViewRotateSensitivity;
		if( hotkeys[HOTKEY_CAMERA_ROTATE_KEYBOARD] )
		{
			if( hotkeys[HOTKEY_CAMERA_LEFT] )
				rightways -= m_ViewRotateSensitivityKeyboard * DeltaTime;
			if( hotkeys[HOTKEY_CAMERA_RIGHT] )
				rightways += m_ViewRotateSensitivityKeyboard * DeltaTime;
		}

		m_ViewCamera.m_Orientation.RotateY( rightways );

		// Up/down rotation

		float upways = 0.0f;
		if( hotkeys[HOTKEY_CAMERA_ROTATE] )
			upways = (float)mouse_dy * m_ViewRotateSensitivity;
		if( hotkeys[HOTKEY_CAMERA_ROTATE_KEYBOARD] )
		{
			if( hotkeys[HOTKEY_CAMERA_UP] )
				upways -= m_ViewRotateSensitivityKeyboard * DeltaTime;
			if( hotkeys[HOTKEY_CAMERA_DOWN] )
				upways += m_ViewRotateSensitivityKeyboard * DeltaTime;
		}

		CQuaternion temp;
		temp.FromAxisAngle(rightwards, upways);

		m_ViewCamera.m_Orientation.Rotate(temp);

		// Retranslate back to the right position
		m_ViewCamera.m_Orientation.Translate(position);

	}
	else if( hotkeys[HOTKEY_CAMERA_ROTATE_ABOUT_TARGET] )
	{
		CVector3D origin = m_ViewCamera.m_Orientation.GetTranslation();
		CVector3D delta = origin - m_CameraPivot;

		CQuaternion rotateH, rotateV; CMatrix3D rotateM;

		// Sideways rotation

		float rightways = (float)mouse_dx * m_ViewRotateAboutTargetSensitivity;

		rotateH.FromAxisAngle( upwards, rightways );

		// Up/down rotation

		float upways = (float)mouse_dy * m_ViewRotateAboutTargetSensitivity;
		rotateV.FromAxisAngle( rightwards, upways );

		rotateH *= rotateV;
		rotateH.ToMatrix( rotateM );

		delta = rotateM.Rotate( delta );

		// Lock the inclination to a rather arbitrary values (for the sake of graphical decency)

		float scan = sqrt( delta.X * delta.X + delta.Z * delta.Z ) / delta.Y;
		if( ( scan >= 0.5f ) )
		{
			// Move the camera to the origin (in preparation for rotation )
			m_ViewCamera.m_Orientation.Translate( origin * -1.0f );

			m_ViewCamera.m_Orientation.Rotate( rotateH );

			// Move the camera back to where it belongs
			m_ViewCamera.m_Orientation.Translate( m_CameraPivot + delta );
		}

	}
	else if( hotkeys[HOTKEY_CAMERA_ROTATE_ABOUT_TARGET_KEYBOARD] )
	{
		// Split up because the keyboard controls use the centre of the screen, not the mouse position.
		CVector3D origin = m_ViewCamera.m_Orientation.GetTranslation();
		CVector3D pivot = m_ViewCamera.GetFocus();
		CVector3D delta = origin - pivot;

		CQuaternion rotateH, rotateV; CMatrix3D rotateM;

		// Sideways rotation

		float rightways = 0.0f;
		if( hotkeys[HOTKEY_CAMERA_LEFT] )
			rightways -= m_ViewRotateAboutTargetSensitivityKeyboard * DeltaTime;
		if( hotkeys[HOTKEY_CAMERA_RIGHT] )
			rightways += m_ViewRotateAboutTargetSensitivityKeyboard * DeltaTime;

		rotateH.FromAxisAngle( upwards, rightways );

		// Up/down rotation

		float upways = 0.0f;
		if( hotkeys[HOTKEY_CAMERA_UP] )
			upways -= m_ViewRotateAboutTargetSensitivityKeyboard * DeltaTime;
		if( hotkeys[HOTKEY_CAMERA_DOWN] )
			upways += m_ViewRotateAboutTargetSensitivityKeyboard * DeltaTime;

		rotateV.FromAxisAngle( rightwards, upways );

		rotateH *= rotateV;
		rotateH.ToMatrix( rotateM );

		delta = rotateM.Rotate( delta );

		// Lock the inclination to a rather arbitrary values (for the sake of graphical decency)

		float scan = sqrt( delta.X * delta.X + delta.Z * delta.Z ) / delta.Y;
		if( ( scan >= 0.5f ) )
		{
			// Move the camera to the origin (in preparation for rotation )
			m_ViewCamera.m_Orientation.Translate( origin * -1.0f );

			m_ViewCamera.m_Orientation.Rotate( rotateH );

			// Move the camera back to where it belongs
			m_ViewCamera.m_Orientation.Translate( pivot + delta );
		}

	}
	else if( hotkeys[HOTKEY_CAMERA_PAN] )
	{
		// Middle-drag to pan
		//keep camera in bounds
			CameraLock(rightwards * (m_ViewDragSensitivity * mouse_dx));
			CameraLock(forwards_horizontal * (-m_ViewDragSensitivity * mouse_dy));
	}

	// Mouse movement
	if( !hotkeys[HOTKEY_CAMERA_ROTATE] && !hotkeys[HOTKEY_CAMERA_ROTATE_ABOUT_TARGET] )
	{
		if (g_mouse_x >= g_xres-2 && g_mouse_x < g_xres)
			CameraLock(rightwards * (m_ViewScrollSpeed * DeltaTime));
		else if (g_mouse_x <= 3 && g_mouse_x >= 0)
			CameraLock(-rightwards * (m_ViewScrollSpeed * DeltaTime));

		if (g_mouse_y >= g_yres-2 && g_mouse_y < g_yres)
			CameraLock(-forwards_horizontal * (m_ViewScrollSpeed * DeltaTime));
		else if (g_mouse_y <= 3 && g_mouse_y >= 0)
			CameraLock(forwards_horizontal * (m_ViewScrollSpeed * DeltaTime));

	}

	// Keyboard movement (added to mouse movement, so you can go faster if you want)

	if( hotkeys[HOTKEY_CAMERA_PAN_KEYBOARD] )
	{
		if( hotkeys[HOTKEY_CAMERA_RIGHT] )
			CameraLock(rightwards * (m_ViewScrollSpeed * DeltaTime));
		if( hotkeys[HOTKEY_CAMERA_LEFT] )
			CameraLock(-rightwards * (m_ViewScrollSpeed * DeltaTime));

		if( hotkeys[HOTKEY_CAMERA_DOWN] )
			CameraLock(-forwards_horizontal * (m_ViewScrollSpeed * DeltaTime));
		if( hotkeys[HOTKEY_CAMERA_UP] )
			CameraLock(forwards_horizontal * (m_ViewScrollSpeed * DeltaTime));

	}

	//Temporary hack for cinematic interface
	if ( hotkeys[HOTKEY_CAMERA_CINEMA_ADD] )
	{
		std::vector<CCinemaPath>::iterator it = m_TestTrack.m_Paths.begin();
		if (it->GetNodeCount() == 0)
			m_TestTrack.m_StartRotation = m_ViewCamera.m_Orientation.GetRotation().m_V;

		it->AddNode(m_ViewCamera.m_Orientation.GetTranslation(), 1.5f);
		it->UpdateSpline();
	}
	if ( hotkeys[HOTKEY_CAMERA_CINEMA_DELETE] )
		m_TestTrack.m_Paths.front().RemoveNode( m_TestTrack.m_Paths.front().GetNodeCount() - 1 );
		m_TestTrack.m_Paths.front().UpdateSpline();

	if ( hotkeys[HOTKEY_CAMERA_CINEMA_DELETE_ALL] )
	{
		for (int i=m_TestTrack.m_Paths.front().GetNodeCount() - 1; i >= 0; i--)
		{
			m_TestTrack.m_Paths.front().RemoveNode( i );
		}
	}
	if ( hotkeys[HOTKEY_CAMERA_CINEMA_QUEUE] )
		m_TrackManager.QueueTrack("test", false);

	// Smoothed zooming (move a certain percentage towards the desired zoom distance every frame)
	// Note that scroll wheel zooming is event-based and handled in game_view_handler

	if( hotkeys[HOTKEY_CAMERA_ZOOM_IN] )
		m_ZoomDelta += m_ViewZoomSensitivity*DeltaTime;
	else if( hotkeys[HOTKEY_CAMERA_ZOOM_OUT] )
		m_ZoomDelta -= m_ViewZoomSensitivity*DeltaTime;

	if (fabsf(m_ZoomDelta) > 0.1f) // use a fairly high limit to avoid nasty flickering when zooming
	{
		float zoom_proportion = powf(m_ViewZoomSmoothness, DeltaTime);
		CameraLock(forwards * (m_ZoomDelta * (1.0f-zoom_proportion)), false);
		m_ZoomDelta *= zoom_proportion;
	}

	m_ViewCamera.UpdateFrustum ();
}

void CGameView::ToUnitView(CEntity* target, CModel* prop)
{
	if( !target )
	{
		//prevent previous zooming
		m_ZoomDelta = 0.0f;
		ResetCamera();
		SetCameraTarget( m_UnitView->m_position );
	}
	m_UnitView = target;
	m_UnitViewProp = prop;

}
void CGameView::PushCameraTarget( const CVector3D& target )
{
	// Save the current position
	m_CameraTargets.push_back( m_ViewCamera.m_Orientation.GetTranslation() );
	// And set the camera
	SetCameraTarget( target );
}

void CGameView::SetCameraTarget( const CVector3D& target )
{
	// Maintain the same orientation and level of zoom, if we can
	// (do this by working out the point the camera is looking at, saving
	//  the difference between that position and the camera point, and restoring
	//  that difference to our new target)

	CVector3D CurrentTarget = m_ViewCamera.GetFocus();
	m_CameraDelta = target - CurrentTarget;
}

void CGameView::PopCameraTarget()
{
	m_CameraDelta = m_CameraTargets.back() - m_ViewCamera.m_Orientation.GetTranslation();
	m_CameraTargets.pop_back();
}

InReaction game_view_handler(const SDL_Event* ev)
{
	// put any events that must be processed even if inactive here

	if(!g_app_has_focus || !g_Game)
		return IN_PASS;

	CGameView *pView=g_Game->GetView();

	return pView->HandleEvent(ev);
}

InReaction CGameView::HandleEvent(const SDL_Event* ev)
{
	switch(ev->type)
	{

	case SDL_HOTKEYDOWN:
		switch(ev->user.code)
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
			m_ZoomDelta += m_ViewZoomSensitivityWheel;
			return( IN_HANDLED );

		case HOTKEY_CAMERA_ZOOM_WHEEL_OUT:
			m_ZoomDelta -= m_ViewZoomSensitivityWheel;
			return( IN_HANDLED );

		default:

			if( ( ev->user.code >= HOTKEY_CAMERA_BOOKMARK_0 ) && ( ev->user.code <= HOTKEY_CAMERA_BOOKMARK_9 ) )
			{
				// The above test limits it to 10 bookmarks, so don't worry about overflowing
				i8 id = (i8)( ev->user.code - HOTKEY_CAMERA_BOOKMARK_0 );

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
		switch( ev->user.code )
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

bool CGameView::JSI_StartCustomSelection(
	JSContext* UNUSED(context), uint UNUSED(argc), jsval* UNUSED(argv))
{
	StartCustomSelection();
	return true;
}

bool CGameView::JSI_EndCustomSelection(
	JSContext* UNUSED(context), uint UNUSED(argc), jsval* UNUSED(argv))
{
	ResetInteraction();
	return true;
}
