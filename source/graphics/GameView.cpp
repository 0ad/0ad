#include "precompiled.h"

#include "Terrain.h"
#include "Renderer.h"
#include "GameView.h"
#include "Game.h"
#include "Camera.h"

#include "Matrix3D.h"
#include "Renderer.h"
#include "Terrain.h"
#include "LightEnv.h"
#include "HFTracer.h"
#include "TextureManager.h"
#include "ObjectManager.h"
#include "Prometheus.h"
#include "Hotkey.h"
#include "ConfigDB.h"

#include "sdl.h"
#include "input.h"
#include "lib.h"

extern int g_xres, g_yres;
extern bool g_active;

extern CLightEnv g_LightEnv;

CGameView::CGameView(CGame *pGame):
	m_pGame(pGame),
	m_pWorld(pGame->GetWorld()),
	m_Camera(),
	m_ViewScrollSpeed(60),
	m_ViewRotateSensitivity(0.002f),
	m_ViewRotateAboutTargetSensitivity(0.010f),
	m_ViewDragSensitivity(0.5f),
	m_ViewZoomSensitivityWheel(16.0f),
	m_ViewZoomSensitivity(256.0f),
	m_ViewZoomSmoothness(0.02f),
	m_ViewSnapSmoothness(0.02f),
	m_CameraPivot(),
	m_CameraDelta()//,
//	m_CameraZoom(10)
{
	InitResources();
}

void CGameView::Initialize(CGameAttributes *pAttribs)
{
	SViewPort vp;
	vp.m_X=0;
	vp.m_Y=0;
	vp.m_Width=g_xres;
	vp.m_Height=g_yres;
	m_Camera.SetViewPort(&vp);

	CConfigValue* cfg;
	
#define getViewParameter( name, value ) STMT( \
	cfg = g_ConfigDB.GetValue( CFG_SYSTEM, name );\
	if( cfg ) cfg->GetFloat( value ); )

	getViewParameter( "view.scroll.speed", m_ViewScrollSpeed );
	getViewParameter( "view.rotate.speed", m_ViewRotateSensitivity );
	getViewParameter( "view.rotate.abouttarget.speed", m_ViewRotateAboutTargetSensitivity );
	getViewParameter( "view.drag.speed", m_ViewDragSensitivity );
	getViewParameter( "view.zoom.speed", m_ViewZoomSensitivity );
	getViewParameter( "view.zoom.wheel.speed", m_ViewZoomSensitivityWheel );
	getViewParameter( "view.zoom.smoothness", m_ViewZoomSmoothness );
	getViewParameter( "view.snap.smoothness", m_ViewSnapSmoothness );
	
	if( ( m_ViewZoomSmoothness < 0.0f ) || ( m_ViewZoomSmoothness > 1.0f ) ) m_ViewZoomSmoothness = 0.02f;
	if( ( m_ViewSnapSmoothness < 0.0f ) || ( m_ViewSnapSmoothness > 1.0f ) ) m_ViewSnapSmoothness = 0.02f;

#undef getViewParameter

	// setup default lighting environment
	g_LightEnv.m_SunColor=RGBColor(1,1,1);
	g_LightEnv.m_Rotation=DEGTORAD(270);
	g_LightEnv.m_Elevation=DEGTORAD(45);
	g_LightEnv.m_TerrainAmbientColor=RGBColor(0,0,0);
	g_LightEnv.m_UnitsAmbientColor=RGBColor(0.4f,0.4f,0.4f);
	g_Renderer.SetLightEnv(&g_LightEnv);

	m_Camera.SetProjection (1, 5000, DEGTORAD(20));
	m_Camera.m_Orientation.SetXRotation(DEGTORAD(30));
	m_Camera.m_Orientation.RotateY(DEGTORAD(-45));
	m_Camera.m_Orientation.Translate (100, 150, -100);
}

void CGameView::Render()
{
	g_Renderer.SetCamera(m_Camera);
	MICROLOG(L"render terrain");
	RenderTerrain(m_pWorld->GetTerrain());
	MICROLOG(L"render models");
	RenderModels(m_pWorld->GetUnitManager());
}

void CGameView::RenderTerrain(CTerrain *pTerrain)
{
	CFrustum frustum=m_Camera.GetFrustum();
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

void CGameView::RenderModels(CUnitManager *pUnitMan)
{
	CFrustum frustum=m_Camera.GetFrustum();

	const std::vector<CUnit*>& units=pUnitMan->GetUnits();
	for (uint i=0;i<units.size();++i) {
		if (frustum.IsBoxVisible(CVector3D(0,0,0),units[i]->GetModel()->GetBounds())) {
			SubmitModelRecursive(units[i]->GetModel());
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

	g_Renderer.SetCamera(m_Camera);

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

void CGameView::InitResources()
{
	g_TexMan.LoadTerrainTextures();
	g_ObjMan.LoadObjects();

	const char* fns[CRenderer::NumAlphaMaps] = {
		"art/textures/terrain/alphamaps/special/blendcircle.png",
		"art/textures/terrain/alphamaps/special/blendlshape.png",
		"art/textures/terrain/alphamaps/special/blendedge.png",
		"art/textures/terrain/alphamaps/special/blendedgecorner.png",
		"art/textures/terrain/alphamaps/special/blendedgetwocorners.png",
		"art/textures/terrain/alphamaps/special/blendfourcorners.png",
		"art/textures/terrain/alphamaps/special/blendtwooppositecorners.png",
		"art/textures/terrain/alphamaps/special/blendlshapecorner.png",
		"art/textures/terrain/alphamaps/special/blendtwocorners.png",
		"art/textures/terrain/alphamaps/special/blendcorner.png",
		"art/textures/terrain/alphamaps/special/blendtwoedges.png",
		"art/textures/terrain/alphamaps/special/blendthreecorners.png",
		"art/textures/terrain/alphamaps/special/blendushape.png",
		"art/textures/terrain/alphamaps/special/blendbad.png"
	};

	g_Renderer.LoadAlphaMaps(fns);
}

void CGameView::ResetCamera()
{
	// quick hack to return camera home, for screenshots (after alt+tabbing)
	m_Camera.SetProjection (1, 5000, DEGTORAD(20));
	m_Camera.m_Orientation.SetXRotation(DEGTORAD(30));
	m_Camera.m_Orientation.RotateY(DEGTORAD(-45));
	m_Camera.m_Orientation.Translate (100, 150, -100);
}

void CGameView::RotateAboutTarget()
{
	CTerrain *pTerrain=m_pWorld->GetTerrain();

	int x, z;
	CHFTracer tracer( pTerrain );
	CVector3D origin, dir;
	origin = m_Camera.m_Orientation.GetTranslation();
	dir = m_Camera.m_Orientation.GetIn();
	m_Camera.BuildCameraRay( origin, dir );

	if( !tracer.RayIntersect( origin, dir, x, z, m_CameraPivot ) )
		m_CameraPivot = origin - dir * ( origin.Y / dir.Y );
}

void CGameView::Update(float DeltaTime)
{
	if (!g_active)
		return;

	float delta = powf( m_ViewSnapSmoothness, DeltaTime );
	m_Camera.m_Orientation.Translate( m_CameraDelta * ( 1.0f - delta ) );
	m_CameraDelta *= delta;

#define CAMERASTYLE 2 // 0 = old style, 1 = relatively new style, 2 = newest style

#if CAMERASTYLE == 2

	// This could be rewritten much more reliably, so it doesn't e.g. accidentally tilt
	// the camera, assuming we know exactly what limits the camera should have.


	// Calculate mouse movement
	static int mouse_last_x = 0;
	static int mouse_last_y = 0;
	int mouse_dx = mouse_x - mouse_last_x;
	int mouse_dy = mouse_y - mouse_last_y;
	mouse_last_x = mouse_x;
	mouse_last_y = mouse_y;

	// Miscellaneous vectors
	CVector3D forwards = m_Camera.m_Orientation.GetIn();
	CVector3D rightwards = m_Camera.m_Orientation.GetLeft() * -1.0f; // upwards.Cross(forwards);
	CVector3D upwards( 0.0f, 1.0f, 0.0f );
	// rightwards.Normalize();
	
	CVector3D forwards_horizontal = forwards;
	forwards_horizontal.Y = 0.0f;
	forwards_horizontal.Normalize();

	/*
	if ((mouseButtons[SDL_BUTTON_MIDDLE] && (keys[SDLK_LCTRL] || keys[SDLK_RCTRL]))
	 || (mouseButtons[SDL_BUTTON_LEFT] && mouseButtons[SDL_BUTTON_RIGHT]) )
	*/
	if( hotkeys[HOTKEY_CAMERA_ROTATE] )
	{
		// Ctrl + middle-drag or left-and-right-drag to rotate view

		// Untranslate the camera, so it rotates around the correct point
		CVector3D position = m_Camera.m_Orientation.GetTranslation();
		m_Camera.m_Orientation.Translate(position*-1);

		// Sideways rotation
		m_Camera.m_Orientation.RotateY(m_ViewRotateSensitivity * (float)(mouse_dx));

		// Up/down rotation
		CQuaternion temp;
		temp.FromAxisAngle(rightwards, m_ViewRotateSensitivity * (float)(mouse_dy));
		m_Camera.m_Orientation.Rotate(temp);

		// Retranslate back to the right position
		m_Camera.m_Orientation.Translate(position);

	}
	else if( hotkeys[HOTKEY_CAMERA_ROTATE_ABOUT_TARGET] )
	{
		CVector3D origin = m_Camera.m_Orientation.GetTranslation();
		CVector3D delta = origin - m_CameraPivot;
		
		CQuaternion rotateH, rotateV; CMatrix3D rotateM;

		// Side-to-side rotation
		rotateH.FromAxisAngle( upwards, m_ViewRotateAboutTargetSensitivity * (float)mouse_dx );

		// Up-down rotation
		rotateV.FromAxisAngle( rightwards, m_ViewRotateAboutTargetSensitivity * (float)mouse_dy );

		rotateH *= rotateV;
		rotateH.ToMatrix( rotateM );

		delta = rotateM.Rotate( delta );

		// Lock the inclination to a rather arbitrary values (for the sake of graphical decency)

		float scan = sqrt( delta.X * delta.X + delta.Z * delta.Z ) / delta.Y;
		if( ( scan >= 0.5f ) ) 
		{
			// Move the camera to the origin (in preparation for rotation )
			m_Camera.m_Orientation.Translate( origin * -1.0f );

			m_Camera.m_Orientation.Rotate( rotateH );

			// Move the camera back to where it belongs
			m_Camera.m_Orientation.Translate( m_CameraPivot + delta );
		}
		
	}
	else if( hotkeys[HOTKEY_CAMERA_PAN] )
	{
		// Middle-drag to pan
		m_Camera.m_Orientation.Translate(rightwards * (m_ViewDragSensitivity * mouse_dx));
		m_Camera.m_Orientation.Translate(forwards_horizontal * (-m_ViewDragSensitivity * mouse_dy));
	}

	// Mouse movement

	if( !hotkeys[HOTKEY_CAMERA_ROTATE] && !hotkeys[HOTKEY_CAMERA_ROTATE_ABOUT_TARGET] )
	{
		if (mouse_x >= g_xres-2)
			m_Camera.m_Orientation.Translate(rightwards * (m_ViewScrollSpeed * DeltaTime));
		else if (mouse_x <= 3)
			m_Camera.m_Orientation.Translate(-rightwards * (m_ViewScrollSpeed * DeltaTime));

		if (mouse_y >= g_yres-2)
			m_Camera.m_Orientation.Translate(-forwards_horizontal * (m_ViewScrollSpeed * DeltaTime));
		else if (mouse_y <= 3)
			m_Camera.m_Orientation.Translate(forwards_horizontal * (m_ViewScrollSpeed * DeltaTime));
	}


	// Keyboard movement (added to mouse movement, so you can go faster if you want)

	if( hotkeys[HOTKEY_CAMERA_PAN_RIGHT] )
		m_Camera.m_Orientation.Translate(rightwards * (m_ViewScrollSpeed * DeltaTime));
	if( hotkeys[HOTKEY_CAMERA_PAN_LEFT] )
		m_Camera.m_Orientation.Translate(-rightwards * (m_ViewScrollSpeed * DeltaTime));

	if( hotkeys[HOTKEY_CAMERA_PAN_BACKWARD] )
		m_Camera.m_Orientation.Translate(-forwards_horizontal * (m_ViewScrollSpeed * DeltaTime));
	if( hotkeys[HOTKEY_CAMERA_PAN_FORWARD] )
		m_Camera.m_Orientation.Translate(forwards_horizontal * (m_ViewScrollSpeed * DeltaTime));

	// Smoothed zooming (move a certain percentage towards the desired zoom distance every frame)

	static float zoom_delta = 0.0f;

	if( hotkeys[HOTKEY_CAMERA_ZOOM_WHEEL_IN] )
		zoom_delta += m_ViewZoomSensitivityWheel;
	else if( hotkeys[HOTKEY_CAMERA_ZOOM_WHEEL_OUT] )
		zoom_delta -= m_ViewZoomSensitivityWheel;

	if( hotkeys[HOTKEY_CAMERA_ZOOM_IN] )
		zoom_delta += m_ViewZoomSensitivity*DeltaTime;
	else if( hotkeys[HOTKEY_CAMERA_ZOOM_OUT] )
		zoom_delta -= m_ViewZoomSensitivity*DeltaTime;

	if (zoom_delta)
	{
		float zoom_proportion = powf(m_ViewZoomSmoothness, DeltaTime);
		m_Camera.m_Orientation.Translate(forwards * (zoom_delta * (1.0f-zoom_proportion)));
		zoom_delta *= zoom_proportion;
	}


#elif CAMERASTYLE == 1

	// Remember previous mouse position, to calculate changes
	static mouse_last_x = 0;
	static mouse_last_y = 0;

	// Miscellaneous vectors
	CVector3D forwards = m_Camera.m_Orientation.GetIn();
	CVector3D upwards (0.0f, 1.0f, 0.0f);
	CVector3D rightwards = upwards.Cross(forwards);

	// Click and drag to look around
	if (mouseButtons[0])
	{
		// Untranslate the camera, so it rotates around the correct point
		CVector3D position = m_Camera.m_Orientation.GetTranslation();
		m_Camera.m_Orientation.Translate(position*-1);

		// Sideways rotation
		m_Camera.m_Orientation.RotateY(m_ViewRotateSpeed*(float)(mouse_x-mouse_last_x));

		// Up/down rotation
		CQuaternion temp;
		temp.FromAxisAngle(rightwards, m_ViewRotateSpeed*(float)(mouse_y-mouse_last_y));
		m_Camera.m_Orientation.Rotate(temp);

		// Retranslate back to the right position
		m_Camera.m_Orientation.Translate(position);
	}
	mouse_last_x = mouse_x;
	mouse_last_y = mouse_y;

	// Calculate the necessary vectors for movement

	rightwards.Normalize();
	CVector3D forwards_horizontal = upwards.Cross(rightwards);
	forwards_horizontal.Normalize();

	// Move when desirable

	if (mouse_x >= g_xres-2)
		m_Camera.m_Orientation.Translate(rightwards);
	else if (mouse_x <= 3)
		m_Camera.m_Orientation.Translate(-rightwards);

	if (mouse_y >= g_yres-2)
		m_Camera.m_Orientation.Translate(forwards_horizontal);
	else if (mouse_y <= 3)
		m_Camera.m_Orientation.Translate(-forwards_horizontal);

	// Smoothed height-changing (move a certain percentage towards the desired height every frame)

	static float height_delta = 0.0f;

	if (mouseButtons[SDL_BUTTON_WHEELUP])
		height_delta -= 4.0f;
	else if (mouseButtons[SDL_BUTTON_WHEELDOWN])
		height_delta += 4.0f;

	const float height_speed = 0.2f;
	m_Camera.m_Orientation.Translate(0.0f, height_delta*height_speed, 0.0f);
	height_delta *= (1.0f - height_speed);

#else // CAMERASTYLE == 0

	const float dx = m_ViewScrollSpeed * DeltaTime;
	const CVector3D Right(dx,0, dx);
	const CVector3D Up   (dx,0,-dx);

	if (mouse_x >= g_xres-2)
		m_Camera.m_Orientation.Translate(Right);
	if (mouse_x <= 3)
		m_Camera.m_Orientation.Translate(Right*-1);

	if (mouse_y >= g_yres-2)
		m_Camera.m_Orientation.Translate(Up);
	if (mouse_y <= 3)
		m_Camera.m_Orientation.Translate(Up*-1);

	/*
	janwas: grr, plotted the zoom vector on paper twice, but it appears
	to be completely wrong. sticking with the FOV hack for now.
	if anyone sees what's wrong, or knows how to correctly implement zoom,
	please put this code out of its misery :)
	*/
	// RC - added ScEd style zoom in and out (actually moving camera, rather than fudging fov)	

	float dir=0;
	if (mouseButtons[SDL_BUTTON_WHEELUP]) dir=-1;
	else if (mouseButtons[SDL_BUTTON_WHEELDOWN]) dir=1;	

	float factor=dir*dir;
	if (factor) {
		if (dir<0) factor=-factor;
		CVector3D forward=m_Camera.m_Orientation.GetIn();

		// check we're not going to zoom into the terrain, or too far out into space
		float h=m_Camera.m_Orientation.GetTranslation().Y+forward.Y*factor*m_Camera.Zoom;
		float minh=65536*HEIGHT_SCALE*1.05f;
		
		if (h<minh || h>1500) {
			// yup, we will; don't move anywhere (do clamped move instead, at some point)
		} else {
			// do a full move
			m_Camera.Zoom-=(factor)*0.1f;
			if (m_Camera.Zoom<0.01f) m_Camera.Zoom=0.01f;
			m_Camera.m_Orientation.Translate(forward*(factor*m_Camera.Zoom));
		}
	}
#endif // CAMERASTYLE

	m_Camera.UpdateFrustum ();
}

void CGameView::PushCameraTarget( const CVector3D& target )
{
	// Save the current position
	m_CameraTargets.push_back( m_Camera.m_Orientation.GetTranslation() );
	// And set the camera
	SetCameraTarget( target );
}

void CGameView::SetCameraTarget( const CVector3D& target )
{
	// Maintain the same orientation and level of zoom, if we can
	// (do this by working out the point the camera is looking at, saving
	//  the difference beteen that position and the camera point, and restoring
	//  that difference to our new target)

	CHFTracer tracer( m_pWorld->GetTerrain() );
	int x, z;
	CVector3D origin, dir, currentTarget;
	origin = m_Camera.m_Orientation.GetTranslation();
	dir = m_Camera.m_Orientation.GetIn();
	if( tracer.RayIntersect( origin, dir, x, z, currentTarget ) )
	{
		m_CameraDelta = target - currentTarget;
	}
	else
		m_CameraDelta = ( target - dir * 160.0f ) - origin;
}

void CGameView::PopCameraTarget()
{
	m_CameraDelta = m_CameraTargets.back() - m_Camera.m_Orientation.GetTranslation();
	m_CameraTargets.pop_back();
}

int game_view_handler(const SDL_Event* ev)
{
	CGameView *pView=g_Game->GetView();
	// put any events that must be processed even if inactive here

	if(!g_active)
		return EV_PASS;

	switch(ev->type)
	{

	case SDL_HOTKEYDOWN:
		switch(ev->user.code)
		{
		case HOTKEY_WIREFRAME:
			if (g_Renderer.GetTerrainRenderMode()==WIREFRAME) {
				g_Renderer.SetTerrainRenderMode(SOLID);
			} else {
				g_Renderer.SetTerrainRenderMode(WIREFRAME);
			}
			return( EV_HANDLED );

		case HOTKEY_CAMERA_RESET:
			pView->ResetCamera();
			return( EV_HANDLED );

		case HOTKEY_CAMERA_ROTATE_ABOUT_TARGET:
			pView->RotateAboutTarget();
			return( EV_HANDLED );

		}

	}

	return EV_PASS;
}
