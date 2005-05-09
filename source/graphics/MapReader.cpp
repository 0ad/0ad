#include "precompiled.h"

#include "MapReader.h"
#include "lib/types.h"
#include "UnitManager.h"
#include "Unit.h"
#include "Game.h"
#include "ObjectManager.h"
#include "BaseEntity.h"
#include "BaseEntityCollection.h"
#include "EntityManager.h"
#include "CLogger.h"

#include "Model.h"
#include "Terrain.h"
#include "TextureManager.h"

#include "timer.h"
#include "Loader.h"
#include "LoaderThunks.h"

#define LOG_CATEGORY "graphics"

CMapReader::CMapReader()
	: xml_reader(0)
{
	cur_terrain_tex = 0;	// important - resets generator state
}


// LoadMap: try to load the map from given file; reinitialise the scene to new data if successful
void CMapReader::LoadMap(const char* filename, CTerrain *pTerrain_, CUnitManager *pUnitMan_, CLightEnv *pLightEnv_)
{
	// latch parameters (held until DelayedLoadFinished)
	pTerrain = pTerrain_;
	pUnitMan = pUnitMan_;
	pLightEnv = pLightEnv_;

	// [25ms]
	unpacker.Read(filename,"PSMP");

	// check version 
	if (unpacker.GetVersion()<FILE_READ_VERSION) {
		throw CFileUnpacker::CFileVersionError();
	}

	// unpack the data
	RegMemFun(this, &CMapReader::UnpackMap, L"CMapReader::UnpackMap", 1900);

	// apply data to the world
	RegMemFun(this, &CMapReader::ApplyData, L"CMapReader::ApplyData", 20);

	if (unpacker.GetVersion()>=3) {
		// read the corresponding XML file
		filename_xml = filename;
		filename_xml = filename_xml.Left(filename_xml.Length()-4) + ".xml";
		RegMemFun(this, &CMapReader::ReadXML, L"CMapReader::ReadXML", 1300);
	}

	RegMemFun(this, &CMapReader::DelayLoadFinished, L"CMapReader::DelayLoadFinished", 5);
}

// UnpackMap: unpack the given data from the raw data stream into local variables
int CMapReader::UnpackMap()
{
	// now unpack everything into local data
	int ret = UnpackTerrain();
	if(ret != 0)	// failed or timed out
		return ret;

	UnpackObjects();
	if (unpacker.GetVersion()>=2) {
		UnpackLightEnv();
	}

	return 0;
}

// UnpackLightEnv: unpack lighting parameters from input stream
void CMapReader::UnpackLightEnv()
{
	unpacker.UnpackRaw(&m_LightEnv.m_SunColor,sizeof(m_LightEnv.m_SunColor));
	unpacker.UnpackRaw(&m_LightEnv.m_Elevation,sizeof(m_LightEnv.m_Elevation));
	unpacker.UnpackRaw(&m_LightEnv.m_Rotation,sizeof(m_LightEnv.m_Rotation));
	unpacker.UnpackRaw(&m_LightEnv.m_TerrainAmbientColor,sizeof(m_LightEnv.m_TerrainAmbientColor));
	unpacker.UnpackRaw(&m_LightEnv.m_UnitsAmbientColor,sizeof(m_LightEnv.m_UnitsAmbientColor));
    m_LightEnv.CalculateSunDirection();
}

// UnpackObjects: unpack world objects from input stream
void CMapReader::UnpackObjects()
{
	// unpack object types
	u32 numObjTypes;
	unpacker.UnpackRaw(&numObjTypes,sizeof(numObjTypes));	
	m_ObjectTypes.resize(numObjTypes);
	for (uint i=0;i<numObjTypes;i++) {
		unpacker.UnpackString(m_ObjectTypes[i]);
	}
	
	// unpack object data
	u32 numObjects;
	unpacker.UnpackRaw(&numObjects,sizeof(numObjects));	
	m_Objects.resize(numObjects);
	unpacker.UnpackRaw(&m_Objects[0],sizeof(SObjectDesc)*numObjects);	
}

// UnpackTerrain: unpack the terrain from the end of the input data stream
//		- data: map size, heightmap, list of textures used by map, texture tile assignments
int CMapReader::UnpackTerrain()
{
	const double end_time = get_time() + 50e-3;

	// first call to generator (this is skipped after first call,
	// i.e. when the loop below was interrupted)
	if(cur_terrain_tex == 0)
	{
		// unpack map size
		unpacker.UnpackRaw(&m_MapSize,sizeof(m_MapSize));

		// unpack heightmap [600µs]
		u32 verticesPerSide=m_MapSize*PATCH_SIZE+1;
		m_Heightmap.resize(SQR(verticesPerSide));
		unpacker.UnpackRaw(&m_Heightmap[0],SQR(verticesPerSide)*sizeof(u16));

		// unpack # textures
		unpacker.UnpackRaw(&num_terrain_tex, sizeof(num_terrain_tex));
		m_TerrainTextures.reserve(num_terrain_tex);
	}

	// unpack texture names; find handle for each texture.
	// interruptible.
	while(cur_terrain_tex < num_terrain_tex)
	{
		CStr texturename;
		unpacker.UnpackString(texturename);

		Handle handle;
		CTextureEntry* texentry = g_TexMan.FindTexture(texturename);
		// mismatch between texture datasets?
		if (!texentry)
			handle = 0;
		else
			handle = texentry->GetHandle();
		m_TerrainTextures.push_back(handle);

		cur_terrain_tex++;
		LDR_CHECK_TIMEOUT(cur_terrain_tex, num_terrain_tex);
	}

	// unpack tile data [3ms]
	u32 tilesPerSide = m_MapSize*PATCH_SIZE;
	m_Tiles.resize(SQR(tilesPerSide));
	unpacker.UnpackRaw(&m_Tiles[0],(u32)(sizeof(STileDesc)*m_Tiles.size()));

	// reset generator state.
	cur_terrain_tex = 0;

	return 0;
}

// ApplyData: take all the input data, and rebuild the scene from it
int CMapReader::ApplyData()
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

	// delete all existing entities
	g_EntityManager.deleteAll();
	// delete all remaining non-entity units
	pUnitMan->DeleteAll();
	
	// add new objects
	for (u32 i=0;i<m_Objects.size();i++) {

		if (unpacker.GetVersion() < 3) {

			debug_warn("Old unsupported map version - objects will be missing");
			// (getTemplateByActor doesn't work, since entity templates are now
			// loaded on demand)

//			// Hijack the standard actor instantiation for actors that correspond to entities.
//			// Not an ideal solution; we'll have to figure out a map format that can define entities separately or somesuch.
//
//			CBaseEntity* templateObject = g_EntityTemplateCollection.getTemplateByActor(m_ObjectTypes.at(m_Objects[i].m_ObjectIndex));
//		
//			if (templateObject)
//			{
//				CVector3D orient = ((CMatrix3D*)m_Objects[i].m_Transform)->GetIn();
//				CVector3D position = ((CMatrix3D*)m_Objects[i].m_Transform)->GetTranslation();
//
//				g_EntityManager.create(templateObject, position, atan2(-orient.X, -orient.Z));
//
//				continue;
//			}
		}

		// MT: Testing:
		if( i == 170 )
		{
			CStrW tom( "dick, harry" );
		}

		CUnit* unit = g_UnitMan.CreateUnit(m_ObjectTypes.at(m_Objects[i].m_ObjectIndex), NULL);

		if (unit)
		{
			CMatrix3D transform;
			memcpy(&transform._11,m_Objects[i].m_Transform,sizeof(float)*16);
			unit->GetModel()->SetTransform(transform);
		}
	}

	if (unpacker.GetVersion()>=2) {
		// copy over the lighting parameters
		*pLightEnv=m_LightEnv;
	}

	return 0;
}




class CXMLReader
{
public:
	CXMLReader(const CStr& xml_filename)
	{
		Init(xml_filename);
	}

	// return semantics: see Loader.cpp!LoadFunc.
	int ProgressiveRead();

private:
	CXeromyces xmb_file;

	int el_scenario, el_entities, el_entity;
	int el_template, el_player;
	int el_position, el_orientation;
	int el_nonentities, el_nonentity;
	int el_actor;
	int at_x, at_y, at_z;
	int at_angle;

	XMBElement root;
	XMBElementList nodes;	// children of root
	XMBElement node;				// a child of nodes
	XMBElementList entities, nonentities;	// children of node

	// loop counters
	int node_idx;
	int entity_idx, nonentity_idx;

	// # entities+nonentities processed and total (for progress calc)
	int completed_jobs, total_jobs;


	void Init(const CStr& xml_filename);
	int ReadEntities(XMBElement& parent, double end_time);
	int ReadNonEntities(XMBElement& parent, double end_time);
};


void CXMLReader::Init(const CStr& xml_filename)
{
	// must only assign once, so do it here
	node_idx = entity_idx = nonentity_idx = 0;

#ifdef SCED
	// HACK: ScEd uses absolute filenames, not VFS paths. I can't be bothered
	// to make Xeromyces work with non-VFS, so just cheat:
	CStr vfs_filename(xml_filename);
	vfs_filename = vfs_filename.substr(vfs_filename.ReverseFind("\\mods\\official\\") + 15);
	vfs_filename.Replace("\\", "/");
	if (xmb_file.Load(vfs_filename) != PSRETURN_OK)
		throw CFileUnpacker::CFileReadError();
#else
	if (xmb_file.Load(xml_filename) != PSRETURN_OK)
		throw CFileUnpacker::CFileReadError();
#endif

	// define all the elements and attributes used in the XML file.
#define EL(x) el_##x = xmb_file.getElementID(#x)
#define AT(x) at_##x = xmb_file.getAttributeID(#x)
	EL(scenario);
	EL(entities);
	EL(entity);
	EL(template);
	EL(player);
	EL(position);
	EL(orientation);
	EL(nonentities);
	EL(nonentity);
	EL(actor);
	AT(x);
	AT(y);
	AT(z);
	AT(angle);
#undef AT
#undef EL

	root = xmb_file.getRoot();
	assert(root.getNodeName() == el_scenario);
	nodes = root.getChildNodes();

	// find out total number of entities+nonentities
	// (used when calculating progress)
	completed_jobs = 0;
	total_jobs = 0;
	for (int i = 0; i < nodes.Count; i++)
	{
		node = nodes.item(i);
		entities = node.getChildNodes();
		total_jobs += entities.Count;
	}
}





int CXMLReader::ReadEntities(XMBElement& parent, double end_time)
{
	entities = parent.getChildNodes();	// ok to set more than once
	while(entity_idx < entities.Count)
	{
		// all new state at this scope and below doesn't need to be
		// wrapped, since we only yield after a complete iteration.

		XMBElement entity = entities.item(entity_idx++);
		assert(entity.getNodeName() == el_entity);

		CStrW TemplateName;
		int PlayerID;
		CVector3D Position;
		float Orientation;

		XMBElementList children3 = entity.getChildNodes();
		for (int k = 0; k < children3.Count; k++)
		{
			XMBElement child3 = children3.item(k);
			int element_name = child3.getNodeName();

			// <template>
			if (element_name == el_template)
			{
				TemplateName = child3.getText();
			}
			// <player>
			else if (element_name == el_player)
			{
				PlayerID = CStr(child3.getText()).ToInt();
			}
			// <position>
			else if (element_name == el_position)
			{
				XMBAttributeList attrs = child3.getAttributes();
				Position = CVector3D(CStr(attrs.getNamedItem(at_x)).ToFloat(),
						                CStr(attrs.getNamedItem(at_y)).ToFloat(),
						                CStr(attrs.getNamedItem(at_z)).ToFloat());
			}
			// <orientation>
			else if (element_name == el_orientation)
			{
				XMBAttributeList attrs = child3.getAttributes();
				Orientation = CStr(attrs.getNamedItem(at_angle)).ToFloat();
			}
			else
				debug_warn("Invalid XML data - DTD shouldn't allow this");
		}

		HEntity ent = g_EntityManager.create(g_EntityTemplateCollection.getTemplate(TemplateName), Position, Orientation);
		if (! ent)
			LOG(ERROR, LOG_CATEGORY, "Failed to create entity '%ls'", TemplateName.c_str());
		else
			ent->SetPlayer(g_Game->GetPlayer(PlayerID));

		completed_jobs++;
		LDR_CHECK_TIMEOUT(completed_jobs, total_jobs);
	}

	return 0;
}


int CXMLReader::ReadNonEntities(XMBElement& parent, double end_time)
{
	nonentities = parent.getChildNodes();	// ok to set more than once
	while(nonentity_idx < nonentities.Count)
	{
		// all new state at this scope and below doesn't need to be
		// wrapped, since we only yield after a complete iteration.

		XMBElement nonentity = nonentities.item(nonentity_idx++);
		assert(nonentity.getNodeName() == el_nonentity);

		CStr ActorName;
		CVector3D Position;
		float Orientation;

		XMBElementList children3 = nonentity.getChildNodes();
		for (int k = 0; k < children3.Count; k++)
		{
			XMBElement child3 = children3.item(k);
			int element_name = child3.getNodeName();

			// <actor>
			if (element_name == el_actor)
			{
				ActorName = child3.getText();
			}
			// <position>
			else if (element_name == el_position)
			{
				XMBAttributeList attrs = child3.getAttributes();
				Position = CVector3D(CStr(attrs.getNamedItem(at_x)).ToFloat(),
					CStr(attrs.getNamedItem(at_y)).ToFloat(),
					CStr(attrs.getNamedItem(at_z)).ToFloat());
			}
			// <orientation>
			else if (element_name == el_orientation)
			{
				XMBAttributeList attrs = child3.getAttributes();
				Orientation = CStr(attrs.getNamedItem(at_angle)).ToFloat();
			}
			else
				debug_warn("Invalid XML data - DTD shouldn't allow this");
		}

		CUnit* unit = g_UnitMan.CreateUnit(ActorName, NULL);
		if (unit && unit->GetModel())
		{
			// Copied from CEntity::updateActorTransforms():
			float s = sin( Orientation );
			float c = cos( Orientation );
			CMatrix3D m;
			m._11 = -c;		m._12 = 0.0f;	m._13 = -s;		m._14 = Position.X;
			m._21 = 0.0f;	m._22 = 1.0f;	m._23 = 0.0f;	m._24 = Position.Y;
			m._31 = s;		m._32 = 0.0f;	m._33 = -c;		m._34 = Position.Z;
			m._41 = 0.0f;	m._42 = 0.0f;	m._43 = 0.0f;	m._44 = 1.0f;
			unit->GetModel()->SetTransform(m);
		}

		completed_jobs++;
		LDR_CHECK_TIMEOUT(completed_jobs, total_jobs);
	}

	return 0;
}


int CXMLReader::ProgressiveRead()
{
	// yield after this time is reached. balances increased progress bar
	// smoothness vs. slowing down loading.
	const double end_time = get_time() + 50e-3;

	int ret;

	while(node_idx < nodes.Count)
	{
		node = nodes.item(node_idx);
		if (node.getNodeName() == el_entities)
		{
			ret = ReadEntities(node, end_time);
			if(ret != 0)	// error or timed out
				return ret;
		}
		else if (node.getNodeName() == el_nonentities)
		{
			ret = ReadNonEntities(node, end_time);
			if(ret != 0)	// error or timed out
				return ret;
		}
		else
			debug_warn("Invalid XML data - DTD shouldn't allow this");

		node_idx++;
	}

	return 0;
}


// progressive
int CMapReader::ReadXML()
{
	if(!xml_reader)
		xml_reader = new CXMLReader(filename_xml);

	int ret = xml_reader->ProgressiveRead();
	// finished or failed
	if(ret <= 0)
	{
		delete xml_reader;
		xml_reader = 0;
	}

	return ret;
}


int CMapReader::DelayLoadFinished()
{
	// we were dynamically allocated by CWorld::Initialize
	delete this;

	return 0;
}
