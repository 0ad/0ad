#ifndef _MINIMAP_H
#define _MINIMAP_H

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
	// texture handle
	unsigned int m_Handle;
	// size of the map texture
	unsigned int m_Size;
};

extern CMiniMap g_MiniMap;

#endif
