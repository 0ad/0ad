#include "MiniMap.h"
#include "UIGlobals.h"
#include "TextureManager.h"
#include "Terrain.h"
#include "Renderer.h"
#include "ogl.h"

extern CTerrain g_Terrain;

static unsigned int ScaleColor(unsigned int color,float x)
{
	unsigned int r=unsigned int(float(color & 0xff)*x);
	unsigned int g=unsigned int(float((color>>8) & 0xff)*x);
	unsigned int b=unsigned int(float((color>>16) & 0xff)*x);
	return (0xff000000 | r | g<<8 | b<<16);
}

static int RoundUpToPowerOf2(int x)
{
	if ((x & (x-1))==0) return x;
	int d=x;
	while (d & (d-1)) {
		d&=(d-1);
	}
	return d<<1;
}

CMiniMap::CMiniMap() : m_Handle(0)
{
}

void CMiniMap::Initialise()
{
	// get rid of existing texture, if we've got one
	if (m_Handle) {
		glDeleteTextures(1,&m_Handle);
	}

	// allocate a handle, bind to it
	glGenTextures(1,&m_Handle);
	g_Renderer.BindTexture(0,m_Handle);

	// allocate an image big enough to fit the entire map into
	m_Size=RoundUpToPowerOf2(g_Terrain.GetVerticesPerSide());
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,m_Size,m_Size,0,GL_BGRA_EXT,GL_UNSIGNED_BYTE,0);

	// set texture parameters
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);

	// force a rebuild to get correct initial view
	Rebuild();
}

void CMiniMap::Render()
{
	// setup renderstate
	glDisable(GL_DEPTH_TEST);
	glDepthMask(0);

	// load identity modelview
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// setup ortho view
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	int w=g_Renderer.GetWidth();
	int h=g_Renderer.GetHeight();
	glOrtho(0,w,0,h,-1,1);

	// bind to the minimap
	g_Renderer.BindTexture(0,m_Handle);
	glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);

	float tclimit=float(g_Terrain.GetVerticesPerSide()-1)/float(m_Size);

	// render minimap as quad
	glBegin(GL_QUADS);
	
	glTexCoord2f(0,tclimit);
	glVertex2i(w-200,200);

	glTexCoord2f(0,0);
	glVertex2i(w-200,2);

	glTexCoord2f(tclimit,0);
	glVertex2i(w-3,2);

	glTexCoord2f(tclimit,tclimit);
	glVertex2i(w-3,200);

	glEnd();

	// switch off textures 
	glDisable(GL_TEXTURE_2D);
	
	// render border
	glLineWidth(2);
	glColor3f(0.4f,0.35f,0.8f);
	glBegin(GL_LINE_LOOP);
	glVertex2i(w-202,202);
	glVertex2i(w-1,202);
	glVertex2i(w-1,1);
	glVertex2i(w-202,1);
	glEnd();

	// render view rect
	glScissor(w-200,2,w,198);
	glEnable(GL_SCISSOR_TEST);
	glLineWidth(2);
	glColor3f(1.0f,0.3f,0.3f);
	glBegin(GL_LINE_LOOP);
	glVertex2f(w-200+m_ViewRect[0][0],m_ViewRect[0][1]);
	glVertex2f(w-200+m_ViewRect[1][0],m_ViewRect[1][1]);
	glVertex2f(w-200+m_ViewRect[2][0],m_ViewRect[2][1]);
	glVertex2f(w-200+m_ViewRect[3][0],m_ViewRect[3][1]);
	glEnd();
	glDisable(GL_SCISSOR_TEST);

	// restore matrices
	glPopMatrix();
	
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	// restore renderstate
	glEnable(GL_DEPTH_TEST);
	glDepthMask(1);
}

void CMiniMap::Update(int x,int y,unsigned int color)
{
	// bind to the minimap
	g_Renderer.BindTexture(0,m_Handle);

	// get height at this pixel
	int hmap=(int) g_Terrain.GetHeightMap()[y*g_Terrain.GetVerticesPerSide() + x]>>8;
	// shift from 0-255 to 170-255
	int val=(hmap/3)+170;

	// get modulated color	
	unsigned int mcolor=ScaleColor(color,float(val)/255.0f);

	// subimage to update pixel (ugh)
	glTexSubImage2D(GL_TEXTURE_2D,0,x,y,1,1,GL_BGRA_EXT,GL_UNSIGNED_BYTE,&mcolor);
}

void CMiniMap::Update(int x,int y,int w,int h,unsigned int color)
{
	// bind to the minimap
	g_Renderer.BindTexture(0,m_Handle);

	u32 mapSize=g_Terrain.GetVerticesPerSide();
	unsigned int* data=new unsigned int[w*h];
	unsigned int* dataptr=data;

	for (int j=0;j<h;j++) {
		for (int i=0;i<w;i++) {
			// get height at this pixel
			int hmap=int(g_Terrain.GetHeightMap()[(y+j)*mapSize + x+i])>>8;
			// shift from 0-255 to 170-255
			int val=(hmap/3)+170;
			// load scaled color into data pointer
			*dataptr++=ScaleColor(color,float(val)/255.0f);
		}
	}
	
	// subimage to update pixels 
	glTexSubImage2D(GL_TEXTURE_2D,0,x,y,w,h,GL_BGRA_EXT,GL_UNSIGNED_BYTE,data);
	delete[] data;
}

void CMiniMap::Rebuild()
{
	// bind to the minimap
	g_Renderer.BindTexture(0,m_Handle);

	u32 mapSize=g_Terrain.GetVerticesPerSide();
	unsigned int* data=new unsigned int[(mapSize-1)*(mapSize-1)];
	unsigned int* dataptr=data;

	for (uint pj=0; pj<g_Terrain.GetPatchesPerSide(); pj++)
	{
		for (int tj=0; tj<16; tj++)
		{
			for (uint pi=0; pi<g_Terrain.GetPatchesPerSide(); pi++)
			{
				for (int ti=0; ti<16; ti++)
				{
					int i=pi*16+ti;
					int j=pj*16+tj;

					// get base color from textures on tile
					unsigned int color;
					CMiniPatch* mp=g_Terrain.GetTile(i,j);
					if (mp) {
						CTextureEntry* tex=mp->Tex1 ? g_TexMan.FindTexture(mp->Tex1) : 0;
						color=tex ? tex->m_BaseColor : 0xffffffff;
					} else {
						color=0xffffffff;
					}

					// get height at this pixel
					int hmap=int(g_Terrain.GetHeightMap()[j*mapSize + i])>>8;
					// shift from 0-255 to 170-255
					int val=(hmap/3)+170;
					// load scaled color into data pointer
					*dataptr++=ScaleColor(color,float(val)/255.0f);
				}
			}
		}
	}

	// subimage to update pixels 
	glTexSubImage2D(GL_TEXTURE_2D,0,0,0,(mapSize-1),(mapSize-1),GL_BGRA_EXT,GL_UNSIGNED_BYTE,data);
	delete[] data;
}

void CMiniMap::Rebuild(int x,int y,int w,int h)
{
	// bind to the minimap
	g_Renderer.BindTexture(0,m_Handle);

	u32 mapSize=g_Terrain.GetVerticesPerSide();
	unsigned int* data=new unsigned int[w*h];
	unsigned int* dataptr=data;

	for (int j=0;j<h;j++) {
		for (int i=0;i<w;i++) {
			// get height at this pixel
			int hmap=int(g_Terrain.GetHeightMap()[(y+j)*mapSize + x+i])>>8;
			// shift from 0-255 to 170-255
			int val=(hmap/3)+170;

			CMiniPatch* mp=g_Terrain.GetTile(x+i,y+j);
			
			unsigned int color;
			if (mp) {
				// get texture on this time
				CTextureEntry* tex=mp->Tex1 ? g_TexMan.FindTexture(mp->Tex1) : 0;
				color=tex ? tex->m_BaseColor : 0xffffffff;
			} else {
				color=0xffffffff;
			}

			// load scaled color into data pointer
			*dataptr++=ScaleColor(color,float(val)/255.0f);
		}
	}

	// subimage to update pixels 
	glTexSubImage2D(GL_TEXTURE_2D,0,x,y,w,h,GL_BGRA_EXT,GL_UNSIGNED_BYTE,data);
	delete[] data;
}
