#include "precompiled.h"

#include "MapReader.h"

#include "graphics/Camera.h"
#include "graphics/CinemaTrack.h"
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
#include "simulation/TriggerManager.h"
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
	g_EntityManager.DeleteAll();
	// delete all remaining non-entity units
	pUnitMan->DeleteAll();
	pUnitMan->SetNextID(0);

	// unpack the data
	RegMemFun(this, &CMapReader::UnpackMap, L"CMapReader::UnpackMap", 1200);

	if (unpacker.GetVersion() >= 3) {
		// read the corresponding XML file
		filename_xml = filename;
		filename_xml = filename_xml.Left(filename_xml.length()-4) + ".xml";
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
	const size_t numObjTypes = unpacker.UnpackSize();
	m_ObjectTypes.resize(numObjTypes);
	for (size_t i=0; i<numObjTypes; i++)
		unpacker.UnpackString(m_ObjectTypes[i]);

	// unpack object data
	const size_t numObjects = unpacker.UnpackSize();
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
	const double end_time = timer_Time() + 200e-3;

	// first call to generator (this is skipped after first call,
	// i.e. when the loop below was interrupted)
	if (cur_terrain_tex == 0)
	{
		m_MapSize = (ssize_t)unpacker.UnpackSize();

		// unpack heightmap [600us]
		size_t verticesPerSide = m_MapSize*PATCH_SIZE+1;
		m_Heightmap.resize(SQR(verticesPerSide));
		unpacker.UnpackRaw(&m_Heightmap[0], SQR(verticesPerSide)*sizeof(u16));

		// unpack # textures
		num_terrain_tex = unpacker.UnpackSize();
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
	ssize_t tilesPerSide = m_MapSize*PATCH_SIZE;
	m_Tiles.resize(size_t(SQR(tilesPerSide)));
	unpacker.UnpackRaw(&m_Tiles[0], sizeof(STileDesc)*m_Tiles.size());

	// reset generator state.
	cur_terrain_tex = 0;

	return 0;
}

// ApplyData: take all the input data, and rebuild the scene from it
int CMapReader::ApplyData()
{
	// initialise the terrain
	pTerrain->Initialize(m_MapSize, &m_Heightmap[0]);
	
	// setup the textures on the minipatches
	STileDesc* tileptr = &m_Tiles[0];
	for (ssize_t j=0; j<m_MapSize; j++) {
		for (ssize_t i=0; i<m_MapSize; i++) {
			for (ssize_t m=0; m<PATCH_SIZE; m++) {
				for (ssize_t k=0; k<PATCH_SIZE; k++) {
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
			// (GetTemplateByActor doesn't work, since entity templates are now
			// loaded on demand)
		}

		std::set<CStr> selections; // TODO: read from file
		CUnit* unit = pUnitMan->CreateUnit(m_ObjectTypes.at(m_Objects[i].m_ObjectIndex), NULL, selections);

		if (unit)
		{
			CMatrix3D transform;
			cpu_memcpy(&transform._11, m_Objects[i].m_Transform, sizeof(float)*16);
			unit->GetModel()->SetTransform(transform);
		}
	}
	//Make units start out conforming correctly
	g_EntityManager.ConformAll();

	if (unpacker.GetVersion() >= 2)
	{
		// copy over the lighting parameters
		*pLightEnv = m_LightEnv;
	}
	return 0;
}



// Holds various state data while reading maps, so that loading can be
// interrupted (e.g. to update the progress display) then later resumed.
class CXMLReader : noncopyable
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
	int at_id;
	int at_angle;
	int at_uid;

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
	void ReadTriggers(XMBElement parent);
	void ReadTriggerGroup(XMBElement parent, MapTriggerGroup& group);
	int ReadEntities(XMBElement parent, double end_time);
	int ReadNonEntities(XMBElement parent, double end_time);
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
#define EL(x) el_##x = xmb_file.GetElementID(#x)
#define AT(x) at_##x = xmb_file.GetAttributeID(#x)
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
	AT(uid);
#undef AT
#undef EL

	XMBElement root = xmb_file.GetRoot();
	debug_assert(xmb_file.GetElementString(root.GetNodeName()) == "Scenario");
	nodes = root.GetChildNodes();

	// find out total number of entities+nonentities
	// (used when calculating progress)
	completed_jobs = 0;
	total_jobs = 0;
	for (int i = 0; i < nodes.Count; i++)
		total_jobs += nodes.Item(i).GetChildNodes().Count;
}


void CXMLReader::ReadEnvironment(XMBElement parent)
{
#define EL(x) int el_##x = xmb_file.GetElementID(#x)
#define AT(x) int at_##x = xmb_file.GetAttributeID(#x)
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
		int element_name = element.GetNodeName();

		XMBAttributeList attrs = element.GetAttributes();

		if (element_name == el_skyset)
		{
			m_MapReader.pSkyMan->SetSkySet(element.GetText());
		}
		else if (element_name == el_suncolour)
		{
			m_MapReader.m_LightEnv.m_SunColor = RGBColor(
				CStr(attrs.GetNamedItem(at_r)).ToFloat(),
				CStr(attrs.GetNamedItem(at_g)).ToFloat(),
				CStr(attrs.GetNamedItem(at_b)).ToFloat());
		}
		else if (element_name == el_sunelevation)
		{
			m_MapReader.m_LightEnv.m_Elevation = CStr(attrs.GetNamedItem(at_angle)).ToFloat();
		}
		else if (element_name == el_sunrotation)
		{
			m_MapReader.m_LightEnv.m_Rotation = CStr(attrs.GetNamedItem(at_angle)).ToFloat();
		}
		else if (element_name == el_terrainambientcolour)
		{
			m_MapReader.m_LightEnv.m_TerrainAmbientColor = RGBColor(
				CStr(attrs.GetNamedItem(at_r)).ToFloat(),
				CStr(attrs.GetNamedItem(at_g)).ToFloat(),
				CStr(attrs.GetNamedItem(at_b)).ToFloat());
		}
		else if (element_name == el_unitsambientcolour)
		{
			m_MapReader.m_LightEnv.m_UnitsAmbientColor = RGBColor(
				CStr(attrs.GetNamedItem(at_r)).ToFloat(),
				CStr(attrs.GetNamedItem(at_g)).ToFloat(),
				CStr(attrs.GetNamedItem(at_b)).ToFloat());
		}
		else if (element_name == el_terrainshadowtransparency)
		{
			m_MapReader.m_LightEnv.SetTerrainShadowTransparency(CStr(element.GetText()).ToFloat());
		}
		else if (element_name == el_water)
		{
			XERO_ITER_EL(element, waterbody)
			{
				debug_assert(waterbody.GetNodeName() == el_waterbody);
				XERO_ITER_EL(waterbody, waterelement)
				{
					int element_name = waterelement.GetNodeName();
					if (element_name == el_type)
					{
						// TODO: implement this, when WaterManager supports it
					}

#define READ_COLOUR(el, out) \
					else if (element_name == el) \
					{ \
						XMBAttributeList attrs = waterelement.GetAttributes(); \
						out = CColor( \
							CStr(attrs.GetNamedItem(at_r)).ToFloat(), \
							CStr(attrs.GetNamedItem(at_g)).ToFloat(), \
							CStr(attrs.GetNamedItem(at_b)).ToFloat(), \
							1.f); \
					}

#define READ_FLOAT(el, out) \
					else if (element_name == el) \
					{ \
						out = CStr(waterelement.GetText()).ToFloat(); \
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
#define EL(x) int el_##x = xmb_file.GetElementID(#x)
#define AT(x) int at_##x = xmb_file.GetAttributeID(#x)
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
		int element_name = element.GetNodeName();

		XMBAttributeList attrs = element.GetAttributes();
		if (element_name == el_declination)
		{
			declination = CStr(attrs.GetNamedItem(at_angle)).ToFloat();
		}
		else if (element_name == el_rotation)
		{
			rotation = CStr(attrs.GetNamedItem(at_angle)).ToFloat();
		}
		else if (element_name == el_position)
		{
			translation = CVector3D(
				CStr(attrs.GetNamedItem(at_x)).ToFloat(),
				CStr(attrs.GetNamedItem(at_y)).ToFloat(),
				CStr(attrs.GetNamedItem(at_z)).ToFloat());
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
	#define EL(x) int el_##x = xmb_file.GetElementID(#x)
	#define AT(x) int at_##x = xmb_file.GetAttributeID(#x)

	EL(path);
	EL(rotation);
	EL(distortion);
	EL(node);
	EL(position);
	EL(time);
	AT(name);
	AT(timescale);
	AT(mode);
	AT(style);
	AT(growth);
	AT(switch);
	AT(x);
	AT(y);
	AT(z);

#undef EL
#undef AT
	
	std::map<CStrW, CCinemaPath> pathList;
	XERO_ITER_EL(parent, element)
	{
		int elementName = element.GetNodeName();
			
		if ( elementName == el_path )
		{
			XMBAttributeList attrs = element.GetAttributes();
			CStrW name( CStr(attrs.GetNamedItem(at_name)) );
			float timescale = CStr(attrs.GetNamedItem(at_timescale)).ToFloat();
			CCinemaData pathData;
			pathData.m_Timescale = timescale;
			TNSpline spline, backwardSpline;

			XERO_ITER_EL(element, pathChild)
			{
				elementName = pathChild.GetNodeName();
				attrs = pathChild.GetAttributes();

				//Load distortion attributes
				if ( elementName == el_distortion )
				{
						pathData.m_Mode = CStr(attrs.GetNamedItem(at_mode)).ToInt();
						pathData.m_Style = CStr(attrs.GetNamedItem(at_style)).ToInt();
						pathData.m_Growth = CStr(attrs.GetNamedItem(at_growth)).ToInt();
						pathData.m_Switch = CStr(attrs.GetNamedItem(at_switch)).ToInt();
				}
				
				//Load node data used for spline
				else if ( elementName == el_node )
				{
					SplineData data;
					XERO_ITER_EL(pathChild, nodeChild)
					{
						elementName = nodeChild.GetNodeName();
						attrs = nodeChild.GetAttributes();
						
						//Fix?:  assumes that time is last element
						if ( elementName == el_position )
						{
							data.Position.X = CStr(attrs.GetNamedItem(at_x)).ToFloat();
							data.Position.Y = CStr(attrs.GetNamedItem(at_y)).ToFloat();
							data.Position.Z = CStr(attrs.GetNamedItem(at_z)).ToFloat();
							continue;
						}
						else if ( elementName == el_rotation )
						{
							data.Rotation.X = CStr(attrs.GetNamedItem(at_x)).ToFloat();
							data.Rotation.Y = CStr(attrs.GetNamedItem(at_y)).ToFloat();
							data.Rotation.Z = CStr(attrs.GetNamedItem(at_z)).ToFloat();
							continue;
						}
						else if ( elementName == el_time )
							data.Distance = CStr( nodeChild.GetText() ).ToFloat();
						else 
							debug_warn("Invalid cinematic element for node child");
					
						backwardSpline.AddNode(data.Position, data.Rotation, data.Distance);
					}
				}
				else
					debug_warn("Invalid cinematic element for path child");
				
				
			}

			//Construct cinema path with data gathered
			CCinemaPath temp(pathData, backwardSpline);
			const std::vector<SplineData>& nodes = temp.GetAllNodes();
			if ( nodes.empty() )
			{
				debug_warn("Failure loading cinematics");
				return;
			}
					
			for ( std::vector<SplineData>::const_reverse_iterator it = nodes.rbegin(); 
															it != nodes.rend(); ++it )
			{
				spline.AddNode(it->Position, it->Rotation, it->Distance);
			}
				
			CCinemaPath path(pathData, spline);
			pathList[name] = path;	
		}
		else
			debug_assert("Invalid cinema child");
	}
	g_Game->GetView()->GetCinema()->SetAllPaths(pathList);
}

void CXMLReader::ReadTriggers(XMBElement parent)
{
	MapTriggerGroup rootGroup( CStrW(L"Triggers"), CStrW(L"") );
	g_TriggerManager.DestroyEngineTriggers();
	ReadTriggerGroup(parent, rootGroup);
}

void CXMLReader::ReadTriggerGroup(XMBElement parent, MapTriggerGroup& group)
{
	#define EL(x) int el_##x = xmb_file.GetElementID(#x)
	#define AT(x) int at_##x = xmb_file.GetAttributeID(#x)
	
	EL(group);
	EL(trigger);
	EL(active);
	EL(delay);
	EL(maxruncount);
	EL(conditions);
	EL(logicblock);
	EL(logicblockend);
	EL(condition);
	EL(parameter);
	EL(linklogic);
	EL(effects);
	EL(effect);
	EL(function);
	EL(display);

	AT(name);
	AT(function);
	AT(display);
	AT(not);
	
	#undef EL
	#undef AT
	
	CStrW name = parent.GetAttributes().GetNamedItem(at_name), parentName = group.parentName;
	if ( group.name == L"Triggers" )
		name = group.name;
	
	MapTriggerGroup mapGroup(name, parentName);

	XERO_ITER_EL(parent, groupChild)
	{
		int elementName = groupChild.GetNodeName();
		if ( elementName == el_group )
			ReadTriggerGroup(groupChild, mapGroup);
		
		else if ( elementName == el_trigger )
		{
			MapTrigger mapTrigger;
			mapTrigger.name = CStrW( groupChild.GetAttributes().GetNamedItem(at_name) );

			//Read everything in this trigger
			XERO_ITER_EL(groupChild, triggerChild)
			{
				elementName = triggerChild.GetNodeName();
				if ( elementName == el_active )
				{
					if ( CStr("false") == CStr( triggerChild.GetText() ) )
						mapTrigger.active = false;
					else
						mapTrigger.active = true;
				}
			
				else if ( elementName == el_maxruncount )
					mapTrigger.maxRunCount = CStr( triggerChild.GetText() ).ToInt();
				else if ( elementName == el_delay )
					mapTrigger.timeValue = CStr( triggerChild.GetText() ).ToFloat();
			
				else if ( elementName == el_conditions )
				{
					//Read in all conditions for this trigger
					XERO_ITER_EL(triggerChild, condition)
					{
						elementName = condition.GetNodeName();
						if ( elementName == el_condition )
						{
							MapTriggerCondition mapCondition;
							mapCondition.name = condition.GetAttributes().GetNamedItem(at_name);
							mapCondition.functionName = condition.GetAttributes().GetNamedItem(at_function);
							mapCondition.displayName = condition.GetAttributes().GetNamedItem(at_display);
							
							CStr notAtt(condition.GetAttributes().GetNamedItem(at_not));
							if ( notAtt == CStr("true") )
								mapCondition.negated = true;

							//Read in each condition child
							XERO_ITER_EL(condition, conditionChild)
							{
								elementName = conditionChild.GetNodeName();
						
								if ( elementName == el_function )
									mapCondition.functionName = CStrW(conditionChild.GetText());
								else if ( elementName == el_display )
									mapCondition.displayName = CStrW(conditionChild.GetText());
								else if ( elementName == el_parameter )
									mapCondition.parameters.push_back( conditionChild.GetText() );
								else if ( elementName == el_linklogic )
								{
									CStr logic = conditionChild.GetText();
									if ( logic == CStr("AND") )
										mapCondition.linkLogic = 1;
									else
										mapCondition.linkLogic = 2;
								}
							}
							mapTrigger.conditions.push_back(mapCondition);
						}	//Read all conditions

						else if ( elementName == el_logicblock)
						{
							if ( CStr(condition.GetAttributes().GetNamedItem(at_not)) == CStr("true") )	
								mapTrigger.AddLogicBlock(true);
							else
								mapTrigger.AddLogicBlock(false);
						}
						else if ( elementName == el_logicblockend)
							mapTrigger.AddLogicBlockEnd();

					}	//Read all conditions
				}	

				else if ( elementName == el_effects )
				{
					//Read all effects
					XERO_ITER_EL(triggerChild, effect)
					{
						if ( effect.GetNodeName() != el_effect )
						{
							debug_warn("Invalid effect tag in trigger XML file");
							return;
						}
						MapTriggerEffect mapEffect;
						mapEffect.name = effect.GetAttributes().GetNamedItem(at_name);
						
						//Read parameters
						XERO_ITER_EL(effect, effectChild)
						{
							elementName = effectChild.GetNodeName();
							if ( elementName == el_function )
								mapEffect.functionName = effectChild.GetText();
							else if ( elementName == el_display )
								mapEffect.displayName = effectChild.GetText();
							else if ( elementName == el_parameter )
								mapEffect.parameters.push_back( effectChild.GetText() );
							else
							{
								debug_warn("Invalid parameter tag in trigger XML file");
								return;
							}
						}
						mapTrigger.effects.push_back(mapEffect);
					}
				}
				else
					debug_warn("Invalid trigger node child in trigger XML file");

			}	//Read trigger children
			g_TriggerManager.AddTrigger(mapGroup, mapTrigger);
		}	
		else
			debug_warn("Invalid group node child in XML file");
	}	//Read group children

	g_TriggerManager.AddGroup(mapGroup);
}

int CXMLReader::ReadEntities(XMBElement parent, double end_time)
{
	XMBElementList entities = parent.GetChildNodes();

	// If this is the first time in ReadEntities, find the next free ID number
	// in case we need to allocate new ones in the future
	if (entity_idx == 0)
	{
		int maxUnitID = -1;

		XERO_ITER_EL(parent, entity)
		{
			debug_assert(entity.GetNodeName() == el_entity);

			XMBAttributeList attrs = entity.GetAttributes();
			utf16string uid = attrs.GetNamedItem(at_uid);
			int unitId = uid.empty() ? -1 : CStr(uid).ToInt();
			maxUnitID = std::max(maxUnitID, unitId);
		}

		m_MapReader.pUnitMan->SetNextID(maxUnitID + 1);
	}

	while (entity_idx < entities.Count)
	{
		// all new state at this scope and below doesn't need to be
		// wrapped, since we only yield after a complete iteration.

		XMBElement entity = entities.Item(entity_idx++);
		debug_assert(entity.GetNodeName() == el_entity);

		XMBAttributeList attrs = entity.GetAttributes();
		utf16string uid = attrs.GetNamedItem(at_uid);
		int unitId = uid.empty() ? -1 : CStr(uid).ToInt();

		CStrW TemplateName;
		int PlayerID = 0;
		CVector3D Position;
		float Orientation = 0.f;

		XERO_ITER_EL(entity, setting)
		{
			int element_name = setting.GetNodeName();

			// <template>
			if (element_name == el_template)
			{
				TemplateName = setting.GetText();
			}
			// <player>
			else if (element_name == el_player)
			{
				PlayerID = CStr(setting.GetText()).ToInt();
			}
			// <position>
			else if (element_name == el_position)
			{
				XMBAttributeList attrs = setting.GetAttributes();
				Position = CVector3D(
					CStr(attrs.GetNamedItem(at_x)).ToFloat(),
					CStr(attrs.GetNamedItem(at_y)).ToFloat(),
					CStr(attrs.GetNamedItem(at_z)).ToFloat());
			}
			// <orientation>
			else if (element_name == el_orientation)
			{
				XMBAttributeList attrs = setting.GetAttributes();
				Orientation = CStr(attrs.GetNamedItem(at_angle)).ToFloat();
			}
			else
				debug_warn("Invalid map XML data");
		}

		CEntityTemplate* base = g_EntityTemplateCollection.GetTemplate(TemplateName, g_Game->GetPlayer(PlayerID));
		if (! base)
			LOG(CLogger::Error, LOG_CATEGORY, "Failed to load entity template '%ls'", TemplateName.c_str());
		else
		{
			std::set<CStr> selections; // TODO: read from file

			HEntity ent = g_EntityManager.Create(base, Position, Orientation, selections);

			if (! ent)
				LOG(CLogger::Error, LOG_CATEGORY, "Failed to create entity of type '%ls'", TemplateName.c_str());
			else
			{
				ent->m_actor->SetPlayerID(PlayerID);
				g_EntityManager.AddEntityClassData(ent);

				if (unitId < 0)
					ent->m_actor->SetID(m_MapReader.pUnitMan->GetNewID());
				else
					ent->m_actor->SetID(unitId);
			}
		}

		completed_jobs++;
		LDR_CHECK_TIMEOUT(completed_jobs, total_jobs);
	}

	return 0;
}


int CXMLReader::ReadNonEntities(XMBElement parent, double end_time)
{
	XMBElementList nonentities = parent.GetChildNodes();
	while (nonentity_idx < nonentities.Count)
	{
		// all new state at this scope and below doesn't need to be
		// wrapped, since we only yield after a complete iteration.

		XMBElement nonentity = nonentities.Item(nonentity_idx++);
		debug_assert(nonentity.GetNodeName() == el_nonentity);

		CStr ActorName;
		CVector3D Position;
		float Orientation = 0.f;

		XERO_ITER_EL(nonentity, setting)
		{
			int element_name = setting.GetNodeName();

			// <actor>
			if (element_name == el_actor)
			{
				ActorName = setting.GetText();
			}
			// <position>
			else if (element_name == el_position)
			{
				XMBAttributeList attrs = setting.GetAttributes();
				Position = CVector3D(
					CStr(attrs.GetNamedItem(at_x)).ToFloat(),
					CStr(attrs.GetNamedItem(at_y)).ToFloat(),
					CStr(attrs.GetNamedItem(at_z)).ToFloat());
			}
			// <orientation>
			else if (element_name == el_orientation)
			{
				XMBAttributeList attrs = setting.GetAttributes();
				Orientation = CStr(attrs.GetNamedItem(at_angle)).ToFloat();
			}
			else
				debug_warn("Invalid map XML data");
		}

		std::set<CStr> selections; // TODO: read from file

		CUnit* unit = m_MapReader.pUnitMan->CreateUnit(ActorName, NULL, selections);

		if (unit)
		{
			CMatrix3D m;
			m.SetYRotation(Orientation + PI);
			m.Translate(Position);
			unit->GetModel()->SetTransform(m);

			// TODO: save object IDs in the map file, and load them again,
			// so that triggers have a persistent identifier for objects
			unit->SetID(m_MapReader.pUnitMan->GetNewID());
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
	const double end_time = timer_Time() + 200e-3;

	int ret;

	while (node_idx < nodes.Count)
	{
		XMBElement node = nodes.Item(node_idx);
		CStr name = xmb_file.GetElementString(node.GetNodeName());
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
		else if (name == "Paths")
		{
			ReadCinema(node);
		}
		else if (name == "Triggers")
		{
			ReadTriggers(node);
		}
		else
		{
			debug_printf("Invalid XML element in map file: %s\n", name.c_str());
			debug_warn("Invalid map XML data");
		}

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
