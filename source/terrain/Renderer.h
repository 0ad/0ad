#ifndef RENDERER_H
#define RENDERER_H

#include "ogl.h"

#include "Terrain.H"

extern bool g_WireFrame;
extern unsigned int g_FrameCounter;

class CRenderer
{
	public:
		CRenderer();
		~CRenderer();

		bool Initialize (int width, int height, int depth);
		void Shutdown ();

		void RenderTerrain (CTerrain *terrain, CCamera *camera);
		void RenderTileOutline (CMiniPatch *mpatch);

	protected:
		void RenderPatchBase (CPatch *patch);
		void RenderPatchTrans (CPatch *patch);

	protected:
		int					m_Width;
		int					m_Height;
		int					m_Depth;

///THERE ARE NOT SUPPOSED TO BE HERE
		float				m_Timer;
		int					m_CurrentSeason;

};


#endif