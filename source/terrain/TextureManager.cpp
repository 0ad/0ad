#include "TextureManager.h"
#include "lib.h"
#include "ogl.h"
#include "res/tex.h"
#ifdef _WIN32
#include <io.h>
#endif
#include <algorithm>

const char* SupportedTextureFormats[] = { "png", "dds", "tga", "bmp" };


CTextureManager g_TexMan;

int GetNumMipmaps(int w,int h)
{
	int mip=0;
	int dim=(w > h) ? w : h;
	while(dim) {
		dim>>=1;
		mip++;
	}
	return mip;
}

CTextureManager::CTextureManager()
{
	m_TerrainTextures.reserve(32);
}

void CTextureManager::AddTextureType(const char* name)
{
	m_TerrainTextures.resize(m_TerrainTextures.size()+1);
	STextureType& ttype=m_TerrainTextures.back();
	ttype.m_Name=name;
	ttype.m_Index=m_TerrainTextures.size()-1;
}

CTextureEntry* CTextureManager::FindTexture(const char* filename)
{
	// check if file already loaded
	for (uint k=0;k<m_TerrainTextures.size();k++) {
		STextureType& ttype=m_TerrainTextures[k];
		for (uint i=0;i<ttype.m_Textures.size();i++) {
			if (strcmp((const char*) ttype.m_Textures[i]->m_Name,filename)==0) {
				return ttype.m_Textures[i];
			}
		}
	}

	return 0;
}

CTextureEntry* CTextureManager::FindTexture(Handle handle)
{
	for (uint k=0;k<m_TerrainTextures.size();k++) {
		STextureType& ttype=m_TerrainTextures[k];
		for (uint i=0;i<ttype.m_Textures.size();i++) {
			if (handle==ttype.m_Textures[i]->m_Handle) {
				return ttype.m_Textures[i];
			}
		}
	}

	return 0;
}

static bool IsCompressed(Handle h)
{
	int fmt;
	tex_info(h, NULL, NULL, &fmt, NULL, NULL);
	if (fmt==GL_COMPRESSED_RGB_S3TC_DXT1_EXT) return true;
	if (fmt==GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) return true;
	if (fmt==GL_COMPRESSED_RGBA_S3TC_DXT3_EXT) return true;
	if (fmt==GL_COMPRESSED_RGBA_S3TC_DXT5_EXT) return true;
	return false;
}

CTextureEntry* CTextureManager::AddTexture(const char* filename,int type)
{
	assert(type<m_TerrainTextures.size());
	
	CStr pathname("art/textures/terrain/types/");
	pathname+=m_TerrainTextures[type].m_Name;
	pathname+='/';
	pathname+=filename;

	Handle h=tex_load((const char*) pathname);
	if (!h) {
		return 0;
	} else {
		int tw;
		int th;

		tex_info(h, &tw, &th, NULL, NULL, NULL);

		tw &= (tw-1);
		th &= (th-1);
		if (tw || th) {
			return 0;
		}
	}
	
	// create new texture entry
	CTextureEntry* texentry=new CTextureEntry;
	texentry->m_Name=filename;
	texentry->m_Handle=h;
	texentry->m_Type=type;

	// upload texture for future GL use
	if (IsCompressed(h)) {
		tex_upload(h,GL_LINEAR);
	} else {
		tex_upload(h,GL_LINEAR_MIPMAP_LINEAR);
	}
	// setup texture to repeat
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// get root color for coloring minimap
	int width,height;
	glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&height);
	int mip=GetNumMipmaps(width,height);
	glGetTexImage(GL_TEXTURE_2D,mip-1,GL_BGRA_EXT,GL_UNSIGNED_BYTE,&texentry->m_BaseColor);

	// add entry to list ..
	m_TerrainTextures[type].m_Textures.push_back(texentry);

	// .. and return it
	return texentry;
}

void CTextureManager::DeleteTexture(CTextureEntry* entry)
{
	// find entry in list
	std::vector<CTextureEntry*>& textures=m_TerrainTextures[entry->m_Type].m_Textures;
		
	typedef std::vector<CTextureEntry*>::iterator Iter;
	Iter i=std::find(textures.begin(),textures.end(),entry);
	if (i!=textures.end()) {
		textures.erase(i);
	}
	delete entry;
}

void CTextureManager::LoadTerrainTextures(int terraintype,const char* fileext)
{
#ifdef _WIN32
	struct _finddata_t file;
	long handle;
	
	// build pathname
	CStr pathname("mods\\official\\art\\textures\\terrain\\types\\");
	pathname+=m_TerrainTextures[terraintype].m_Name;
	pathname+="\\";
	
	CStr findname(pathname);
	findname+="*.";
	findname+=fileext;

	// Find first matching file in directory for this terrain type
    if ((handle=_findfirst((const char*) findname,&file))!=-1) {
		
		AddTexture(file.name,terraintype);

		// Find the rest of the matching files
        while( _findnext(handle,&file)==0) {
			AddTexture((const char*) file.name,terraintype);
		}

        _findclose(handle);
	}
#endif
}

void CTextureManager::BuildTerrainTypes()
{
#ifdef _WIN32
	struct _finddata_t file;
	long handle;
	
	// Find first matching directory in terrain\textures
	if ((handle=_findfirst("mods\\official\\art\\textures\\terrain\\types\\*",&file))!=-1) {
		
		if ((file.attrib & _A_SUBDIR) && file.name[0]!='.' && file.name[0]!='_') {
			AddTextureType(file.name);
		}

		// Find the rest of the matching files
        while( _findnext(handle,&file)==0) {
			if ((file.attrib & _A_SUBDIR) && file.name[0]!='.' && file.name[0]!='_') {
				AddTextureType(file.name);
			}
		}

        _findclose(handle);
	}
#endif
}

void CTextureManager::LoadTerrainTextures()
{
	// find all the terrain types by directory name
	BuildTerrainTypes();

	// now iterate through terrain types loading all textures of that type
	for (uint i=0;i<m_TerrainTextures.size();i++) {
		for (uint j=0;j<sizeof(SupportedTextureFormats)/sizeof(const char*);j++) {
			LoadTerrainTextures(i,SupportedTextureFormats[j]);
		}
	}
}

