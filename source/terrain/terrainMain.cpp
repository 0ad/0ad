#include "Matrix3D.H"
#include "Renderer.H"
#include "Terrain.H"
#include "../ps/Prometheus.h"

#include "time.h"
#include "wsdl.h"
#include "tex.h"

// TODO: fix scrolling hack - framerate independent, use SDL
#include "win.h"	// REMOVEME

void InitScene ();
void InitResources ();
void RenderScene ();

extern bool keys[256];


CMatrix3D			g_WorldMat;
CRenderer			g_Renderer;
CTerrain			g_Terrain;
CCamera				g_Camera;

int					SelPX, SelPY, SelTX, SelTY;
int					g_BaseTexCounter = 0;
int					g_SecTexCounter = 1;
int					g_TransTexCounter = 0;

int					g_TickCounter = 0;
double				g_LastTime;


const int NUM_ALPHA_MAPS = 13;

//CTexture			g_BaseTexture[5];
Handle BaseTexs[5];

Handle AlphaMaps[NUM_ALPHA_MAPS];
//CTexture			g_TransitionTexture[NUM_ALPHA_MAPS];

int mouse_x=50, mouse_y=50;


void terr_init()
{
#ifdef WIDEASPECT
	g_Renderer.Initialize( 1440, 900, 32 );
#else
	g_Renderer.Initialize (1600, 1200, 32);
#endif

	InitResources ();
	InitScene ();
}

void terr_update()
{
g_FrameCounter++;

	/////////////////////////////////////////////
		POINT MousePos;

		GetCursorPos (&MousePos);
		CVector3D right(1,0,1);
		CVector3D up(1,0,-1);
		right.Normalize ();
		up.Normalize ();
		
		if (mouse_x >= g_xres-2)
			g_Camera.m_Orientation.Translate (right);
		if (mouse_x <= 3)
			g_Camera.m_Orientation.Translate (right*-1);

		if (mouse_y >= g_yres-2)
			g_Camera.m_Orientation.Translate (up);
		if (mouse_y <= 3)
			g_Camera.m_Orientation.Translate (up*-1);



		float fov = g_Camera.GetFOV();
		float d = DEGTORAD(0.4f);
		if(keys[SDLK_KP_MINUS])
			if (fov+d < DEGTORAD(90))
				g_Camera.SetProjection (1, 1000, fov + d);
		if(keys[SDLK_KP_ADD])
			if (fov-d > DEGTORAD(20))
			g_Camera.SetProjection (1, 1000, fov - d);

		g_Camera.UpdateFrustum ();
/////////////////////////////////////////////


	g_Renderer.RenderTerrain (&g_Terrain, &g_Camera);
	g_Renderer.RenderTileOutline (&(g_Terrain.m_Patches[SelPY][SelPX].m_MiniPatches[SelTY][SelTX]));



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
			g_WireFrame = !g_WireFrame;
			break;

		case 'H':
			// quick hack to return camera home, for screenshots (after alt+tabbing)
			g_Camera.SetProjection (1, 1000, DEGTORAD(20));
			g_Camera.m_Orientation.SetXRotation(DEGTORAD(30));
			g_Camera.m_Orientation.RotateY(DEGTORAD(-45));
			g_Camera.m_Orientation.Translate (100, 150, -100);
			break;

		case 'L':
			g_HillShading = !g_HillShading;
			break;

// tile selection
		case SDLK_DOWN:
			if(++SelTX > 15)
				if(SelPX == NUM_PATCHES_PER_SIDE-1)
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
				if(SelPY == NUM_PATCHES_PER_SIDE-1)
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
					CMiniPatch *MPatch = &g_Terrain.m_Patches[SelPY][SelPX].m_MiniPatches[SelTY][SelTX];
					if (!MPatch->Tex2)
					{
						MPatch->m_AlphaMap = AlphaMaps[g_TransTexCounter];
						MPatch->Tex2 = BaseTexs[g_SecTexCounter];
					}
					else
					{
						MPatch->Tex2 = 0;
						MPatch->m_AlphaMap = 0;
					}
					break;
				}

		case SDLK_KP1:
				{
					CMiniPatch *MPatch = &g_Terrain.m_Patches[SelPY][SelPX].m_MiniPatches[SelTY][SelTX];

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
					
					if (MPatch->/*m_pTransitionTexture*/m_AlphaMap)
					{
						g_TransTexCounter++;
						if (g_TransTexCounter >= NUM_ALPHA_MAPS)
							g_TransTexCounter = 0;

						MPatch->m_AlphaMap = AlphaMaps[g_TransTexCounter];
					}

					break;
				}

		}
	}

	return false;
}




void InitScene ()
{
	g_Terrain.Initalize ("terrain.raw");

	for (int pj=0; pj<NUM_PATCHES_PER_SIDE; pj++)
	{
		for (int pi=0; pi<NUM_PATCHES_PER_SIDE; pi++)
		{
			for (int tj=0; tj<16; tj++)
			{
				for (int ti=0; ti<16; ti++)
				{
					g_Terrain.m_Patches[pj][pi].m_MiniPatches[tj][ti].Tex1 = BaseTexs[0];//rand()%5];
					g_Terrain.m_Patches[pj][pi].m_MiniPatches[tj][ti].Tex2 = NULL;//&g_BaseTexture[rand()%5];
					g_Terrain.m_Patches[pj][pi].m_MiniPatches[tj][ti].m_AlphaMap = 0;//&g_TransitionTexture[rand()%5];
				}
			}
		}
	}

	g_Camera.SetProjection (1, 1000, DEGTORAD(20));
	g_Camera.m_Orientation.SetXRotation(DEGTORAD(30));
	g_Camera.m_Orientation.RotateY(DEGTORAD(-45));

	g_Camera.m_Orientation.Translate (100, 150, -100);

	SelPX = SelPY = SelTX = SelTY = 0;
}

void InitResources()
{
	int i;
	char* base_fns[] =
	{
	"Base1.bmp",
	"Base2.bmp",
	"Base3.bmp",
	"Base4.bmp",
	"Base5.bmp"
	};

	for(i = 0; i < 5; i++)
	{
		BaseTexs[i] = tex_load(base_fns[i]);
		tex_upload(BaseTexs[i], GL_LINEAR_MIPMAP_LINEAR);
	}


int cnt;
#if 1

	char* fns[NUM_ALPHA_MAPS] = {
"blendcircle.raw",
"blendcorner.raw",
"blendedge.raw",
"blendedgecorner.raw",
"blendedgetwocorners.raw",
"blendfourcorners.raw",
"blendlshape.raw",
"blendlshapecorner.raw",
"blendthreecorners.raw",
"blendtwocorners.raw",
"blendtwoedges.raw",
"blendtwooppositecorners.raw",
"blendushape.raw"
	};

/*
//for(i = 0; i < NUM_ALPHA_MAPS;i++)
i=5;
{
FILE* f = fopen(fns[i],"rb");
u8 buf[5000],buf2[5000];
fread(buf,5000,1,f);
fclose(f);
for(int j = 0; j < 1024; j++)
buf2[2*j] = buf2[2*j+1] = buf[j];
f=fopen(fns[i],"wb");
fwrite(buf2,2048,1,f);
fclose(f);
}
/**/
cnt=13;
#else

	char* fns[NUM_ALPHA_MAPS] = {
"Transition1.bmp",
"Transition2.bmp",
"Transition3.bmp",
"Transition4.bmp",
"Transition5.bmp",
	};
cnt=5;
#endif

for(i = 0; i < cnt; i++)
{
    AlphaMaps[i] = tex_load(fns[i]);
	tex_upload(AlphaMaps[i], GL_LINEAR, GL_INTENSITY4);
}

}

