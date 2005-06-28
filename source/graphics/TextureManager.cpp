#include "precompiled.h"

#include <algorithm>
#include <vector>

#include "TextureManager.h"
#include "TextureEntry.h"

#include "res/res.h"
#include "res/ogl_tex.h"
#include "ogl.h"
#include "timer.h"

#include "CLogger.h"
#include "Xeromyces.h"
#include "XeroXMB.h"

#define LOG_CATEGORY "graphics"

using namespace std;

CTextureManager::CTextureManager():
	m_LastGroupIndex(0)
{}

CTextureManager::~CTextureManager()
{
	for (size_t i=0;i<m_TextureEntries.size();i++) {
		delete m_TextureEntries[i];
	}
	
	TerrainTypeGroupMap::iterator it=m_TerrainTypeGroups.begin();
	while (it != m_TerrainTypeGroups.end())
	{
		delete it->second;
		++it;
	}
}

CTextureEntry* CTextureManager::FindTexture(CStr tag)
{
	for (uint i=0;i<m_TextureEntries.size();i++)
	{
		if (m_TextureEntries[i]->GetTag() == tag)
			return m_TextureEntries[i];
	}

	return 0;
}

CTextureEntry* CTextureManager::FindTexture(Handle handle)
{
	for (uint i=0;i<m_TextureEntries.size();i++)
	{
		// Don't bother looking at textures that haven't been loaded yet - since
		// the caller has given us a Handle to the texture, it must be loaded.
		// (This matters because GetHandle would load the texture, even though
		// there's no need to.)
		if (m_TextureEntries[i]->IsLoaded()
			&& handle==m_TextureEntries[i]->GetHandle())
		{
			return m_TextureEntries[i];
		}
	}

	return 0;
}

void CTextureManager::LoadTerrainsFromXML(const char *filename)
{
	CXeromyces XeroFile;
	if (XeroFile.Load(filename) != PSRETURN_OK)
		return;

	XMBElement root = XeroFile.getRoot();
	CStr rootName = XeroFile.getElementString(root.getNodeName());

	// Check that we've got the right kind of xml document
	if (rootName != "terrains")
	{
		LOG(ERROR,
			LOG_CATEGORY,
			"TextureManager: Loading %s: Root node is not terrains (found \"%s\")",
			filename,
			rootName.c_str());
		return;
	}
	
	#define ELMT(x) int el_##x = XeroFile.getElementID(#x)
	#define ATTR(x) int at_##x = XeroFile.getAttributeID(#x)
	ELMT(terrain);
	#undef ELMT
	#undef ATTR
	
	// Load terrains

	// Iterate main children
	//  they should all be <object> or <script> elements
	XMBElementList children = root.getChildNodes();
	for (int i=0; i<children.Count; ++i)
	{
		//debug_printf("Object %d\n", i);
		XMBElement child = children.item(i);

		if (child.getNodeName() == el_terrain)
		{
			CTextureEntry *pEntry = CTextureEntry::FromXML(child, &XeroFile);
			if (pEntry)
				m_TextureEntries.push_back(pEntry);
		}
		else
		{
			LOG(WARNING, LOG_CATEGORY, 
				"TextureManager: Loading %s: Unexpected node %s\n",
				filename,
				XeroFile.getElementString(child.getNodeName()).c_str());
			// Keep reading - typos shouldn't be showstoppers
		}
	}
}

/*CTextureEntry* CTextureManager::AddTexture(const char* filename,int type)
{
	debug_assert((uint)type<m_TerrainTextures.size());

	// create new texture entry
	CTextureEntry* texentry=new CTextureEntry(filename,type);

	// add entry to list ..
	m_TerrainTextures[type].m_Textures.push_back(texentry);

	return texentry;
}*/

void CTextureManager::DeleteTexture(CTextureEntry* entry)
{
	typedef std::vector<CTextureEntry*>::iterator Iter;
	Iter i=std::find(m_TextureEntries.begin(),m_TextureEntries.end(),entry);
	if (i!=m_TextureEntries.end()) {
		m_TextureEntries.erase(i);
	}
	delete entry;
}

void CTextureManager::RecurseDirectory(CStr path)
{
	LOG(NORMAL, LOG_CATEGORY, "CTextureManager::RecurseDirectory(%s)", path.c_str());

	Handle dir=vfs_open_dir(path.c_str());
	vfsDirEnt dent;

	if (dir > 0)
	{
		while (vfs_next_dirent(dir, &dent, "/") == 0)
		{
			RecurseDirectory(path+dent.name+"/");
		}
		
		vfs_close_dir(dir);
	}

	dir=vfs_open_dir(path.c_str());

	if (dir > 0)
	{
		while (vfs_next_dirent(dir, &dent, "*.xml") == 0)
		{
			CStr xmlFileName = path+dent.name;
			LOG(NORMAL, LOG_CATEGORY, "CTextureManager::LoadTerrainTextures(): loading terrain XML %s", xmlFileName.c_str());
			LoadTerrainsFromXML(xmlFileName);
		}
		
		vfs_close_dir(dir);
	}
}

int CTextureManager::LoadTerrainTextures()
{
	RecurseDirectory("art/textures/terrain/types/");
	
	std::vector<CTextureEntry *>::iterator it=m_TextureEntries.begin();
	for (;it != m_TextureEntries.end();++it)
		(*it)->LoadParent();
	
	return 0;
}

CTerrainTypeGroup *CTextureManager::FindGroup(CStr name)
{
	TerrainTypeGroupMap::const_iterator it=m_TerrainTypeGroups.find(name);
	if (it != m_TerrainTypeGroups.end())
		return it->second;
	else
		return m_TerrainTypeGroups[name] = new CTerrainTypeGroup(name, ++m_LastGroupIndex);
}

/* There was a GetRandomTexture in MainFrm.cpp (sced) previously that gave compile errors...
So I thought "better fix it up and put it in CTextureManager instead".. well, it is never used
except for one *comment* in MainFrm.cpp - d'oh */
CTextureEntry* CTextureManager::GetRandomTexture()
{
	if (!m_TextureEntries.size())
		return NULL;

	u32 type=rand()%(u32)m_TextureEntries.size();
	return m_TextureEntries[type];
}

void CTerrainTypeGroup::AddTerrain(CTextureEntry *pTerrain)
{
	m_Terrains.push_back(pTerrain);
}

void CTerrainTypeGroup::RemoveTerrain(CTextureEntry *pTerrain)
{
	vector<CTextureEntry *>::iterator it;
	it=find(m_Terrains.begin(), m_Terrains.end(), pTerrain);
	if (it != m_Terrains.end())
		m_Terrains.erase(it);
}
