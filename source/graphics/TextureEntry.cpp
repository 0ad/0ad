#include "precompiled.h"

#include <string>

#include "ogl.h"
#include "res/ogl_tex.h"

#include "CLogger.h"

#include "TextureEntry.h"
#include "TextureManager.h"
#include "Texture.h"
#include "Renderer.h"
#include "Overlay.h"

#include "Parser.h"
#include "XeroXMB.h"
#include "Xeromyces.h"

using namespace std;

#define LOG_CATEGORY "graphics"

/////////////////////////////////////////////////////////////////////////////////////
// CTextureEntry constructor
CTextureEntry::CTextureEntry():
	m_pParent(NULL),
	m_Bitmap(NULL),
	m_Handle(-1),
	m_BaseColor(0),
	m_BaseColorValid(false)
{}

/////////////////////////////////////////////////////////////////////////////////////
// CTextureEntry destructor
CTextureEntry::~CTextureEntry()
{
	if (m_Handle > 0)
		tex_free(m_Handle);
	
	GroupVector::iterator it=m_Groups.begin();
	for (;it!=m_Groups.end();++it)
		(*it)->RemoveTerrain(this);
}

CTextureEntry *CTextureEntry::FromXML(XMBElement node, CXeromyces *pFile)
{
	CTextureEntry *thiz=new CTextureEntry();

	#define ELMT(x) int elmt_##x = pFile->getElementID(#x)
	#define ATTR(x) int attr_##x = pFile->getAttributeID(#x)
	ELMT(doodad);
	ELMT(passable);
	ELMT(event);
	// Terrain Attribs
	ATTR(tag);
	ATTR(skin);
	ATTR(parent);
	ATTR(mmap);
	ATTR(groups);
	ATTR(properties);
	// Passable Attribs
	ATTR(type);
	ATTR(speed);
	ATTR(effect);
	// Doodad Attribs
	ATTR(name);
	ATTR(max);
	// Event attribs
	ATTR(on);
	#undef ELMT
	#undef ATTR
	
	XMBAttributeList attribs = node.getAttributes();
	for (int i=0;i<attribs.Count;i++)
	{
		XMBAttribute attr = attribs.item(i);

		if (attr.Name == attr_tag)
			thiz->m_Tag = (CStr)attr.Value;
		if (attr.Name == attr_parent)
			thiz->m_ParentName = (CStr)attr.Value;
		else if (attr.Name == attr_skin)
		{
			thiz->m_TexturePath = (CStr)attr.Value;
		}
		else if (attr.Name == attr_groups)
		{
			// Parse a comma-separated list of groups, add the new entry to
			// each of them
			CParser parser;
			CParserLine parserLine;
			parser.InputTaskType("GroupList", "<_$value_,>_$value_");
			
			if (!parserLine.ParseString(parser, (CStr)attr.Value))
				continue;
			for (size_t i=0;i<parserLine.GetArgCount();i++)
			{
				string value;
				if (!parserLine.GetArgString(i, value))
					continue;
				CTerrainTypeGroup *pType = g_TexMan.FindGroup(value);
				thiz->m_Groups.push_back(pType);
				pType->AddTerrain(thiz);
			}
		}
		else if (attr.Name == attr_mmap)
		{
			CColor col;
			if (!col.ParseString((CStr)attr.Value, 255))
				continue;
			
			// m_BaseColor is RGBA
			u8 (&baseColor)[4] = *(u8 (*)[4])&thiz->m_BaseColor;
			baseColor[0] = (u8)(col.r*255);
			baseColor[1] = (u8)(col.g*255);
			baseColor[2] = (u8)(col.b*255);
			baseColor[3] = (u8)(col.a*255);
			thiz->m_BaseColorValid = true;
		}
		else if (attr.Name == attr_properties)
		{
			// TODO Parse a list of properties and store them somewhere
		}
	}
	
	// TODO Parse information in child nodes and store them somewhere
	
	LOG(NORMAL, LOG_CATEGORY, "CTextureEntry::FromXML(): Terrain type %s (texture %s) loaded", thiz->m_Tag.c_str(), thiz->m_TexturePath.c_str());
	
	return thiz;
}

void CTextureEntry::LoadParent()
{
	// Don't do anything at all if we already have a parent, or if we are a root
	// entry
	if (m_pParent || !m_ParentName.size())
		return;
	
	m_pParent = g_TexMan.FindTexture(m_ParentName);
	if (!m_pParent)
	{
		LOG(ERROR, LOG_CATEGORY, "CTextureEntry::LoadParent(): %s: Could not find parent terrain type %s", m_Tag.c_str(), m_ParentName.c_str());
		return;
	}
	m_pParent->LoadParent(); // Make sure we're loaded all the way to our root
}

/////////////////////////////////////////////////////////////////////////////////////
// LoadTexture: actually load the texture resource from file
void CTextureEntry::LoadTexture()	
{
	if (m_TexturePath.size() == 0)
	{
		debug_assert(m_pParent);
		m_pParent->LoadTexture();
		m_Handle = m_pParent->m_Handle;
		return;
	}
	else
	{
		CTexture texture;
		texture.SetName(m_TexturePath);
		if (g_Renderer.LoadTexture(&texture,GL_REPEAT)) {
			m_Handle=texture.GetHandle();
		} else {
			m_Handle=0;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BuildBaseColor: calculate the root color of the texture, used for coloring minimap, and store
// in m_BaseColor member
void CTextureEntry::BuildBaseColor()
{
	// a) We have an XML override: m_BaseColorValid=true, so we never get here
	// b) No XML override, overriden texture: use own texture, calc. BaseColor
	
	// c) No XML override, parent texture: use parent BaseColor
	// d) No XML override, but parent has: use parent BaseColor
	
	// cases c&d: We don't have our own texture, use parent base color instead
	if (m_TexturePath.size() == 0)
	{
		debug_assert(m_pParent);
		m_BaseColor=m_pParent->GetBaseColor();
		m_BaseColorValid=true;
		return;
	}

	// case b: Calculate base color by mipmapping our texture
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
