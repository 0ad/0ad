#ifndef _MINIMAP_H
#define _MINIMAP_H

#include "lib.h"

class CMiniMap
{
public:
	CMiniMap();

	void Initialise();
	void Render();
	void Update(int x,int y,unsigned int color);
	void Update(int x,int y,int w,int h,unsigned int color);
	void Rebuild();
	void Rebuild(int x,int y,int w,int h);

	// current viewing frustum, in minimap coordinate space
	float m_ViewRect[4][2];

private:
	// send data to GL; update stored texture data
	void UpdateTexture();
	// texture handle
	u32 m_Handle;
	// size of the map texture
	u32 m_Size;
	// raw BGRA_EXT data for the minimap
	u32* m_Data;
};

extern CMiniMap g_MiniMap;

#endif
