#include "precompiled.h"
#include "ogl.h"
#include "res/tex.h"
#include "TextureEntry.h"
#include "TextureManager.h"
#include "Texture.h"
#include "CLogger.h"
#include "Renderer.h"


/////////////////////////////////////////////////////////////////////////////////////
// CTextureEntry constructor
CTextureEntry::CTextureEntry(const char* name,int type)	
	: m_Name(name), m_Bitmap(0), m_Handle(0xffffffff), m_BaseColor(0), m_Type(type), m_BaseColorValid(false) 
{
}

/////////////////////////////////////////////////////////////////////////////////////
// LoadTexture: actually load the texture resource from file
void CTextureEntry::LoadTexture()	
{
	CStr pathname("art/textures/terrain/types/");
	pathname+=g_TexMan.m_TerrainTextures[m_Type].m_Name;
	pathname+='/';
	pathname+=m_Name;

	CTexture texture;
	texture.SetName(pathname);
	if (g_Renderer.LoadTexture(&texture,GL_REPEAT)) {
		m_Handle=texture.GetHandle();
	} else {
		m_Handle=0;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BuildBaseColor: calculate the root color of the texture, used for coloring minimap, and store
// in m_BaseColor member
void CTextureEntry::BuildBaseColor()
{
	Handle handle=GetHandle();
	g_Renderer.BindTexture(0,tex_id(handle));

	// get root color for coloring minimap by querying root level of the texture 
	// (this should decompress any compressed textures for us),
	// then scaling it down to a 1x1 size 
	//	- an alternative approach of just grabbing the top level of the mipmap tree fails 
	//	(or gives an incorrect colour) in some cases: 
	//		- suspect bug on Radeon cards when SGIS_generate_mipmap is used 
	//		- any textures without mipmaps
	// we'll just take the basic approach here:
	int width,height;
	glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&height);

	unsigned char* buf=new unsigned char[width*height*4];
	glGetTexImage(GL_TEXTURE_2D,0,GL_BGRA_EXT,GL_UNSIGNED_BYTE,buf);
	gluScaleImage(GL_BGRA_EXT,width,height,GL_UNSIGNED_BYTE,buf,
			1,1,GL_UNSIGNED_BYTE,&m_BaseColor);
	delete[] buf;

	m_BaseColorValid=true;
}