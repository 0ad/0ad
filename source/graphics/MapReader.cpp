#include "precompiled.h"

#include "types.h"
#include "MapReader.h"
#include "UnitManager.h"
#include "ObjectManager.h"
#include "BaseEntity.h"
#include "BaseEntityCollection.h"
#include "EntityManager.h"

#include "Model.h"
#include "Terrain.h"
#include "TextureManager.h"

// CMapReader constructor: nothing to do at the minute
CMapReader::CMapReader()
{
}

// LoadMap: try to load the map from given file; reinitialise the scene to new data if successful
void CMapReader::LoadMap(const char* filename, CTerrain *pTerrain, CUnitManager *pUnitMan, CLightEnv *pLightEnv)
{
	CFileUnpacker unpacker;
	unpacker.Read(filename,"PSMP");

	// check version 
	if (unpacker.GetVersion()<FILE_READ_VERSION) {
		throw CFileUnpacker::CFileVersionError();
	}

	// unpack the data
	UnpackMap(unpacker);

	// finally, apply data to the world
	ApplyData(unpacker, pTerrain, pUnitMan, pLightEnv);
}

// UnpackMap: unpack the given data from the raw data stream into local variables
void CMapReader::UnpackMap(CFileUnpacker& unpacker)
{
	// now unpack everything into local data
	UnpackTerrain(unpacker);
	UnpackObjects(unpacker);
	if (unpacker.GetVersion()>=2) {
		UnpackLightEnv(unpacker);
	}
}

// UnpackLightEnv: unpack lighting parameters from input stream
void CMapReader::UnpackLightEnv(CFileUnpacker& unpacker)
{
	unpacker.UnpackRaw(&m_LightEnv.m_SunColor,sizeof(m_LightEnv.m_SunColor));
	unpacker.UnpackRaw(&m_LightEnv.m_Elevation,sizeof(m_LightEnv.m_Elevation));
	unpacker.UnpackRaw(&m_LightEnv.m_Rotation,sizeof(m_LightEnv.m_Rotation));
	unpacker.UnpackRaw(&m_LightEnv.m_TerrainAmbientColor,sizeof(m_LightEnv.m_TerrainAmbientColor));
	unpacker.UnpackRaw(&m_LightEnv.m_UnitsAmbientColor,sizeof(m_LightEnv.m_UnitsAmbientColor));
    m_LightEnv.CalculateSunDirection();
}

// UnpackObjects: unpack world objects from input stream
void CMapReader::UnpackObjects(CFileUnpacker& unpacker)
{
	// unpack object types
	u32 numObjTypes;
	unpacker.UnpackRaw(&numObjTypes,sizeof(numObjTypes));	
	m_ObjectTypes.resize(numObjTypes);
	for (uint i=0;i<numObjTypes;i++) {
		CStr objname;
		unpacker.UnpackString(objname);

		CObjectEntry* object=g_ObjMan.FindObject((const char*) objname);
		m_ObjectTypes[i]=object;
	}
	
	// unpack object data
	u32 numObjects;
	unpacker.UnpackRaw(&numObjects,sizeof(numObjects));	
	m_Objects.resize(numObjects);
	unpacker.UnpackRaw(&m_Objects[0],sizeof(SObjectDesc)*numObjects);	
}

// UnpackTerrain: unpack the terrain from the end of the input data stream
//		- data: map size, heightmap, list of textures used by map, texture tile assignments
void CMapReader::UnpackTerrain(CFileUnpacker& unpacker)
{
	// unpack map size
	unpacker.UnpackRaw(&m_MapSize,sizeof(m_MapSize));	
	
	// unpack heightmap
	u32 verticesPerSide=m_MapSize*PATCH_SIZE+1;
	m_Heightmap.resize(SQR(verticesPerSide));
	unpacker.UnpackRaw(&m_Heightmap[0],SQR(verticesPerSide)*sizeof(u16));	
	
	// unpack texture names; find handle for each texture
	u32 numTextures;
	unpacker.UnpackRaw(&numTextures,sizeof(numTextures));	

	m_TerrainTextures.reserve(numTextures);
	for (uint i=0;i<numTextures;i++) {
		CStr texturename;
		unpacker.UnpackString(texturename);

		Handle handle;
		CTextureEntry* texentry=g_TexMan.FindTexture(texturename);
		if (!texentry) {
			// ack; mismatch between texture datasets?
			handle=0;
		} else {
			handle=texentry->GetHandle();
		}
		m_TerrainTextures.push_back(handle);
	}
	
	// unpack tile data
	u32 tilesPerSide=m_MapSize*PATCH_SIZE;
	m_Tiles.resize(SQR(tilesPerSide));
	unpacker.UnpackRaw(&m_Tiles[0],(u32)(sizeof(STileDesc)*m_Tiles.size()));	
}

// ApplyData: take all the input data, and rebuild the scene from it
void CMapReader::ApplyData(CFileUnpacker& unpacker, CTerrain *pTerrain, CUnitManager *pUnitMan, CLightEnv *pLightEnv)
{
	// initialise the terrain 
	pTerrain->Initialize(m_MapSize,&m_Heightmap[0]);	

	// setup the textures on the minipatches
	STileDesc* tileptr=&m_Tiles[0];
	for (u32 j=0;j<m_MapSize;j++) {
		for (u32 i=0;i<m_MapSize;i++) {
			for (u32 m=0;m<PATCH_SIZE;m++) {
				for (u32 k=0;k<PATCH_SIZE;k++) {
					CMiniPatch& mp=pTerrain->GetPatch(i,j)->m_MiniPatches[m][k];
					
					mp.Tex1=m_TerrainTextures[tileptr->m_Tex1Index];
					mp.Tex1Priority=tileptr->m_Priority;

					tileptr++;
				}
			}
		}
	}

	// empty out existing units
	pUnitMan->DeleteAll();
	
	// add new objects
	for (u32 i=0;i<m_Objects.size();i++) {
		CObjectEntry* objentry=m_ObjectTypes[m_Objects[i].m_ObjectIndex];
		if (objentry && objentry->m_Model) {
			// Hijack the standard actor instantiation for actors that correspond to entities.
			// Not an ideal solution; we'll have to figure out a map format that can define entities seperately or somesuch.

			CBaseEntity* templateObject = g_EntityTemplateCollection.getTemplateByActor( objentry );
			
			if( templateObject )
			{
				CVector3D orient = ((CMatrix3D*)m_Objects[i].m_Transform)->GetIn();
				CVector3D position = ((CMatrix3D*)m_Objects[i].m_Transform)->GetTranslation();

				g_EntityManager.create( templateObject, position, atan2( -orient.X, -orient.Z ) );
			}
			else
			{
				CUnit* unit=new CUnit(objentry,objentry->m_Model->Clone());
			
				CMatrix3D transform;
				memcpy(&transform._11,m_Objects[i].m_Transform,sizeof(float)*16);
				unit->GetModel()->SetTransform(transform);
				
				// add this unit to list of units stored in unit manager
				pUnitMan->AddUnit(unit);
			}
		}
	}

	if (unpacker.GetVersion()>=2) {
		// copy over the lighting parameters
		*pLightEnv=m_LightEnv;
	}
}
