// switch off warnings before including stl files
#pragma warning(disable : 4786) // identifier truncated to 255 chars

#include "Types.h"
#include "MapWriter.h"
#include "UnitManager.h"
#include "ObjectManager.h"
#include "Model.h"
#include "Terrain.h"
#include "LightEnv.h"
#include "TextureManager.h"

extern CTerrain g_Terrain;
extern CLightEnv g_LightEnv;

#include <set>
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
// CMapWriter constructor: nothing to do at the minute
CMapWriter::CMapWriter()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// SaveMap: try to save the current map to the given file
void CMapWriter::SaveMap(const char* filename)
{
	CFilePacker packer;

	// build necessary data
	PackMap(packer);

	// write it out
	packer.Write(filename,FILE_VERSION,"PSMP");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// GetHandleIndex: return the index of the given handle in the given list; or 0xffff if
// handle isn't in list
static u16 GetHandleIndex(const Handle handle,const std::vector<Handle>& handles)
{
	for (uint i=0;i<handles.size();i++) {
		if (handles[i]==handle) {
			return i;
		}
	}

	return 0xffff;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// GetObjectIndex: return the index of the given object in the given list; or 0xffff if
// object isn't in list
static u16 GetObjectIndex(const CObjectEntry* object,const std::vector<CObjectEntry*>& objects)
{
	for (uint i=0;i<objects.size();i++) {
		if (objects[i]==object) {
			return i;
		}
	}

	return 0xffff;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// EnumTerrainTextures: build lists of textures used by map, and tile descriptions for 
// each tile on the terrain
void CMapWriter::EnumTerrainTextures(std::vector<CStr>& textures,
									 std::vector<STileDesc>& tiles)
{
	// the list of all handles in use
	std::vector<Handle> handles;
	
	// resize tile array to required size
	tiles.resize(SQR(g_Terrain.GetVerticesPerSide()-1));
	STileDesc* tileptr=&tiles[0];

	// now iterate through all the tiles
	u32 mapsize=g_Terrain.GetPatchesPerSide();
	for (u32 j=0;j<mapsize;j++) {
		for (u32 i=0;i<mapsize;i++) {
			for (u32 m=0;m<PATCH_SIZE;m++) {
				for (u32 k=0;k<PATCH_SIZE;k++) {
					CMiniPatch& mp=g_Terrain.GetPatch(i,j)->m_MiniPatches[m][k];
					u16 index=u16(GetHandleIndex(mp.Tex1,handles));
					if (index==0xffff) {
						index=handles.size();
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
	for (uint i=0;i<handles.size();i++) {
		CStr texturename;
		CTextureEntry* texentry=g_TexMan.FindTexture(handles[i]);
		if (!texentry) {
			// uh-oh, this shouldn't happen; set texturename to empty string
			texturename="";
		} else {
			texturename=texentry->m_Name;
		}
		textures.push_back(texturename);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// EnumObjects: build lists of object types used by map, and object descriptions for 
// each object in the world
void CMapWriter::EnumObjects(std::vector<CStr>& objectTypes,std::vector<SObjectDesc>& objects)
{
	// the list of all object entries in use
	std::vector<CObjectEntry*> objectsInUse;
	
	// resize object array to required size
	const std::vector<CUnit*>& units=g_UnitMan.GetUnits();
	objects.resize(units.size());
	SObjectDesc* objptr=&objects[0];

	// now iterate through all the units
	for (u32 j=0;j<units.size();j++) {
		CUnit* unit=units[j];

		u16 index=u16(GetObjectIndex(unit->GetObject(),objectsInUse));
		if (index==0xffff) {
			index=objectsInUse.size();
			objectsInUse.push_back(unit->GetObject());
		}

		objptr->m_ObjectIndex=index;
		memcpy(objptr->m_Transform,&unit->GetModel()->GetTransform()._11,sizeof(float)*16);
		objptr++;
	}

	// now build outgoing objectTypes array
	for (uint i=0;i<objectsInUse.size();i++) {
		objectTypes.push_back(objectsInUse[i]->m_Name);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// PackMap: pack the current world into a raw data stream
void CMapWriter::PackMap(CFilePacker& packer)
{
	// now pack everything up
	PackTerrain(packer);
	PackObjects(packer);
	PackLightEnv(packer);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// PackLightEnv: pack lighting parameters onto the end of the output data stream
void CMapWriter::PackLightEnv(CFilePacker& packer)
{
	packer.PackRaw(&g_LightEnv.m_SunColor,sizeof(g_LightEnv.m_SunColor));
	packer.PackRaw(&g_LightEnv.m_Elevation,sizeof(g_LightEnv.m_Elevation));
	packer.PackRaw(&g_LightEnv.m_Rotation,sizeof(g_LightEnv.m_Rotation));
	packer.PackRaw(&g_LightEnv.m_TerrainAmbientColor,sizeof(g_LightEnv.m_TerrainAmbientColor));
	packer.PackRaw(&g_LightEnv.m_UnitsAmbientColor,sizeof(g_LightEnv.m_UnitsAmbientColor));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// PackObjects: pack world objects onto the end of the output data stream
//		- data: list of objects types used by map, list of object descriptions
void CMapWriter::PackObjects(CFilePacker& packer)
{
	// the list of object types used by map
	std::vector<CStr> objectTypes;
	// descriptions of each object
	std::vector<SObjectDesc> objects;
	
	// build lists by scanning through the world
	EnumObjects(objectTypes,objects);

	// pack object types
	u32 numObjTypes=objectTypes.size();
	packer.PackRaw(&numObjTypes,sizeof(numObjTypes));	
	for (uint i=0;i<numObjTypes;i++) {
		packer.PackString(objectTypes[i]);
	}
	
	// pack object data
	u32 numObjects=objects.size();
	packer.PackRaw(&numObjects,sizeof(numObjects));	
	packer.PackRaw(&objects[0],sizeof(SObjectDesc)*numObjects);	
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// PackTerrain: pack the terrain onto the end of the output data stream
//		- data: map size, heightmap, list of textures used by map, texture tile assignments
void CMapWriter::PackTerrain(CFilePacker& packer)
{
	// pack map size
	u32 mapsize=g_Terrain.GetPatchesPerSide();
	packer.PackRaw(&mapsize,sizeof(mapsize));	
	
	// pack heightmap
	packer.PackRaw(g_Terrain.GetHeightMap(),sizeof(u16)*SQR(g_Terrain.GetVerticesPerSide()));	

	// the list of textures used by map
	std::vector<CStr> terrainTextures;
	// descriptions of each tile
	std::vector<STileDesc> tiles;
	
	// build lists by scanning through the terrain
	EnumTerrainTextures(terrainTextures,tiles);
	
	// pack texture names
	u32 numTextures=terrainTextures.size();
	packer.PackRaw(&numTextures,sizeof(numTextures));	
	for (uint i=0;i<numTextures;i++) {
		packer.PackString(terrainTextures[i]);
	}
	
	// pack tile data
	packer.PackRaw(&tiles[0],sizeof(STileDesc)*tiles.size());	
}

