#include "precompiled.h"
#include "MiniMap.h"
#include "ui/UIGlobals.h"
#include "TextureManager.h"
#include "Game.h"
#include "Renderer.h"
#include "ogl.h"

struct TGAHeader {
	// header stuff
	unsigned char  iif_size;            
	unsigned char  cmap_type;           
	unsigned char  image_type;          
	unsigned char  pad[5];

	// origin : unused
	unsigned short d_x_origin;
	unsigned short d_y_origin;
	
	// dimensions
	unsigned short width;
	unsigned short height;

	// bits per pixel : 16, 24 or 32
	unsigned char  bpp;          

	// image descriptor : Bits 3-0: size of alpha channel
	//					  Bit 4: must be 0 (reserved)
	//					  Bit 5: should be 0 (origin)
	//					  Bits 6-7: should be 0 (interleaving)
   unsigned char image_descriptor;    
};

static bool saveTGA(const char* filename,int width,int height,int bpp,unsigned char* data) 
{
	FILE* fp=fopen(filename,"wb");
	if (!fp) return false;

	// fill file header
	TGAHeader header;
	header.iif_size=0;
	header.cmap_type=0;
	header.image_type=2;
	memset(header.pad,0,sizeof(header.pad));
	header.d_x_origin=0;
	header.d_y_origin=0;
	header.width=width;
	header.height=height;
	header.bpp=bpp;
	header.image_descriptor=(bpp==32) ? 8 : 0;

	if (fwrite(&header,sizeof(TGAHeader),1,fp)!=1) {
		fclose(fp);
		return false;
	}

	// write data 
	if (fwrite(data,width*height*bpp/8,1,fp)!=1) {
		fclose(fp);
		return false;
	}

	// return success ..
    fclose(fp);
	return true;
}



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

CMiniMap::CMiniMap() : m_Handle(0), m_Data(0), m_Size(0)
{
}

CMiniMap::~CMiniMap()
{
	delete m_Data;
}

void CMiniMap::Initialise()
{
	// get rid of existing texture, if we've got one
	if (m_Handle) {
		glDeleteTextures(1,(GLuint*) &m_Handle);
		delete[] m_Data;
	}

	// allocate a handle, bind to it
	glGenTextures(1,(GLuint*) &m_Handle);
	g_Renderer.BindTexture(0,m_Handle);

	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();

	// allocate an image big enough to fit the entire map into
	m_Size=RoundUpToPowerOf2(terrain->GetVerticesPerSide());
	u32 mapSize=terrain->GetVerticesPerSide();
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,m_Size,m_Size,0,GL_BGRA_EXT,GL_UNSIGNED_BYTE,0);

	// allocate local copy
	m_Data=new u32[(mapSize-1)*(mapSize-1)];

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

	float tclimit=float(g_Game->GetWorld()->GetTerrain()->GetVerticesPerSide()-1)/float(m_Size);

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
	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();

	// get height at this pixel
	int hmap=(int) terrain->GetHeightMap()[y*terrain->GetVerticesPerSide() + x]>>8;
	// shift from 0-255 to 170-255
	int val=(hmap/3)+170;

	// get modulated color	
	u32 mapSize=terrain->GetVerticesPerSide();
	*(m_Data+(y*(mapSize-1)+x))=ScaleColor(color,float(val)/255.0f);

	UpdateTexture();
}

void CMiniMap::Update(int x,int y,int w,int h,unsigned int color)
{
	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();

	u32 mapSize=terrain->GetVerticesPerSide();

	for (int j=0;j<h;j++) {
		u32* dataptr=m_Data+((y+j)*(mapSize-1))+x;
		for (int i=0;i<w;i++) {
			// get height at this pixel
			int hmap=int(terrain->GetHeightMap()[(y+j)*mapSize + x+i])>>8;
			// shift from 0-255 to 170-255
			int val=(hmap/3)+170;
			// load scaled color into data pointer
			*dataptr++=ScaleColor(color,float(val)/255.0f);
		}
	}
	
	UpdateTexture();
}

void CMiniMap::Rebuild()
{
	u32 mapSize=g_Game->GetWorld()->GetTerrain()->GetVerticesPerSide();
	Rebuild(0,0,mapSize-1,mapSize-1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UpdateTexture: send data to GL; update stored texture data
void CMiniMap::UpdateTexture()
{
	// bind to the minimap
	g_Renderer.BindTexture(0,m_Handle);
	// subimage to update pixels 
	u32 mapSize=g_Game->GetWorld()->GetTerrain()->GetVerticesPerSide();
	glTexSubImage2D(GL_TEXTURE_2D,0,0,0,mapSize-1,mapSize-1,GL_BGRA_EXT,GL_UNSIGNED_BYTE,m_Data);	
}

void CMiniMap::Rebuild(int x,int y,int w,int h)
{
	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();

	u32 mapSize=terrain->GetVerticesPerSide();

	for (int j=0;j<h;j++) {
		u32* dataptr=m_Data+((y+j)*(mapSize-1))+x;
		for (int i=0;i<w;i++) {
			// get height at this pixel
			int hmap=int(terrain->GetHeightMap()[(y+j)*mapSize + x+i])>>8;
			// shift from 0-255 to 170-255
			int val=(hmap/3)+170;

			CMiniPatch* mp=terrain->GetTile(x+i,y+j);
			
			unsigned int color;
			if (mp) {
				// get texture on this time
				CTextureEntry* tex=mp->Tex1 ? g_TexMan.FindTexture(mp->Tex1) : 0;
				color=tex ? tex->GetBaseColor() : 0xffffffff;
			} else {
				color=0xffffffff;
			}

			// load scaled color into data pointer
			*dataptr++=ScaleColor(color,float(val)/255.0f);
		}
	}

	UpdateTexture();
}
