#include "precompiled.h"

#include "types.h"
#include "MapWriter.h"
#include "UnitManager.h"
#include "ObjectManager.h"
#include "Model.h"
#include "Terrain.h"
#include "LightEnv.h"
#include "TextureManager.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// CMapWriter constructor: nothing to do at the minute
CMapWriter::CMapWriter()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// SaveMap: try to save the current map to the given file
void CMapWriter::SaveMap(const char* filename, CTerrain *pTerrain, CLightEnv *pLightEnv, CUnitManager *pUnitMan)
{
	CFilePacker packer;

	// build necessary data
	PackMap(packer, pTerrain, pLightEnv, pUnitMan);

	// write it out
	packer.Write(filename,FILE_VERSION,"PSMP");
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
// GetObjectIndex: return the index of the given object in the given list; or 0xffff if
// object isn't in list
static u16 GetObjectIndex(const CObjectEntry* object,const std::vector<CObjectEntry*>& objects)
{
	for (uint i=0;i<(uint)objects.size();i++) {
		if (objects[i]==object) {
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
// EnumObjects: build lists of object types used by map, and object descriptions for 
// each object in the world
void CMapWriter::EnumObjects(CUnitManager *pUnitMan,
	std::vector<CStr>& objectTypes, std::vector<SObjectDesc>& objects)
{
	// the list of all object entries in use
	std::vector<CObjectEntry*> objectsInUse;
	
	// resize object array to required size
	const std::vector<CUnit*>& units=pUnitMan->GetUnits();
	objects.resize(units.size());
	SObjectDesc* objptr=&objects[0];

	// now iterate through all the units
	for (u32 j=0;j<(u32)units.size();j++) {
		CUnit* unit=units[j];

		u16 index=u16(GetObjectIndex(unit->GetObject(),objectsInUse));
		if (index==0xffff) {
			index=(u16)objectsInUse.size();
			objectsInUse.push_back(unit->GetObject());
		}

		objptr->m_ObjectIndex=index;
		memcpy(objptr->m_Transform,&unit->GetModel()->GetTransform()._11,sizeof(float)*16);
		objptr++;
	}

	// now build outgoing objectTypes array
	for (uint i=0;i<(uint)objectsInUse.size();i++) {
		objectTypes.push_back(objectsInUse[i]->m_Name);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// PackMap: pack the current world into a raw data stream
void CMapWriter::PackMap(CFilePacker& packer, CTerrain *pTerrain, CLightEnv *pLightEnv, CUnitManager *pUnitMan)
{
	// now pack everything up
	PackTerrain(packer, pTerrain);
	PackObjects(packer, pUnitMan);
	PackLightEnv(packer, pLightEnv);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// PackLightEnv: pack lighting parameters onto the end of the output data stream
void CMapWriter::PackLightEnv(CFilePacker& packer, CLightEnv *pLightEnv)
{
	packer.PackRaw(&pLightEnv->m_SunColor,sizeof(pLightEnv->m_SunColor));
	packer.PackRaw(&pLightEnv->m_Elevation,sizeof(pLightEnv->m_Elevation));
	packer.PackRaw(&pLightEnv->m_Rotation,sizeof(pLightEnv->m_Rotation));
	packer.PackRaw(&pLightEnv->m_TerrainAmbientColor,sizeof(pLightEnv->m_TerrainAmbientColor));
	packer.PackRaw(&pLightEnv->m_UnitsAmbientColor,sizeof(pLightEnv->m_UnitsAmbientColor));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// PackObjects: pack world objects onto the end of the output data stream
//		- data: list of objects types used by map, list of object descriptions
void CMapWriter::PackObjects(CFilePacker& packer, CUnitManager *pUnitMan)
{
	// the list of object types used by map
	std::vector<CStr> objectTypes;
	// descriptions of each object
	std::vector<SObjectDesc> objects;
	
	// build lists by scanning through the world
	EnumObjects(pUnitMan, objectTypes, objects);

	// pack object types
	u32 numObjTypes=(u32)objectTypes.size();
	packer.PackRaw(&numObjTypes,sizeof(numObjTypes));	
	for (uint i=0;i<numObjTypes;i++) {
		packer.PackString(objectTypes[i]);
	}
	
	// pack object data
	u32 numObjects=(u32)objects.size();
	packer.PackRaw(&numObjects,sizeof(numObjects));	
	packer.PackRaw(&objects[0],sizeof(SObjectDesc)*numObjects);	
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

