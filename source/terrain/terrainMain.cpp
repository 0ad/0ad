#include "precompiled.h"

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
#include "res/tex.h"
#include "detect.h"
#include "input.h"
#include "lib.h"

void InitScene ();
void InitResources ();

extern bool keys[512];	// SDL also defines non-ascii keys; 512 should be enough
extern bool mouseButtons[5];
extern int mouse_x, mouse_y;
extern bool g_active;


CMatrix3D			g_WorldMat;
CCamera				g_Camera;
CLightEnv			g_LightEnv;


float g_CameraZoom = 10;

std::vector<CVector3D> cameraTargets;
CVector3D cameraBookmarks[10];
bool bookmarkInUse[10] = { false, false, false, false, false, false, false, false, false, false };
u8 currentBookmark = 255;
CVector3D cameraDelta;
CVector3D cameraPivot;

// These were 'const'; loaded from config now.

float ViewScrollSpeed = 60;
float ViewRotateSensitivity = 0.002f;
float ViewRotateAboutTargetSensitivity = 0.010f;
float ViewDragSensitivity = 0.5f;
float ViewZoomSensitivityWheel = 16.0f;
float ViewZoomSensitivity = 256.0f;
float ViewZoomSmoothness = 0.02f; // 0.0 = instantaneous zooming, 1.0 = so slow it never moves
float ViewSnapSmoothness = 0.02f; // Just the same.

float ViewFOV;

extern int g_xres, g_yres;

void terr_init()
{
	SViewPort vp;
	vp.m_X=0;
	vp.m_Y=0;
	vp.m_Width=g_xres;
	vp.m_Height=g_yres;
	g_Camera.SetViewPort(&vp);

	CConfigValue* cfg;
	
#define getViewParameter( name, value ) STMT( \
	cfg = g_ConfigDB.GetValue( CFG_SYSTEM, name );\
	if( cfg ) cfg->GetFloat( value ); )

	getViewParameter( "view.scroll.speed", ViewScrollSpeed );
	getViewParameter( "view.rotate.speed", ViewRotateSensitivity );
	getViewParameter( "view.rotate.abouttarget.speed", ViewRotateAboutTargetSensitivity );
	getViewParameter( "view.drag.speed", ViewDragSensitivity );
	getViewParameter( "view.zoom.speed", ViewZoomSensitivity );
	getViewParameter( "view.zoom.wheel.speed", ViewZoomSensitivityWheel );
	getViewParameter( "view.zoom.smoothness", ViewZoomSmoothness );
	getViewParameter( "view.snap.smoothness", ViewSnapSmoothness );
	
	if( ( ViewZoomSmoothness < 0.0f ) || ( ViewZoomSmoothness > 1.0f ) ) ViewZoomSmoothness = 0.02f;
	if( ( ViewSnapSmoothness < 0.0f ) || ( ViewSnapSmoothness > 1.0f ) ) ViewSnapSmoothness = 0.02f;

#undef getViewParameter

	InitResources ();
	InitScene ();
}


static void move_camera(float DeltaTime)
{
	float delta = powf( ViewSnapSmoothness, DeltaTime );
	g_Camera.m_Orientation.Translate( cameraDelta * ( 1.0f - delta ) );
	cameraDelta *= delta;

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
	CVector3D forwards = g_Camera.m_Orientation.GetIn();
	CVector3D rightwards = g_Camera.m_Orientation.GetLeft() * -1.0f; // upwards.Cross(forwards);
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
		CVector3D position = g_Camera.m_Orientation.GetTranslation();
		g_Camera.m_Orientation.Translate(position*-1);

		// Sideways rotation
		g_Camera.m_Orientation.RotateY(ViewRotateSensitivity * (float)(mouse_dx));

		// Up/down rotation
		CQuaternion temp;
		temp.FromAxisAngle(rightwards, ViewRotateSensitivity * (float)(mouse_dy));
		g_Camera.m_Orientation.Rotate(temp);

		// Retranslate back to the right position
		g_Camera.m_Orientation.Translate(position);

	}
	else if( hotkeys[HOTKEY_CAMERA_ROTATE_ABOUT_TARGET] )
	{
		CVector3D origin = g_Camera.m_Orientation.GetTranslation();
		CVector3D delta = origin - cameraPivot;
		
		CQuaternion rotateH, rotateV; CMatrix3D rotateM;

		// Side-to-side rotation
		rotateH.FromAxisAngle( upwards, ViewRotateAboutTargetSensitivity * (float)mouse_dx );

		// Up-down rotation
		rotateV.FromAxisAngle( rightwards, ViewRotateAboutTargetSensitivity * (float)mouse_dy );

		rotateH *= rotateV;
		rotateH.ToMatrix( rotateM );

		delta = rotateM.Rotate( delta );

		// Lock the inclination to a rather arbitrary values (for the sake of graphical decency)

		float scan = sqrt( delta.X * delta.X + delta.Z * delta.Z ) / delta.Y;
		if( ( scan >= 0.5f ) ) 
		{
			// Move the camera to the origin (in preparation for rotation )
			g_Camera.m_Orientation.Translate( origin * -1.0f );

			g_Camera.m_Orientation.Rotate( rotateH );

			// Move the camera back to where it belongs
			g_Camera.m_Orientation.Translate( cameraPivot + delta );
		}
		
	}
	else if( hotkeys[HOTKEY_CAMERA_PAN] )
	{
		// Middle-drag to pan
		g_Camera.m_Orientation.Translate(rightwards * (ViewDragSensitivity * mouse_dx));
		g_Camera.m_Orientation.Translate(forwards_horizontal * (-ViewDragSensitivity * mouse_dy));
	}

	// Mouse movement

	if( !hotkeys[HOTKEY_CAMERA_ROTATE] && !hotkeys[HOTKEY_CAMERA_ROTATE_ABOUT_TARGET] )
	{
		if (mouse_x >= g_xres-2)
			g_Camera.m_Orientation.Translate(rightwards * (ViewScrollSpeed * DeltaTime));
		else if (mouse_x <= 3)
			g_Camera.m_Orientation.Translate(-rightwards * (ViewScrollSpeed * DeltaTime));

		if (mouse_y >= g_yres-2)
			g_Camera.m_Orientation.Translate(-forwards_horizontal * (ViewScrollSpeed * DeltaTime));
		else if (mouse_y <= 3)
			g_Camera.m_Orientation.Translate(forwards_horizontal * (ViewScrollSpeed * DeltaTime));
	}


	// Keyboard movement (added to mouse movement, so you can go faster if you want)

	if( hotkeys[HOTKEY_CAMERA_PAN_RIGHT] )
		g_Camera.m_Orientation.Translate(rightwards * (ViewScrollSpeed * DeltaTime));
	if( hotkeys[HOTKEY_CAMERA_PAN_LEFT] )
		g_Camera.m_Orientation.Translate(-rightwards * (ViewScrollSpeed * DeltaTime));

	if( hotkeys[HOTKEY_CAMERA_PAN_BACKWARD] )
		g_Camera.m_Orientation.Translate(-forwards_horizontal * (ViewScrollSpeed * DeltaTime));
	if( hotkeys[HOTKEY_CAMERA_PAN_FORWARD] )
		g_Camera.m_Orientation.Translate(forwards_horizontal * (ViewScrollSpeed * DeltaTime));

	// Smoothed zooming (move a certain percentage towards the desired zoom distance every frame)

	static float zoom_delta = 0.0f;

	if( hotkeys[HOTKEY_CAMERA_ZOOM_WHEEL_IN] )
		zoom_delta += ViewZoomSensitivityWheel;
	else if( hotkeys[HOTKEY_CAMERA_ZOOM_WHEEL_OUT] )
		zoom_delta -= ViewZoomSensitivityWheel;

	if( hotkeys[HOTKEY_CAMERA_ZOOM_IN] )
		zoom_delta += ViewZoomSensitivity*DeltaTime;
	else if( hotkeys[HOTKEY_CAMERA_ZOOM_OUT] )
		zoom_delta -= ViewZoomSensitivity*DeltaTime;

	if (zoom_delta)
	{
		float zoom_proportion = powf(ViewZoomSmoothness, DeltaTime);
		g_Camera.m_Orientation.Translate(forwards * (zoom_delta * (1.0f-zoom_proportion)));
		zoom_delta *= zoom_proportion;
	}


#elif CAMERASTYLE == 1

	// Remember previous mouse position, to calculate changes
	static mouse_last_x = 0;
	static mouse_last_y = 0;

	// Miscellaneous vectors
	CVector3D forwards = g_Camera.m_Orientation.GetIn();
	CVector3D upwards (0.0f, 1.0f, 0.0f);
	CVector3D rightwards = upwards.Cross(forwards);

	// Click and drag to look around
	if (mouseButtons[0])
	{
		// Untranslate the camera, so it rotates around the correct point
		CVector3D position = g_Camera.m_Orientation.GetTranslation();
		g_Camera.m_Orientation.Translate(position*-1);

		// Sideways rotation
		g_Camera.m_Orientation.RotateY(ViewRotateSpeed*(float)(mouse_x-mouse_last_x));

		// Up/down rotation
		CQuaternion temp;
		temp.FromAxisAngle(rightwards, ViewRotateSpeed*(float)(mouse_y-mouse_last_y));
		g_Camera.m_Orientation.Rotate(temp);

		// Retranslate back to the right position
		g_Camera.m_Orientation.Translate(position);
	}
	mouse_last_x = mouse_x;
	mouse_last_y = mouse_y;

	// Calculate the necessary vectors for movement

	rightwards.Normalize();
	CVector3D forwards_horizontal = upwards.Cross(rightwards);
	forwards_horizontal.Normalize();

	// Move when desirable

	if (mouse_x >= g_xres-2)
		g_Camera.m_Orientation.Translate(rightwards);
	else if (mouse_x <= 3)
		g_Camera.m_Orientation.Translate(-rightwards);

	if (mouse_y >= g_yres-2)
		g_Camera.m_Orientation.Translate(forwards_horizontal);
	else if (mouse_y <= 3)
		g_Camera.m_Orientation.Translate(-forwards_horizontal);

	// Smoothed height-changing (move a certain percentage towards the desired height every frame)

	static float height_delta = 0.0f;

	if (mouseButtons[SDL_BUTTON_WHEELUP])
		height_delta -= 4.0f;
	else if (mouseButtons[SDL_BUTTON_WHEELDOWN])
		height_delta += 4.0f;

	const float height_speed = 0.2f;
	g_Camera.m_Orientation.Translate(0.0f, height_delta*height_speed, 0.0f);
	height_delta *= (1.0f - height_speed);

#else // CAMERASTYLE == 0

	const float dx = ViewScrollSpeed * DeltaTime;
	const CVector3D Right(dx,0, dx);
	const CVector3D Up   (dx,0,-dx);

	if (mouse_x >= g_xres-2)
		g_Camera.m_Orientation.Translate(Right);
	if (mouse_x <= 3)
		g_Camera.m_Orientation.Translate(Right*-1);

	if (mouse_y >= g_yres-2)
		g_Camera.m_Orientation.Translate(Up);
	if (mouse_y <= 3)
		g_Camera.m_Orientation.Translate(Up*-1);

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
		CVector3D forward=g_Camera.m_Orientation.GetIn();

		// check we're not going to zoom into the terrain, or too far out into space
		float h=g_Camera.m_Orientation.GetTranslation().Y+forward.Y*factor*g_CameraZoom;
		float minh=65536*HEIGHT_SCALE*1.05f;
		
		if (h<minh || h>1500) {
			// yup, we will; don't move anywhere (do clamped move instead, at some point)
		} else {
			// do a full move
			g_CameraZoom-=(factor)*0.1f;
			if (g_CameraZoom<0.01f) g_CameraZoom=0.01f;
			g_Camera.m_Orientation.Translate(forward*(factor*g_CameraZoom));
		}
	}
#endif // CAMERASTYLE

	g_Camera.UpdateFrustum ();
}


void terr_update(const float DeltaTime)
{
	if(g_active)
		move_camera(DeltaTime);
}

int terr_handler(const SDL_Event* ev)
{
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
			// quick hack to return camera home, for screenshots (after alt+tabbing)
			g_Camera.SetProjection (1, 5000, DEGTORAD(20));
			g_Camera.m_Orientation.SetXRotation(DEGTORAD(30));
			g_Camera.m_Orientation.RotateY(DEGTORAD(-45));
			g_Camera.m_Orientation.Translate (100, 150, -100);
			return( EV_HANDLED );

		case HOTKEY_CAMERA_ROTATE_ABOUT_TARGET:
			{
			int x, z;
			CHFTracer tracer( g_Terrain.GetHeightMap(), g_Terrain.GetVerticesPerSide(), (float)CELL_SIZE, HEIGHT_SCALE );
			CVector3D origin, dir;
			origin = g_Camera.m_Orientation.GetTranslation();
			dir = g_Camera.m_Orientation.GetIn();
			g_Camera.BuildCameraRay( origin, dir );

			if( !tracer.RayIntersect( origin, dir, x, z, cameraPivot ) )
				cameraPivot = origin - dir * ( origin.Y / dir.Y );
			}
			return( EV_HANDLED );

		}

	}

	return EV_PASS;
}




void InitScene ()
{
	// setup default lighting environment
	g_LightEnv.m_SunColor=RGBColor(1,1,1);
	g_LightEnv.m_Rotation=DEGTORAD(270);
	g_LightEnv.m_Elevation=DEGTORAD(45);
	g_LightEnv.m_TerrainAmbientColor=RGBColor(0,0,0);
	g_LightEnv.m_UnitsAmbientColor=RGBColor(0.4f,0.4f,0.4f);
	g_Renderer.SetLightEnv(&g_LightEnv);

	// load terrain
	TexInfo ti;
	int err = tex_load("terrain.raw", &ti);
	if(err == 0)
	{
		int w = ti.w, h = ti.h;
		printf("terrain.raw: %dx%d\n", w, h);
		char* p = (char*)mem_get_ptr(ti.hm);

		u16 *p16=new u16[w*h];
		u16 *p16p=p16;
		while (p16p < p16+(w*h))
			*p16p++ = (*p++) << 8;

		g_Terrain.Resize(w/PATCH_SIZE);
		g_Terrain.SetHeightMap(p16);

		delete[] p16;
		
		tex_free(&ti);
	}

	// get default texture to apply to terrain
	CTextureEntry* texture=0;
	for (uint ii=0;ii<g_TexMan.m_TerrainTextures.size();ii++) {  
		if (g_TexMan.m_TerrainTextures[ii].m_Textures.size()) {
			texture=g_TexMan.m_TerrainTextures[ii].m_Textures[0];
			break;
		}
	}


	// cover entire terrain with default texture
	u32 patchesPerSide=g_Terrain.GetPatchesPerSide();
	for (uint pj=0; pj<patchesPerSide; pj++) {
		for (uint pi=0; pi<patchesPerSide; pi++) {
			
			CPatch* patch=g_Terrain.GetPatch(pi,pj);
			
			for (int j=0;j<PATCH_SIZE;j++) {
				for (int i=0;i<PATCH_SIZE;i++) {
					patch->m_MiniPatches[j][i].Tex1=texture ? texture->GetHandle() :0;
				}
			}
		}
	}

	g_Camera.SetProjection (1, 5000, DEGTORAD(20));
	g_Camera.m_Orientation.SetXRotation(DEGTORAD(30));
	g_Camera.m_Orientation.RotateY(DEGTORAD(-45));

	g_Camera.m_Orientation.Translate (100, 150, -100);
}

void InitResources()
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

