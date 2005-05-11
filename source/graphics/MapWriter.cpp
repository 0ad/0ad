#include "precompiled.h"

#include "lib/types.h"
#include "MapWriter.h"
#include "MapReader.h"
#include "UnitManager.h"
#include "Unit.h"
#include "ObjectManager.h"
#include "ObjectBase.h"
#include "Model.h"
#include "Terrain.h"
#include "LightEnv.h"
#include "TextureManager.h"
#include "VFSUtil.h"
#include "Loader.h"

#include "ps/XMLWriter.h"
#include "lib/res/vfs.h"
#include "simulation/Entity.h"
#include "simulation/BaseEntity.h"
#include "simulation/BaseEntityCollection.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// CMapWriter constructor: nothing to do at the minute
CMapWriter::CMapWriter()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// SaveMap: try to save the current map to the given file
void CMapWriter::SaveMap(const char* filename, CTerrain *pTerrain, CLightEnv *pLightEnv, CUnitManager *pUnitMan)
{
	CFilePacker packer(FILE_VERSION, "PSMP");

	// build necessary data
	PackMap(packer, pTerrain, pLightEnv, pUnitMan);

	// write it out
	packer.Write(filename);

	CStr filename_xml (filename);
	filename_xml = filename_xml.Left(filename_xml.Length()-4) + ".xml";
	WriteXML(filename_xml, pUnitMan, pLightEnv);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// GetHandleIndex: return the index of the given handle in the given list; or 0xffff if
// handle isn't in list
static u16 GetHandleIndex(const Handle handle,const std::vector<Handle>& handles)
{
	for (uint i=0;i<(uint)handles.size();i++) {
		if (handles[i]==handle) {
			return i;
		}
	}

	return 0xffff;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// EnumTerrainTextures: build lists of textures used by map, and tile descriptions for 
// each tile on the terrain
void CMapWriter::EnumTerrainTextures(CTerrain *pTerrain,
									 std::vector<CStr>& textures,
									 std::vector<STileDesc>& tiles)
{
	// the list of all handles in use
	std::vector<Handle> handles;
	
	// resize tile array to required size
	tiles.resize(SQR(pTerrain->GetVerticesPerSide()-1));
	STileDesc* tileptr=&tiles[0];

	// now iterate through all the tiles
	u32 mapsize=pTerrain->GetPatchesPerSide();
	for (u32 j=0;j<mapsize;j++) {
		for (u32 i=0;i<mapsize;i++) {
			for (u32 m=0;m<PATCH_SIZE;m++) {
				for (u32 k=0;k<PATCH_SIZE;k++) {
					CMiniPatch& mp=pTerrain->GetPatch(i,j)->m_MiniPatches[m][k];
					u16 index=u16(GetHandleIndex(mp.Tex1,handles));
					if (index==0xffff) {
						index=(u16)handles.size();
						handles.push_back(mp.Tex1);
					}

					tileptr->m_Tex1Index=index;
					tileptr->m_Tex2Index=0xffff;
					tileptr->m_Priority=mp.Tex1Priority;
					tileptr++;
				}
			}
		}
	}

	// now find the texture names for each handle
	for (uint i=0;i<(uint)handles.size();i++) {
		CStr texturename;
		CTextureEntry* texentry=g_TexMan.FindTexture(handles[i]);
		if (!texentry) {
			// uh-oh, this shouldn't happen; set texturename to empty string
			texturename="";
		} else {
			texturename=texentry->GetName();
		}
		textures.push_back(texturename);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// PackMap: pack the current world into a raw data stream
void CMapWriter::PackMap(CFilePacker& packer, CTerrain *pTerrain, CLightEnv *pLightEnv, CUnitManager *pUnitMan)
{
	// now pack everything up
	PackTerrain(packer, pTerrain);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// PackTerrain: pack the terrain onto the end of the output data stream
//		- data: map size, heightmap, list of textures used by map, texture tile assignments
void CMapWriter::PackTerrain(CFilePacker& packer, CTerrain *pTerrain)
{
	// pack map size
	u32 mapsize=pTerrain->GetPatchesPerSide();
	packer.PackRaw(&mapsize,sizeof(mapsize));	
	
	// pack heightmap
	packer.PackRaw(pTerrain->GetHeightMap(),sizeof(u16)*SQR(pTerrain->GetVerticesPerSide()));	

	// the list of textures used by map
	std::vector<CStr> terrainTextures;
	// descriptions of each tile
	std::vector<STileDesc> tiles;
	
	// build lists by scanning through the terrain
	EnumTerrainTextures(pTerrain, terrainTextures, tiles);
	
	// pack texture names
	u32 numTextures=(u32)terrainTextures.size();
	packer.PackRaw(&numTextures,sizeof(numTextures));	
	for (uint i=0;i<numTextures;i++) {
		packer.PackString(terrainTextures[i]);
	}
	
	// pack tile data
	packer.PackRaw(&tiles[0],(u32)(sizeof(STileDesc)*tiles.size()));	
}



void CMapWriter::WriteXML(const char* filename, CUnitManager* pUnitMan, CLightEnv *pLightEnv)
{
	// HACK: ScEd uses non-VFS filenames, so just use fopen instead of vfs_open
#ifdef SCED
	FILE* f = fopen(filename, "wb");
	if (! f)
	{
		debug_warn("Failed to open map XML file");
		return;
	}
#else
	Handle h = vfs_open(filename, FILE_WRITE|FILE_NO_AIO);
	if (h <= 0)
	{
		debug_warn("Failed to open map XML file");
		return;
	}
#endif

	XML_Start("utf-8");
	// DTDs are rather annoying. They ought to be strict in what they accept,
	// else they serve no purpose (given that the only purpose is complaining
	// nicely when someone provides invalid input). But then it can't be
	// backwards-compatible, because the old data files don't follow the new
	// format, and there's no way we're going to rebuild all the old data
	// files. So, just create an entirely new DTD for each revision of the
	// format:
	XML_Doctype("Scenario", "/maps/scenario_v4.dtd");

	{
		XML_Element("Scenario");

		{
			XML_Element("Environment");

			{
				XML_Element("SunColour");
				XML_Attribute("r", pLightEnv->m_SunColor.X); // yes, it's X/Y/Z...
				XML_Attribute("g", pLightEnv->m_SunColor.Y);
				XML_Attribute("b", pLightEnv->m_SunColor.Z);
			}
			{
				XML_Element("SunElevation");
				XML_Attribute("angle", pLightEnv->m_Elevation);
			}
			{
				XML_Element("SunRotation");
				XML_Attribute("angle", pLightEnv->m_Rotation);
			}
			{
				XML_Element("TerrainAmbientColour");
				XML_Attribute("r", pLightEnv->m_TerrainAmbientColor.X);
				XML_Attribute("g", pLightEnv->m_TerrainAmbientColor.Y);
				XML_Attribute("b", pLightEnv->m_TerrainAmbientColor.Z);
			}
			{
				XML_Element("UnitsAmbientColour");
				XML_Attribute("r", pLightEnv->m_UnitsAmbientColor.X);
				XML_Attribute("g", pLightEnv->m_UnitsAmbientColor.Y);
				XML_Attribute("b", pLightEnv->m_UnitsAmbientColor.Z);
			}
		}

		{
			XML_Element("Entities");

			const std::vector<CUnit*>& units = pUnitMan->GetUnits();
			for (std::vector<CUnit*>::const_iterator unit = units.begin(); unit != units.end(); ++unit)
			{

				CEntity* entity = (*unit)->GetEntity();

				// Ignore objects that aren't entities
				if (! entity)
					continue;

				XML_Element("Entity");

				XML_Setting("Template", entity->m_base->m_Tag);

				XML_Setting("Player", entity->GetPlayer()->GetPlayerID());

				{
					CVector3D position = entity->m_position;

					XML_Element("Position");
					XML_Attribute("x", position.X);
					XML_Attribute("y", position.Y);
					XML_Attribute("z", position.Z);
				}

				{
					float angle = entity->m_orientation;

					XML_Element("Orientation");
					XML_Attribute("angle", angle);
				}
			}
		}
		{
			XML_Element("Nonentities");

			const std::vector<CUnit*>& units = pUnitMan->GetUnits();
			for (std::vector<CUnit*>::const_iterator unit = units.begin(); unit != units.end(); ++unit) {

				// Ignore objects that are entities
				if ((*unit)->GetEntity())
					continue;

				XML_Element("Nonentity");

				XML_Setting("Actor", (*unit)->GetObject()->m_Base->m_Name);

				{
					CVector3D position = (*unit)->GetModel()->GetTransform().GetTranslation();

					XML_Element("Position");
					XML_Attribute("x", position.X);
					XML_Attribute("y", position.Y);
					XML_Attribute("z", position.Z);
				}

				{
					CVector3D orient = (*unit)->GetModel()->GetTransform().GetIn();
					float angle = atan2(-orient.X, -orient.Z);

					XML_Element("Orientation");
					XML_Attribute("angle", angle);
				}
			}
		}
	}

	// HACK: continued from above
#ifdef SCED
	CStr d = xml_file_.HACK_GetData();
	fwrite(d.data(), d.length(), 1, f);
	fclose(f);
#else
	if (! XML_StoreVFS(h))
	{
		debug_warn("Failed to write map XML file");
	}
	vfs_close(h);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// RewriteAllMaps
void CMapWriter::RewriteAllMaps(CTerrain *pTerrain, CUnitManager *pUnitMan, CLightEnv *pLightEnv)
{
	VFSUtil::FileList files;
	VFSUtil::FindFiles("maps/scenarios", "*.pmp", files);

	for (VFSUtil::FileList::iterator it = files.begin(); it != files.end(); ++it)
	{
		CMapReader* reader = new CMapReader;
		LDR_BeginRegistering();
		reader->LoadMap(*it, pTerrain, pUnitMan, pLightEnv);
		LDR_EndRegistering();
		LDR_NonprogressiveLoad();

		CStr n (*it);
		n.Replace("scenarios/", "scenarios/new/");
		CMapWriter writer;
		writer.SaveMap(n, pTerrain, pLightEnv, pUnitMan);
	}
}
