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
CLightEnv			g_LightEnv;


/*void InitScene ()
{
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

}

*/
