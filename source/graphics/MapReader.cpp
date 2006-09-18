#include "precompiled.h"

#include "MapReader.h"

#include "graphics/Camera.h"
#include "graphics/GameView.h"
#include "graphics/Model.h"
#include "graphics/ObjectManager.h"
#include "graphics/Patch.h"
#include "graphics/Terrain.h"
#include "graphics/TextureEntry.h"
#include "graphics/TextureManager.h"
#include "graphics/Unit.h"
#include "graphics/UnitManager.h"
#include "lib/timer.h"
#include "lib/types.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/Game.h"
#include "ps/Loader.h"
#include "ps/LoaderThunks.h"
#include "ps/XML/Xeromyces.h"
#include "renderer/SkyManager.h"
#include "renderer/WaterManager.h"
#include "simulation/Entity.h"
#include "simulation/EntityManager.h"
#include "simulation/EntityTemplate.h"
#include "simulation/EntityTemplateCollection.h"

#define LOG_CATEGORY "graphics"

CMapReader::CMapReader()
	: xml_reader(0)
{
	cur_terrain_tex = 0;	// important - resets generator state
}


// LoadMap: try to load the map from given file; reinitialise the scene to new data if successful
void CMapReader::LoadMap(const char* filename, CTerrain *pTerrain_,
						 CUnitManager *pUnitMan_, WaterManager* pWaterMan_, SkyManager* pSkyMan_,
						 CLightEnv *pLightEnv_, CCamera *pCamera_, CCinemaManager* pCinema_)
{
	// latch parameters (held until DelayedLoadFinished)
	pTerrain = pTerrain_;
	pUnitMan = pUnitMan_;
	pLightEnv = pLightEnv_;
	pCamera = pCamera_;
	pWaterMan = pWaterMan_;
	pSkyMan = pSkyMan_;
	pCinema = pCinema_;

	// [25ms]
	unpacker.Read(filename, "PSMP");

	// check version
	if (unpacker.GetVersion() < FILE_READ_VERSION) {
		throw PSERROR_File_InvalidVersion();
	}

	// delete all existing entities
	g_EntityManager.deleteAll();
	// delete all remaining non-entity units
	pUnitMan->DeleteAll();
	g_UnitMan.SetNextID(0);

	// unpack the data
	RegMemFun(this, &CMapReader::UnpackMap, L"CMapReader::UnpackMap", 1200);

	if (unpacker.GetVersion() >= 3) {
		// read the corresponding XML file
		filename_xml = filename;
		filename_xml = filename_xml.Left(filename_xml.Length()-4) + ".xml";
		RegMemFun(this, &CMapReader::ReadXML, L"CMapReader::ReadXML", 5800);
	}

	// apply data to the world
	RegMemFun(this, &CMapReader::ApplyData, L"CMapReader::ApplyData", 5);

	RegMemFun(this, &CMapReader::DelayLoadFinished, L"CMapReader::DelayLoadFinished", 5);
}

// UnpackMap: unpack the given data from the raw data stream into local variables
int CMapReader::UnpackMap()
{
	// now unpack everything into local data
	int ret = UnpackTerrain();
	if(ret != 0)	// failed or timed out
		return ret;

	if (unpacker.GetVersion() < 4)
		UnpackObjects();

	if (unpacker.GetVersion() >= 2 && unpacker.GetVersion() < 4)
		UnpackLightEnv();

	return 0;
}

// UnpackLightEnv: unpack lighting parameters from input stream
void CMapReader::UnpackLightEnv()
{
	unpacker.UnpackRaw(&m_LightEnv.m_SunColor, sizeof(m_LightEnv.m_SunColor));
	unpacker.UnpackRaw(&m_LightEnv.m_Elevation, sizeof(m_LightEnv.m_Elevation));
	unpacker.UnpackRaw(&m_LightEnv.m_Rotation, sizeof(m_LightEnv.m_Rotation));
	unpacker.UnpackRaw(&m_LightEnv.m_TerrainAmbientColor, sizeof(m_LightEnv.m_TerrainAmbientColor));
	unpacker.UnpackRaw(&m_LightEnv.m_UnitsAmbientColor, sizeof(m_LightEnv.m_UnitsAmbientColor));
	m_LightEnv.CalculateSunDirection();
}

// UnpackObjects: unpack world objects from input stream
void CMapReader::UnpackObjects()
{
	// unpack object types
	u32 numObjTypes;
	unpacker.UnpackRaw(&numObjTypes, sizeof(numObjTypes));
	m_ObjectTypes.resize(numObjTypes);
	for (u32 i=0; i<numObjTypes; i++) {
		unpacker.UnpackString(m_ObjectTypes[i]);
	}

	// unpack object data
	u32 numObjects;
	unpacker.UnpackRaw(&numObjects, sizeof(numObjects));
	m_Objects.resize(numObjects);
	if (numObjects)
		unpacker.UnpackRaw(&m_Objects[0], sizeof(SObjectDesc)*numObjects);
}

// UnpackTerrain: unpack the terrain from the end of the input data stream
//		- data: map size, heightmap, list of textures used by map, texture tile assignments
int CMapReader::UnpackTerrain()
{
	// yield after this time is reached. balances increased progress bar
	// smoothness vs. slowing down loading.
	const double end_time = get_time() + 200e-3;

	// first call to generator (this is skipped after first call,
	// i.e. when the loop below was interrupted)
	if (cur_terrain_tex == 0)
	{
		// unpack map size
		unpacker.UnpackRaw(&m_MapSize, sizeof(m_MapSize));

		// unpack heightmap [600us]
		u32 verticesPerSide = m_MapSize*PATCH_SIZE+1;
		m_Heightmap.resize(SQR(verticesPerSide));
		unpacker.UnpackRaw(&m_Heightmap[0], SQR(verticesPerSide)*sizeof(u16));

		// unpack # textures
		unpacker.UnpackRaw(&num_terrain_tex, sizeof(num_terrain_tex));
		m_TerrainTextures.reserve(num_terrain_tex);
	}

	// unpack texture names; find handle for each texture.
	// interruptible.
	while (cur_terrain_tex < num_terrain_tex)
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
	unpacker.UnpackRaw(&m_Tiles[0], (u32)(sizeof(STileDesc)*m_Tiles.size()));

	// reset generator state.
	cur_terrain_tex = 0;

	return 0;
}
int CMapReader::UnpackCinema()
{
	size_t numTracks;
	unpacker.UnpackRaw(&numTracks, (u32)sizeof(size_t));
	
	for ( size_t track=0; track < numTracks; ++track )
	{
		CCinemaTrack trackObj;
		std::vector<CCinemaPath> paths;
		CStr name;
		size_t numPaths;
		CVector3D startRotation;
		float timescale;

		unpacker.UnpackString(name);
		unpacker.UnpackRaw(&timescale, sizeof(float));
		unpacker.UnpackRaw(&numPaths, sizeof(size_t));
		unpacker.UnpackRaw(&startRotation, sizeof(CVector3D));

		trackObj.SetStartRotation(startRotation);
		trackObj.SetTimescale(timescale);

		for ( size_t i=0; i<numPaths; ++i )
		{
			TNSpline spline;
			size_t numNodes;
			CCinemaData data;
			
			if ( i != 0 )
				unpacker.UnpackRaw(&data.m_TotalRotation, sizeof(CVector3D));
			unpacker.UnpackRaw(&data.m_Mode, sizeof(data.m_Mode));
			unpacker.UnpackRaw(&data.m_Style, sizeof(data.m_Style));
			unpacker.UnpackRaw(&data.m_Growth, sizeof(data.m_Growth));
			unpacker.UnpackRaw(&data.m_Switch, sizeof(data.m_Switch));
			unpacker.UnpackRaw(&numNodes, sizeof(size_t));
			data.m_GrowthCount = data.m_Growth;

			for ( size_t j=0; j < numNodes; ++j )
			{
				CVector3D position;
				float distance;
				unpacker.UnpackRaw(&position, sizeof(position));
				unpacker.UnpackRaw(&distance, sizeof(distance));
				spline.AddNode(position, distance);
			}
			trackObj.AddPath(data, spline);
		}
		m_Tracks[CStrW(name)] = trackObj;
	}
	return 0;
}

// ApplyData: take all the input data, and rebuild the scene from it
int CMapReader::ApplyData()
{
	// initialise the terrain
	pTerrain->Initialize(m_MapSize, &m_Heightmap[0]);
	
	// setup the textures on the minipatches
	STileDesc* tileptr = &m_Tiles[0];
	for (u32 j=0; j<m_MapSize; j++) {
		for (u32 i=0; i<m_MapSize; i++) {
			for (u32 m=0; m<(u32)PATCH_SIZE; m++) {
				for (u32 k=0; k<(u32)PATCH_SIZE; k++) {
					CMiniPatch& mp = pTerrain->GetPatch(i,j)->m_MiniPatches[m][k];

					mp.Tex1 = m_TerrainTextures[tileptr->m_Tex1Index];
					mp.Tex1Priority = tileptr->m_Priority;

					tileptr++;
				}
			}
		}
	}

	// add new objects
	for (size_t i = 0; i < m_Objects.size(); ++i)
	{

		if (unpacker.GetVersion() < 3)
		{
			debug_warn("Old unsupported map version - objects will be missing");
			// (getTemplateByActor doesn't work, since entity templates are now
			// loaded on demand)
		}

		std::set<CStr8> selections; // TODO: read from file
		CUnit* unit = g_UnitMan.CreateUnit(m_ObjectTypes.at(m_Objects[i].m_ObjectIndex), NULL, selections);

		if (unit)
		{
			CMatrix3D transform;
			memcpy2(&transform._11, m_Objects[i].m_Transform, sizeof(float)*16);
			unit->GetModel()->SetTransform(transform);
		}
	}
	//Make units start out conforming correctly
	g_EntityManager.conformAll();

	if (unpacker.GetVersion() >= 2)
	{
		// copy over the lighting parameters
		*pLightEnv = m_LightEnv;
	}
	return 0;
}



// Holds various state data while reading maps, so that loading can be
// interrupted (e.g. to update the progress display) then later resumed.
class CXMLReader
{
public:
	CXMLReader(const CStr& xml_filename, CMapReader& mapReader)
		: m_MapReader(mapReader)
	{
		Init(xml_filename);
	}

	// return semantics: see Loader.cpp!LoadFunc.
	int ProgressiveRead();

private:
	CXeromyces xmb_file;

	CMapReader& m_MapReader;

	int el_entity;
	int el_tracks;
	int el_template, el_player;
	int el_position, el_orientation;
	int el_nonentity;
	int el_actor;
	int at_x, at_y, at_z;
	int at_angle;

	XMBElementList nodes; // children of root

	// loop counters
	int node_idx;
	int entity_idx, nonentity_idx;

	// # entities+nonentities processed and total (for progress calc)
	int completed_jobs, total_jobs;


	void Init(const CStr& xml_filename);

	void ReadEnvironment(XMBElement parent);
	void ReadCamera(XMBElement parent);
	void ReadCinema(XMBElement parent);
	int ReadEntities(XMBElement parent, double end_time);
	int ReadNonEntities(XMBElement parent, double end_time);

	NO_COPY_CTOR(CXMLReader);
};


void CXMLReader::Init(const CStr& xml_filename)
{
	// must only assign once, so do it here
	node_idx = entity_idx = nonentity_idx = 0;

	if (xmb_file.Load(xml_filename) != PSRETURN_OK)
		throw PSERROR_File_ReadFailed();

	// define the elements and attributes that are frequently used in the XML file,
	// so we don't need to do lots of string construction and comparison when
	// reading the data.
	// (Needs to be synchronised with the list in CXMLReader - ugh)
#define EL(x) el_##x = xmb_file.getElementID(#x)
#define AT(x) at_##x = xmb_file.getAttributeID(#x)
	EL(entity);
	EL(tracks);
	EL(template);
	EL(player);
	EL(position);
	EL(orientation);
	EL(nonentity);
	EL(actor);
	AT(x); AT(y); AT(z);
	AT(angle);
#undef AT
#undef EL

	XMBElement root = xmb_file.getRoot();
	debug_assert(xmb_file.getElementString(root.getNodeName()) == "Scenario");
	nodes = root.getChildNodes();

	// find out total number of entities+nonentities
	// (used when calculating progress)
	completed_jobs = 0;
	total_jobs = 0;
	for (int i = 0; i < nodes.Count; i++)
		total_jobs += nodes.item(i).getChildNodes().Count;
}


void CXMLReader::ReadEnvironment(XMBElement parent)
{
#define EL(x) int el_##x = xmb_file.getElementID(#x)
#define AT(x) int at_##x = xmb_file.getAttributeID(#x)
	EL(skyset);
	EL(suncolour);
	EL(sunelevation);
	EL(sunrotation);
	EL(terrainambientcolour);
	EL(unitsambientcolour);
	EL(terrainshadowtransparency);
	EL(water);
	EL(waterbody);
	EL(type);
	EL(colour);
	EL(height);
	EL(shininess);
	EL(waviness);
	EL(murkiness);
	EL(tint);
	EL(reflectiontint);
	EL(reflectiontintstrength);
	AT(r); AT(g); AT(b);
#undef AT
#undef EL

	XERO_ITER_EL(parent, element)
	{
		int element_name = element.getNodeName();

		XMBAttributeList attrs = element.getAttributes();

		if (element_name == el_skyset)
		{
			m_MapReader.pSkyMan->SetSkySet(element.getText());
		}
		else if (element_name == el_suncolour)
		{
			m_MapReader.m_LightEnv.m_SunColor = RGBColor(
				CStr(attrs.getNamedItem(at_r)).ToFloat(),
				CStr(attrs.getNamedItem(at_g)).ToFloat(),
				CStr(attrs.getNamedItem(at_b)).ToFloat());
		}
		else if (element_name == el_sunelevation)
		{
			m_MapReader.m_LightEnv.m_Elevation = CStr(attrs.getNamedItem(at_angle)).ToFloat();
		}
		else if (element_name == el_sunrotation)
		{
			m_MapReader.m_LightEnv.m_Rotation = CStr(attrs.getNamedItem(at_angle)).ToFloat();
		}
		else if (element_name == el_terrainambientcolour)
		{
			m_MapReader.m_LightEnv.m_TerrainAmbientColor = RGBColor(
				CStr(attrs.getNamedItem(at_r)).ToFloat(),
				CStr(attrs.getNamedItem(at_g)).ToFloat(),
				CStr(attrs.getNamedItem(at_b)).ToFloat());
		}
		else if (element_name == el_unitsambientcolour)
		{
			m_MapReader.m_LightEnv.m_UnitsAmbientColor = RGBColor(
				CStr(attrs.getNamedItem(at_r)).ToFloat(),
				CStr(attrs.getNamedItem(at_g)).ToFloat(),
				CStr(attrs.getNamedItem(at_b)).ToFloat());
		}
		else if (element_name == el_terrainshadowtransparency)
		{
			m_MapReader.m_LightEnv.SetTerrainShadowTransparency(CStr(element.getText()).ToFloat());
		}
		else if (element_name == el_water)
		{
			XERO_ITER_EL(element, waterbody)
			{
				debug_assert(waterbody.getNodeName() == el_waterbody);
				XERO_ITER_EL(waterbody, waterelement)
				{
					int element_name = waterelement.getNodeName();
					if (element_name == el_type)
					{
						// TODO: implement this, when WaterManager supports it
					}

#define READ_COLOUR(el, out) \
					else if (element_name == el) \
					{ \
						XMBAttributeList attrs = waterelement.getAttributes(); \
						out = CColor( \
							CStr(attrs.getNamedItem(at_r)).ToFloat(), \
							CStr(attrs.getNamedItem(at_g)).ToFloat(), \
							CStr(attrs.getNamedItem(at_b)).ToFloat(), \
							1.f); \
					}

#define READ_FLOAT(el, out) \
					else if (element_name == el) \
					{ \
						out = CStr(waterelement.getText()).ToFloat(); \
					} \

					READ_COLOUR(el_colour, m_MapReader.pWaterMan->m_WaterColor)
					READ_FLOAT(el_height, m_MapReader.pWaterMan->m_WaterHeight)
					READ_FLOAT(el_shininess, m_MapReader.pWaterMan->m_Shininess)
					READ_FLOAT(el_waviness, m_MapReader.pWaterMan->m_Waviness)
					READ_FLOAT(el_murkiness, m_MapReader.pWaterMan->m_Murkiness)
					READ_COLOUR(el_tint, m_MapReader.pWaterMan->m_WaterTint)
					READ_COLOUR(el_reflectiontint, m_MapReader.pWaterMan->m_ReflectionTint)
					READ_FLOAT(el_reflectiontintstrength, m_MapReader.pWaterMan->m_ReflectionTintStrength)

#undef READ_FLOAT
#undef READ_COLOUR

					else
						debug_warn("Invalid map XML data");
				}

			}
		}
		else
			debug_warn("Invalid map XML data");
	}

	m_MapReader.m_LightEnv.CalculateSunDirection();
}

void CXMLReader::ReadCamera(XMBElement parent)
{
#define EL(x) int el_##x = xmb_file.getElementID(#x)
#define AT(x) int at_##x = xmb_file.getAttributeID(#x)
	EL(declination);
	EL(rotation);
	EL(position);
	AT(angle);
	AT(x); AT(y); AT(z);
#undef AT
#undef EL

	float declination = DEGTORAD(30.f), rotation = DEGTORAD(-45.f);
	CVector3D translation = CVector3D(100, 150, -100);

	XERO_ITER_EL(parent, element)
	{
		int element_name = element.getNodeName();

		XMBAttributeList attrs = element.getAttributes();
		if (element_name == el_declination)
		{
			declination = CStr(attrs.getNamedItem(at_angle)).ToFloat();
		}
		else if (element_name == el_rotation)
		{
			rotation = CStr(attrs.getNamedItem(at_angle)).ToFloat();
		}
		else if (element_name == el_position)
		{
			translation = CVector3D(
				CStr(attrs.getNamedItem(at_x)).ToFloat(),
				CStr(attrs.getNamedItem(at_y)).ToFloat(),
				CStr(attrs.getNamedItem(at_z)).ToFloat());
		}
		else
			debug_warn("Invalid map XML data");
	}

	m_MapReader.pCamera->m_Orientation.SetXRotation(declination);
	m_MapReader.pCamera->m_Orientation.RotateY(rotation);
	m_MapReader.pCamera->m_Orientation.Translate(translation);
	m_MapReader.pCamera->UpdateFrustum();
}
void CXMLReader::ReadCinema(XMBElement parent)
{
	#define EL(x) int el_##x = xmb_file.getElementID(#x)
	#define AT(x) int at_##x = xmb_file.getAttributeID(#x)

	EL(track);
	EL(startrotation);
	EL(path);
	EL(rotation);
	EL(distortion);
	EL(node);
	AT(name);
	AT(timescale);
	AT(mode);
	AT(style);
	AT(growth);
	AT(switch);
	AT(x);
	AT(y);
	AT(z);
	AT(t);

#undef EL
#undef AT
	
	std::map<CStrW, CCinemaTrack> trackList;
	XERO_ITER_EL(parent, element)
	{
		int elementName = element.getNodeName();
			
		if ( elementName == el_track )
		{
			CCinemaTrack track;
			XMBAttributeList attrs = element.getAttributes();
			CStrW name( CStr(attrs.getNamedItem(at_name)) );
			float timescale = CStr(attrs.getNamedItem(at_timescale)).ToFloat();
			track.SetTimescale(timescale);

			XERO_ITER_EL(element, trackChild)
			{
				elementName = trackChild.getNodeName();

				if ( elementName == el_startrotation )
				{
					attrs = trackChild.getAttributes();
					float x = CStr(attrs.getNamedItem(at_x)).ToFloat();
					float y = CStr(attrs.getNamedItem(at_y)).ToFloat();
					float z = CStr(attrs.getNamedItem(at_z)).ToFloat();
					track.SetStartRotation(CVector3D(x, y, z));
				}
				else if ( elementName == el_path )
				{
					CCinemaData pathData;
					TNSpline spline, backwardSpline;

					XERO_ITER_EL(trackChild, pathChild)
					{
						elementName = pathChild.getNodeName();
						attrs = pathChild.getAttributes();

						if ( elementName == el_rotation )
						{
							float x = CStr(attrs.getNamedItem(at_x)).ToFloat();
							float y = CStr(attrs.getNamedItem(at_y)).ToFloat();
							float z = CStr(attrs.getNamedItem(at_z)).ToFloat();
							pathData.m_TotalRotation = CVector3D(x, y, z);
						}
						else if ( elementName == el_distortion )
						{
							pathData.m_Mode = CStr(attrs.getNamedItem(at_mode)).ToInt();
							pathData.m_Style = CStr(attrs.getNamedItem(at_style)).ToInt();
							pathData.m_Growth = CStr(attrs.getNamedItem(at_growth)).ToInt();
							pathData.m_Switch = CStr(attrs.getNamedItem(at_switch)).ToInt();
						}
						else if ( elementName == el_node )
						{
							SplineData data;
							data.Position.X = CStr(attrs.getNamedItem(at_x)).ToFloat();
							data.Position.Y = CStr(attrs.getNamedItem(at_y)).ToFloat();
							data.Position.Z = CStr(attrs.getNamedItem(at_z)).ToFloat();
							data.Distance = CStr(attrs.getNamedItem(at_t)).ToFloat();
							backwardSpline.AddNode(data.Position, data.Distance);
						}
						else
							debug_warn("Invalid cinematic element for path child");
					}	//node loop
					CCinemaPath temp(pathData, backwardSpline);
					const std::vector<SplineData>& nodes = temp.GetAllNodes();
					if ( nodes.empty() )
					{
						debug_warn("Failure loading cinematics");
						return;
					}
					
					for ( std::vector<SplineData>::const_reverse_iterator 
						it=nodes.rbegin(); it != nodes.rend(); ++it )
					{
						spline.AddNode(it->Position, it->Distance);
					}
					track.AddPath(pathData, spline);

				}	// == el_path
				else
					debug_warn("Invalid cinematic element for track child");
			}
			trackList[name] = track;
		}
		else 
			debug_warn("Invalid cinematic element for root track child");
	}
	g_Game->GetView()->GetCinema()->SetAllTracks(trackList);
}

int CXMLReader::ReadEntities(XMBElement parent, double end_time)
{
	XMBElementList entities = parent.getChildNodes();
	while (entity_idx < entities.Count)
	{
		// all new state at this scope and below doesn't need to be
		// wrapped, since we only yield after a complete iteration.

		XMBElement entity = entities.item(entity_idx++);
		debug_assert(entity.getNodeName() == el_entity);

		CStrW TemplateName;
		int PlayerID = 0;
		CVector3D Position;
		float Orientation = 0.f;

		XERO_ITER_EL(entity, setting)
		{
			int element_name = setting.getNodeName();

			// <template>
			if (element_name == el_template)
			{
				TemplateName = setting.getText();
			}
			// <player>
			else if (element_name == el_player)
			{
				PlayerID = CStr(setting.getText()).ToInt();
			}
			// <position>
			else if (element_name == el_position)
			{
				XMBAttributeList attrs = setting.getAttributes();
				Position = CVector3D(
					CStr(attrs.getNamedItem(at_x)).ToFloat(),
					CStr(attrs.getNamedItem(at_y)).ToFloat(),
					CStr(attrs.getNamedItem(at_z)).ToFloat());
			}
			// <orientation>
			else if (element_name == el_orientation)
			{
				XMBAttributeList attrs = setting.getAttributes();
				Orientation = CStr(attrs.getNamedItem(at_angle)).ToFloat();
			}
			else
				debug_warn("Invalid map XML data");
		}

		CEntityTemplate* base = g_EntityTemplateCollection.getTemplate( TemplateName, g_Game->GetPlayer(PlayerID) );
		if (! base)
			LOG(ERROR, LOG_CATEGORY, "Failed to load entity template '%ls'", TemplateName.c_str());
		else
		{
			std::set<CStr8> selections; // TODO: read from file

			HEntity ent = g_EntityManager.create(base, Position, Orientation, selections);

			if (! ent)
				LOG(ERROR, LOG_CATEGORY, "Failed to create entity of type '%ls'", TemplateName.c_str());
			else
			{
				ent->m_actor->SetPlayerID(PlayerID);

				// TODO: save object IDs in the map file, and load them again,
				// so that triggers have a persistent identifier for objects
				ent->m_actor->SetID(g_UnitMan.GetNewID());
			}
		}

		completed_jobs++;
		LDR_CHECK_TIMEOUT(completed_jobs, total_jobs);
	}

	return 0;
}


int CXMLReader::ReadNonEntities(XMBElement parent, double end_time)
{
	XMBElementList nonentities = parent.getChildNodes();
	while (nonentity_idx < nonentities.Count)
	{
		// all new state at this scope and below doesn't need to be
		// wrapped, since we only yield after a complete iteration.

		XMBElement nonentity = nonentities.item(nonentity_idx++);
		debug_assert(nonentity.getNodeName() == el_nonentity);

		CStr ActorName;
		CVector3D Position;
		float Orientation = 0.f;

		XERO_ITER_EL(nonentity, setting)
		{
			int element_name = setting.getNodeName();

			// <actor>
			if (element_name == el_actor)
			{
				ActorName = setting.getText();
			}
			// <position>
			else if (element_name == el_position)
			{
				XMBAttributeList attrs = setting.getAttributes();
				Position = CVector3D(CStr(attrs.getNamedItem(at_x)).ToFloat(),
					CStr(attrs.getNamedItem(at_y)).ToFloat(),
					CStr(attrs.getNamedItem(at_z)).ToFloat());
			}
			// <orientation>
			else if (element_name == el_orientation)
			{
				XMBAttributeList attrs = setting.getAttributes();
				Orientation = CStr(attrs.getNamedItem(at_angle)).ToFloat();
			}
			else
				debug_warn("Invalid map XML data");
		}

		std::set<CStr8> selections; // TODO: read from file

		CUnit* unit = g_UnitMan.CreateUnit(ActorName, NULL, selections);

		if (unit)
		{
			// Copied from CEntity::updateActorTransforms():
			float s = sin(Orientation);
			float c = cos(Orientation);
			CMatrix3D m;
			m._11 = -c;     m._12 = 0.0f;   m._13 = -s;     m._14 = Position.X;
			m._21 = 0.0f;   m._22 = 1.0f;   m._23 = 0.0f;   m._24 = Position.Y;
			m._31 = s;      m._32 = 0.0f;   m._33 = -c;     m._34 = Position.Z;
			m._41 = 0.0f;   m._42 = 0.0f;   m._43 = 0.0f;   m._44 = 1.0f;
			unit->GetModel()->SetTransform(m);

			// TODO: save object IDs in the map file, and load them again,
			// so that triggers have a persistent identifier for objects
			unit->SetID(g_UnitMan.GetNewID());
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
	const double end_time = get_time() + 200e-3;

	int ret;

	while (node_idx < nodes.Count)
	{
		XMBElement node = nodes.item(node_idx);
		CStr name = xmb_file.getElementString(node.getNodeName());
		if (name == "Environment")
		{
			ReadEnvironment(node);
		}
		else if (name == "Camera")
		{
			ReadCamera(node);
		}
		else if (name == "Entities")
		{
			ret = ReadEntities(node, end_time);
			if (ret != 0)	// error or timed out
				return ret;
		}
		else if (name == "Nonentities")
		{
			ret = ReadNonEntities(node, end_time);
			if (ret != 0)	// error or timed out
				return ret;
		}
		else if (name == "Tracks")
		{
			ReadCinema(node);
		}
		else
			debug_warn("Invalid map XML data");

		node_idx++;
	}

	return 0;
}


// progressive
int CMapReader::ReadXML()
{
	if (!xml_reader)
		xml_reader = new CXMLReader(filename_xml, *this);

	int ret = xml_reader->ProgressiveRead();
	// finished or failed
	if (ret <= 0)
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
