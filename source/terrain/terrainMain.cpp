
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
CRenderer			g_Renderer;
CTerrain			g_Terrain;
CCamera				g_Camera;
CLightEnv			g_LightEnv;

int					SelPX, SelPY, SelTX, SelTY;
int					g_BaseTexCounter = 0;
int					g_SecTexCounter = 1;
int					g_TransTexCounter = 0;

int					g_TickCounter = 0;
double				g_LastTime;


const int NUM_ALPHA_MAPS = 13;
const float ViewScrollSpeed = 60;
float ViewFOV;

int mouse_x=50, mouse_y=50;

void terr_init()
{
	int xres,yres;
	get_cur_resolution(xres,yres);
	g_Renderer.Open(xres,yres,32);

	SViewPort vp;
	vp.m_X=0;
	vp.m_Y=0;
	vp.m_Width=xres;
	vp.m_Height=yres;
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
	const float s30 = sin(DEGTORAD(30.0f));
	const float c30 = cos(DEGTORAD(30.0f));
	const float s45 = sin(DEGTORAD(45.0f));
	const float c45 = cos(DEGTORAD(45.0f));
	const float s60 = sin(DEGTORAD(60.0f));
	const float c60 = cos(DEGTORAD(60.0f));

	const CVector3D viewer_back(c30*c45, s45, -s30*c45);

janwas: grr, plotted the zoom vector on paper twice, but it appears
to be completely wrong. sticking with the FOV hack for now.
if anyone sees what's wrong, or knows how to correctly implement zoom,
please put this code out of its misery :)

*/

	float fov = g_Camera.GetFOV();
	const float d_key = DEGTORAD(10.0f) * DeltaTime;
	const float d_wheel = DEGTORAD( 50.0f ) * DeltaTime;
	const float fov_max = DEGTORAD( 60.0f );
	const float fov_min = DEGTORAD( 10.0f );

	if(keys[SDLK_KP_MINUS])
		if (fov < fov_max)
			fov += d_key;
	if(keys[SDLK_KP_PLUS])
		if (fov-d_key > fov_min)
			fov -= d_key;
	if( mouseButtons[SDL_BUTTON_WHEELUP] )
	{
		fov += d_wheel;
		if( fov > fov_max )
			fov = fov_max;
	}
	if( mouseButtons[SDL_BUTTON_WHEELDOWN] )
	{
		fov -= d_wheel;
		if( fov < fov_min )
			fov = fov_min;
	}


	ViewFOV = fov;

	g_Camera.SetProjection(1, 1000, fov);
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
			g_Camera.SetProjection (1, 1000, DEGTORAD(20));
			g_Camera.m_Orientation.SetXRotation(DEGTORAD(30));
			g_Camera.m_Orientation.RotateY(DEGTORAD(-45));
			g_Camera.m_Orientation.Translate (100, 150, -100);
			break;

/*		case 'L':
			g_HillShading = !g_HillShading;
			break;

// tile selection
		case SDLK_DOWN:
			if(++SelTX > 15)
				if(SelPX == g_Terrain.GetPatchesPerSide()-1)
					SelTX = 15;
				else
					SelTX = 0, SelPX++;
			break;

		case SDLK_UP:
			if(--SelTX < 0)
				if(SelPX == 0)
					SelTX = 0;
				else
					SelTX = 15, SelPX--;
			break;
		case SDLK_RIGHT:
			if(++SelTY > 15)
				if(SelPY == g_Terrain.GetPatchesPerSide()-1)
					SelTY = 15;
				else
					SelTY = 0, SelPY++;
			break;

		case SDLK_LEFT:
			if(--SelTY < 0)
				if(SelPY == 0)
					SelTY = 0;
				else
					SelTY = 15, SelPY--;
			break;


		case SDLK_KP0:
				{
					CMiniPatch *MPatch = &g_Terrain.GetPatch(SelPY, SelPX)->m_MiniPatches[SelTY][SelTX];
					/*if (!MPatch->Tex2)
					{
						MPatch->m_AlphaMap = AlphaMaps[g_TransTexCounter];
						MPatch->Tex2 = BaseTexs[g_SecTexCounter];
					}
					else
					{
						MPatch->Tex2 = 0;
						MPatch->m_AlphaMap = 0;
					}*/
					break;
				}

		/*case SDLK_KP1:
				{
					CMiniPatch *MPatch = &g_Terrain.GetPatch(SelPY, SelPX)->m_MiniPatches[SelTY][SelTX];

					g_BaseTexCounter++;
					if (g_BaseTexCounter > 4)
						g_BaseTexCounter = 0;
					
					MPatch->Tex1 = BaseTexs[g_BaseTexCounter];
					break;
				}

		case SDLK_KP2:
				{
					CMiniPatch *MPatch = &g_Terrain.m_Patches[SelPY][SelPX].m_MiniPatches[SelTY][SelTX];
					
					if (MPatch->Tex2)
					{
						g_SecTexCounter++;
						if (g_SecTexCounter > 4)
							g_SecTexCounter = 0;

						MPatch->Tex2 = BaseTexs[g_SecTexCounter];
					}

					break;
				}
						
		case SDLK_KP3:
				{
					CMiniPatch *MPatch = &g_Terrain.m_Patches[SelPY][SelPX].m_MiniPatches[SelTY][SelTX];
					
					if (MPatch->m_AlphaMap)
					{
						g_TransTexCounter++;
						if (g_TransTexCounter >= NUM_ALPHA_MAPS)
							g_TransTexCounter = 0;

						MPatch->m_AlphaMap = AlphaMaps[g_TransTexCounter];
					}

					break;
				}*/

		//}
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

		g_Terrain.Resize(20);
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
			
			for (int j=0;j<16;j++) {
				for (int i=0;i<16;i++) {
					patch->m_MiniPatches[j][i].Tex1=texture ? texture->m_Handle :0;
				}
			}
		}
	}

	g_Camera.SetProjection (1, 1000, DEGTORAD(20));
	g_Camera.m_Orientation.SetXRotation(DEGTORAD(30));
	g_Camera.m_Orientation.RotateY(DEGTORAD(-45));

	g_Camera.m_Orientation.Translate (100, 150, -100);

}

void InitResources()
{
#ifndef _WIN32
	g_TexMan.AddTextureType("grass");
	g_TexMan.AddTexture("Base1.tga", 0);
#else
	g_TexMan.LoadTerrainTextures();
	g_ObjMan.LoadObjects();
#endif

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
