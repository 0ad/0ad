#include "precompiled.h"

#include "Matrix3D.h"
#include "Renderer.h"
#include "Terrain.h"
#include "LightEnv.h"
#include "TextureManager.h"
#include "ObjectManager.h"
#include "Prometheus.h"

#include "sdl.h"
#include "res/tex.h"
#include "detect.h"

void InitScene ();
void InitResources ();
void RenderScene ();

extern bool keys[512];	// SDL also defines non-ascii keys; 512 should be enough
extern bool mouseButtons[5];

CMatrix3D			g_WorldMat;
CTerrain			g_Terrain;
CCamera				g_Camera;
CLightEnv			g_LightEnv;

int					SelPX, SelPY, SelTX, SelTY;
int					g_BaseTexCounter = 0;
int					g_SecTexCounter = 1;
int					g_TransTexCounter = 0;

int					g_TickCounter = 0;
double				g_LastTime;

static float g_CameraZoom = 10;

const int NUM_ALPHA_MAPS = 13;
const float ViewScrollSpeed = 60;
float ViewFOV;

int mouse_x=50, mouse_y=50;

extern int g_xres, g_yres;

void terr_init()
{
	SViewPort vp;
	vp.m_X=0;
	vp.m_Y=0;
	vp.m_Width=g_xres;
	vp.m_Height=g_yres;
	g_Camera.SetViewPort(&vp);

	InitResources ();
	InitScene ();
}

void terr_update(const float DeltaTime)
{
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

	g_Camera.UpdateFrustum ();
}






bool terr_handler(const SDL_Event& ev)
{
	switch(ev.type)
	{
	case SDL_MOUSEMOTION:
		mouse_x = ev.motion.x;
		mouse_y = ev.motion.y;
		break;
	

	case SDL_KEYDOWN:
		switch(ev.key.keysym.sym)
		{
		case 'W':
			if (g_Renderer.GetTerrainRenderMode()==WIREFRAME) {
				g_Renderer.SetTerrainRenderMode(SOLID);
			} else {
				g_Renderer.SetTerrainRenderMode(WIREFRAME);
			}
			break;

		case 'H':
			// quick hack to return camera home, for screenshots (after alt+tabbing)
			g_Camera.SetProjection (1, 5000, DEGTORAD(20));
			g_Camera.m_Orientation.SetXRotation(DEGTORAD(30));
			g_Camera.m_Orientation.RotateY(DEGTORAD(-45));
			g_Camera.m_Orientation.Translate (100, 150, -100);
			break;

		}
	}

	return false;
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
	Handle ht = tex_load("terrain.raw");
	if(ht > 0)
	{
		const u8* p;
		int w;
		int h;

		tex_info(ht, &w, &h, NULL, NULL, (void **)&p);

		printf("terrain.raw: %dx%d\n", w, h);

		u16 *p16=new u16[w*h];
		u16 *p16p=p16;
		while (p16p < p16+(w*h))
			*p16p++ = (*p++) << 8;

		g_Terrain.Resize(w/PATCH_SIZE);
		g_Terrain.SetHeightMap(p16);

		delete[] p16;
		
		tex_free(ht);
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
