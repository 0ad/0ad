/* Copyright (C) 2013 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "MapReader.h"

#include "graphics/Camera.h"
#include "graphics/CinemaTrack.h"
#include "graphics/Entity.h"
#include "graphics/GameView.h"
#include "graphics/MapGenerator.h"
#include "graphics/Patch.h"
#include "graphics/Terrain.h"
#include "graphics/TerrainTextureEntry.h"
#include "graphics/TerrainTextureManager.h"
#include "lib/timer.h"
#include "lib/external_libraries/libsdl.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/Loader.h"
#include "ps/LoaderThunks.h"
#include "ps/World.h"
#include "ps/XML/Xeromyces.h"
#include "renderer/PostprocManager.h"
#include "renderer/SkyManager.h"
#include "renderer/WaterManager.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpObstruction.h"
#include "simulation2/components/ICmpOwnership.h"
#include "simulation2/components/ICmpPlayer.h"
#include "simulation2/components/ICmpPlayerManager.h"
#include "simulation2/components/ICmpPosition.h"
#include "simulation2/components/ICmpTerrain.h"
#include "simulation2/components/ICmpVisual.h"
#include "simulation2/components/ICmpWaterManager.h"

#include <boost/algorithm/string/predicate.hpp>


CMapReader::CMapReader()
	: xml_reader(0), m_PatchesPerSide(0), m_MapGen(0)
{
	cur_terrain_tex = 0;	// important - resets generator state

	// Maps that don't override the default probably want the old lighting model
	//m_LightEnv.SetLightingModel("old");
	//pPostproc->SetPostEffect(L"default");
	
}

// LoadMap: try to load the map from given file; reinitialise the scene to new data if successful
void CMapReader::LoadMap(const VfsPath& pathname,  const CScriptValRooted& settings, CTerrain *pTerrain_,
						 WaterManager* pWaterMan_, SkyManager* pSkyMan_,
						 CLightEnv *pLightEnv_, CGameView *pGameView_, CCinemaManager* pCinema_, CTriggerManager* pTrigMan_, CPostprocManager* pPostproc_,
						 CSimulation2 *pSimulation2_, const CSimContext* pSimContext_, int playerID_, bool skipEntities)
{
	// latch parameters (held until DelayedLoadFinished)
	pTerrain = pTerrain_;
	pLightEnv = pLightEnv_;
	pGameView = pGameView_;
	pWaterMan = pWaterMan_;
	pSkyMan = pSkyMan_;
	pCinema = pCinema_;
	pTrigMan = pTrigMan_;
	pPostproc = pPostproc_;
	pSimulation2 = pSimulation2_;
	pSimContext = pSimContext_;
	m_PlayerID = playerID_;
	m_SkipEntities = skipEntities;
	m_StartingCameraTarget = INVALID_ENTITY;
	m_ScriptSettings = settings;

	filename_xml = pathname.ChangeExtension(L".xml");

	// In some cases (particularly tests) we don't want to bother storing a large
	// mostly-empty .pmp file, so we let the XML file specify basic terrain instead.
	// If there's an .xml file and no .pmp, then we're probably in this XML-only mode
	only_xml = false;
	if (!VfsFileExists(pathname) && VfsFileExists(filename_xml))
	{
		only_xml = true;
	}

	file_format_version = CMapIO::FILE_VERSION; // default if there's no .pmp

	if (!only_xml)
	{
		// [25ms]
		unpacker.Read(pathname, "PSMP");
		file_format_version = unpacker.GetVersion();
	}

	// check oldest supported version
	if (file_format_version < FILE_READ_VERSION)
		throw PSERROR_File_InvalidVersion();

	// delete all existing entities
	if (pSimulation2)
		pSimulation2->ResetState();
	
	// reset post effects
	if (pPostproc)
		pPostproc->SetPostEffect(L"default");

	// load map or script settings script
	if (settings.undefined())
		RegMemFun(this, &CMapReader::LoadScriptSettings, L"CMapReader::LoadScriptSettings", 50);
	else
		RegMemFun(this, &CMapReader::LoadRMSettings, L"CMapReader::LoadRMSettings", 50);

	// load player settings script (must be done before reading map)
	RegMemFun(this, &CMapReader::LoadPlayerSettings, L"CMapReader::LoadPlayerSettings", 50);

	// unpack the data
	if (!only_xml)
		RegMemFun(this, &CMapReader::UnpackMap, L"CMapReader::UnpackMap", 1200);

	// read the corresponding XML file
	RegMemFun(this, &CMapReader::ReadXML, L"CMapReader::ReadXML", 50);

	// apply terrain data to the world
	RegMemFun(this, &CMapReader::ApplyTerrainData, L"CMapReader::ApplyTerrainData", 5);

	// read entities
	RegMemFun(this, &CMapReader::ReadXMLEntities, L"CMapReader::ReadXMLEntities", 5800);

	// apply misc data to the world
	RegMemFun(this, &CMapReader::ApplyData, L"CMapReader::ApplyData", 5);

	// load map settings script (must be done after reading map)
	RegMemFun(this, &CMapReader::LoadMapSettings, L"CMapReader::LoadMapSettings", 5);

	RegMemFun(this, &CMapReader::DelayLoadFinished, L"CMapReader::DelayLoadFinished", 5);
}

// LoadRandomMap: try to load the map data; reinitialise the scene to new data if successful
void CMapReader::LoadRandomMap(const CStrW& scriptFile, const CScriptValRooted& settings, CTerrain *pTerrain_,
						 WaterManager* pWaterMan_, SkyManager* pSkyMan_,
						 CLightEnv *pLightEnv_, CGameView *pGameView_, CCinemaManager* pCinema_, CTriggerManager* pTrigMan_, CPostprocManager* pPostproc_,
						 CSimulation2 *pSimulation2_, int playerID_)
{
	// latch parameters (held until DelayedLoadFinished)
	m_ScriptFile = scriptFile;
	m_ScriptSettings = settings;
	pTerrain = pTerrain_;
	pLightEnv = pLightEnv_;
	pGameView = pGameView_;
	pWaterMan = pWaterMan_;
	pSkyMan = pSkyMan_;
	pCinema = pCinema_;
	pTrigMan = pTrigMan_;
	pPostproc = pPostproc_;
	pSimulation2 = pSimulation2_;
	pSimContext = pSimulation2 ? &pSimulation2->GetSimContext() : NULL;
	m_PlayerID = playerID_;
	m_SkipEntities = false;
	m_StartingCameraTarget = INVALID_ENTITY;

	// delete all existing entities
	if (pSimulation2)
		pSimulation2->ResetState();

	only_xml = false;

	// copy random map settings (before entity creation)
	RegMemFun(this, &CMapReader::LoadRMSettings, L"CMapReader::LoadRMSettings", 50);

	// load player settings script (must be done before reading map)
	RegMemFun(this, &CMapReader::LoadPlayerSettings, L"CMapReader::LoadPlayerSettings", 50);

	// load map generator with random map script
	RegMemFun(this, &CMapReader::GenerateMap, L"CMapReader::GenerateMap", 5000);

	// parse RMS results into terrain structure
	RegMemFun(this, &CMapReader::ParseTerrain, L"CMapReader::ParseTerrain", 500);

	// parse RMS results into environment settings
	RegMemFun(this, &CMapReader::ParseEnvironment, L"CMapReader::ParseEnvironment", 5);

	// parse RMS results into camera settings
	RegMemFun(this, &CMapReader::ParseCamera, L"CMapReader::ParseCamera", 5);

	// apply terrain data to the world
	RegMemFun(this, &CMapReader::ApplyTerrainData, L"CMapReader::ApplyTerrainData", 5);

	// parse RMS results into entities
	RegMemFun(this, &CMapReader::ParseEntities, L"CMapReader::ParseEntities", 1000);

	// apply misc data to the world
	RegMemFun(this, &CMapReader::ApplyData, L"CMapReader::ApplyData", 5);

	// load map settings script (must be done after reading map)
	RegMemFun(this, &CMapReader::LoadMapSettings, L"CMapReader::LoadMapSettings", 5);

	RegMemFun(this, &CMapReader::DelayLoadFinished, L"CMapReader::DelayLoadFinished", 5);
}

// UnpackMap: unpack the given data from the raw data stream into local variables
int CMapReader::UnpackMap()
{
	// now unpack everything into local data
	int ret = UnpackTerrain();
	if (ret != 0)	// failed or timed out
	{
		return ret;
	}

	return 0;
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
		m_PatchesPerSide = (ssize_t)unpacker.UnpackSize();

		// unpack heightmap [600us]
		size_t verticesPerSide = m_PatchesPerSide*PATCH_SIZE+1;
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

		ENSURE(CTerrainTextureManager::IsInitialised()); // we need this for the terrain properties (even when graphics are disabled)
		CTerrainTextureEntry* texentry = g_TexMan.FindTexture(texturename);
		m_TerrainTextures.push_back(texentry);

		cur_terrain_tex++;
		LDR_CHECK_TIMEOUT(cur_terrain_tex, num_terrain_tex);
	}

	// unpack tile data [3ms]
	ssize_t tilesPerSide = m_PatchesPerSide*PATCH_SIZE;
	m_Tiles.resize(size_t(SQR(tilesPerSide)));
	unpacker.UnpackRaw(&m_Tiles[0], sizeof(STileDesc)*m_Tiles.size());

	// reset generator state.
	cur_terrain_tex = 0;

	return 0;
}

int CMapReader::ApplyTerrainData()
{
	if (m_PatchesPerSide == 0)
	{
		// we'll probably crash when trying to use this map later
		throw PSERROR_Game_World_MapLoadFailed("Error loading map: no terrain data.\nCheck application log for details.");
	}

	if (!only_xml)
	{
		// initialise the terrain
		pTerrain->Initialize(m_PatchesPerSide, &m_Heightmap[0]);

		// setup the textures on the minipatches
		STileDesc* tileptr = &m_Tiles[0];
		for (ssize_t j=0; j<m_PatchesPerSide; j++) {
			for (ssize_t i=0; i<m_PatchesPerSide; i++) {
				for (ssize_t m=0; m<PATCH_SIZE; m++) {
					for (ssize_t k=0; k<PATCH_SIZE; k++) {
						CMiniPatch& mp = pTerrain->GetPatch(i,j)->m_MiniPatches[m][k];	// can't fail

						mp.Tex = m_TerrainTextures[tileptr->m_Tex1Index];
						mp.Priority = tileptr->m_Priority;
	
						tileptr++;
					}
				}
			}
		}
	}

	CmpPtr<ICmpTerrain> cmpTerrain(*pSimContext, SYSTEM_ENTITY);
	if (cmpTerrain)
		cmpTerrain->ReloadTerrain();

	return 0;
}

// ApplyData: take all the input data, and rebuild the scene from it
int CMapReader::ApplyData()
{
	// copy over the lighting parameters
	if (pLightEnv)
		*pLightEnv = m_LightEnv;

	CmpPtr<ICmpPlayerManager> cmpPlayerManager(*pSimContext, SYSTEM_ENTITY);

	if (pGameView && cmpPlayerManager)
	{
		// Default to global camera (with constraints)
		pGameView->ResetCameraTarget(pGameView->GetCamera()->GetFocus());
	
		// TODO: Starting rotation?
		CmpPtr<ICmpPlayer> cmpPlayer(*pSimContext, cmpPlayerManager->GetPlayerByID(m_PlayerID));
		if (cmpPlayer && cmpPlayer->HasStartingCamera())
		{
			// Use player starting camera
			CFixedVector3D pos = cmpPlayer->GetStartingCameraPos();
			pGameView->ResetCameraTarget(CVector3D(pos.X.ToFloat(), pos.Y.ToFloat(), pos.Z.ToFloat()));
		}
		else if (m_StartingCameraTarget != INVALID_ENTITY)
		{
			// Point camera at entity
			CmpPtr<ICmpPosition> cmpPosition(*pSimContext, m_StartingCameraTarget);
			if (cmpPosition)
			{
				CFixedVector3D pos = cmpPosition->GetPosition();
				pGameView->ResetCameraTarget(CVector3D(pos.X.ToFloat(), pos.Y.ToFloat(), pos.Z.ToFloat()));
			}
		}
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////


PSRETURN CMapSummaryReader::LoadMap(const VfsPath& pathname)
{
	VfsPath filename_xml = pathname.ChangeExtension(L".xml");

	CXeromyces xmb_file;
	if (xmb_file.Load(g_VFS, filename_xml) != PSRETURN_OK)
		return PSRETURN_File_ReadFailed;

	// Define all the relevant elements used in the XML file
	#define EL(x) int el_##x = xmb_file.GetElementID(#x)
	#define AT(x) int at_##x = xmb_file.GetAttributeID(#x)
	EL(scenario);
	EL(scriptsettings);
	#undef AT
	#undef EL

	XMBElement root = xmb_file.GetRoot();
	ENSURE(root.GetNodeName() == el_scenario);

	XERO_ITER_EL(root, child)
	{
		int child_name = child.GetNodeName();
		if (child_name == el_scriptsettings)
		{
			m_ScriptSettings = child.GetText();
		}
	}

	return PSRETURN_OK;
}

CScriptValRooted CMapSummaryReader::GetMapSettings(ScriptInterface& scriptInterface)
{
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);
	
	JS::RootedValue data(cx);
	scriptInterface.Eval("({})", &data);
	if (!m_ScriptSettings.empty())
		scriptInterface.SetProperty(data, "settings", scriptInterface.ParseJSON(m_ScriptSettings), false);
	return CScriptValRooted(cx, data);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Holds various state data while reading maps, so that loading can be
// interrupted (e.g. to update the progress display) then later resumed.
class CXMLReader
{
	NONCOPYABLE(CXMLReader);
public:
	CXMLReader(const VfsPath& xml_filename, CMapReader& mapReader)
		: m_MapReader(mapReader)
	{
		Init(xml_filename);
	}

	CStr ReadScriptSettings();

	// read everything except for entities
	void ReadXML();

	// return semantics: see Loader.cpp!LoadFunc.
	int ProgressiveReadEntities();

private:
	CXeromyces xmb_file;

	CMapReader& m_MapReader;

	int el_entity;
	int el_tracks;
	int el_template, el_player;
	int el_position, el_orientation, el_obstruction;
	int el_actor;
	int at_x, at_y, at_z;
	int at_group, at_group2;
	int at_id;
	int at_angle;
	int at_uid;
	int at_seed;

	XMBElementList nodes; // children of root

	// loop counters
	int node_idx;
	int entity_idx;

	// # entities+nonentities processed and total (for progress calc)
	int completed_jobs, total_jobs;

	// maximum used entity ID, so we can safely allocate new ones
	entity_id_t max_uid;

	void Init(const VfsPath& xml_filename);

	void ReadTerrain(XMBElement parent);
	void ReadEnvironment(XMBElement parent);
	void ReadCamera(XMBElement parent);
	void ReadCinema(XMBElement parent);
	void ReadTriggers(XMBElement parent);
	int ReadEntities(XMBElement parent, double end_time);
};


void CXMLReader::Init(const VfsPath& xml_filename)
{
	// must only assign once, so do it here
	node_idx = entity_idx = 0;

	if (xmb_file.Load(g_VFS, xml_filename) != PSRETURN_OK)
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
	EL(obstruction);
	EL(actor);
	AT(x); AT(y); AT(z);
	AT(group); AT(group2);
	AT(angle);
	AT(uid);
	AT(seed);
#undef AT
#undef EL

	XMBElement root = xmb_file.GetRoot();
	ENSURE(xmb_file.GetElementString(root.GetNodeName()) == "Scenario");
	nodes = root.GetChildNodes();

	// find out total number of entities+nonentities
	// (used when calculating progress)
	completed_jobs = 0;
	total_jobs = 0;
	for (int i = 0; i < nodes.Count; i++)
		total_jobs += nodes.Item(i).GetChildNodes().Count;

	// Find the maximum entity ID, so we can safely allocate new IDs without conflicts

	max_uid = SYSTEM_ENTITY;

	XMBElement ents = nodes.GetFirstNamedItem(xmb_file.GetElementID("Entities"));
	XERO_ITER_EL(ents, ent)
	{
		CStr uid = ent.GetAttributes().GetNamedItem(at_uid);
		max_uid = std::max(max_uid, (entity_id_t)uid.ToUInt());
	}
}


CStr CXMLReader::ReadScriptSettings()
{
	XMBElement root = xmb_file.GetRoot();
	ENSURE(xmb_file.GetElementString(root.GetNodeName()) == "Scenario");
	nodes = root.GetChildNodes();

	XMBElement settings = nodes.GetFirstNamedItem(xmb_file.GetElementID("ScriptSettings"));

	return settings.GetText();
}


void CXMLReader::ReadTerrain(XMBElement parent)
{
#define AT(x) int at_##x = xmb_file.GetAttributeID(#x)
	AT(patches);
	AT(texture);
	AT(priority);
	AT(height);
#undef AT

	ssize_t patches = 9;
	CStr texture = "grass1_spring";
	int priority = 0;
	u16 height = 16384;

	XERO_ITER_ATTR(parent, attr)
	{
		if (attr.Name == at_patches)
			patches = attr.Value.ToInt();
		else if (attr.Name == at_texture)
			texture = attr.Value;
		else if (attr.Name == at_priority)
			priority = attr.Value.ToInt();
		else if (attr.Name == at_height)
			height = (u16)attr.Value.ToInt();
	}

	m_MapReader.m_PatchesPerSide = patches;

	// Load the texture
	ENSURE(CTerrainTextureManager::IsInitialised()); // we need this for the terrain properties (even when graphics are disabled)
	CTerrainTextureEntry* texentry = g_TexMan.FindTexture(texture);

	m_MapReader.pTerrain->Initialize(patches, NULL);

	// Fill the heightmap
	u16* heightmap = m_MapReader.pTerrain->GetHeightMap();
	ssize_t verticesPerSide = m_MapReader.pTerrain->GetVerticesPerSide();
	for (ssize_t i = 0; i < SQR(verticesPerSide); ++i)
		heightmap[i] = height;

	// Fill the texture map
	for (ssize_t pz = 0; pz < patches; ++pz)
	{
		for (ssize_t px = 0; px < patches; ++px)
		{
			CPatch* patch = m_MapReader.pTerrain->GetPatch(px, pz);	// can't fail

			for (ssize_t z = 0; z < PATCH_SIZE; ++z)
			{
				for (ssize_t x = 0; x < PATCH_SIZE; ++x)
				{
					patch->m_MiniPatches[z][x].Tex = texentry;
					patch->m_MiniPatches[z][x].Priority = priority;
				}
			}
		}
	}
}

void CXMLReader::ReadEnvironment(XMBElement parent)
{
#define EL(x) int el_##x = xmb_file.GetElementID(#x)
#define AT(x) int at_##x = xmb_file.GetAttributeID(#x)
	EL(lightingmodel);
	EL(posteffect);
	EL(skyset);
	EL(suncolour);
	EL(sunelevation);
	EL(sunrotation);
	EL(terrainambientcolour);
	EL(unitsambientcolour);
	EL(water);
	EL(waterbody);
	EL(type);
	EL(colour);
	EL(tint);
	EL(height);
	EL(shininess);	// for compatibility
	EL(waviness);
	EL(murkiness);
	EL(windangle);
	EL(reflectiontint);	// for compatibility
	EL(reflectiontintstrength);	// for compatibility
	EL(fog);
	EL(fogcolour);
	EL(fogfactor);
	EL(fogthickness);
	EL(postproc);
	EL(brightness);
	EL(contrast);
	EL(saturation);
	EL(bloom);
	AT(r); AT(g); AT(b);
#undef AT
#undef EL

	XERO_ITER_EL(parent, element)
	{
		int element_name = element.GetNodeName();

		XMBAttributeList attrs = element.GetAttributes();

		if (element_name == el_lightingmodel)
		{
			// NOP - obsolete.
		}
		else if (element_name == el_skyset)
		{
			if (m_MapReader.pSkyMan)
				m_MapReader.pSkyMan->SetSkySet(element.GetText().FromUTF8());
		}
		else if (element_name == el_suncolour)
		{
			m_MapReader.m_LightEnv.m_SunColor = RGBColor(
				attrs.GetNamedItem(at_r).ToFloat(),
				attrs.GetNamedItem(at_g).ToFloat(),
				attrs.GetNamedItem(at_b).ToFloat());
		}
		else if (element_name == el_sunelevation)
		{
			m_MapReader.m_LightEnv.m_Elevation = attrs.GetNamedItem(at_angle).ToFloat();
		}
		else if (element_name == el_sunrotation)
		{
			m_MapReader.m_LightEnv.m_Rotation = attrs.GetNamedItem(at_angle).ToFloat();
		}
		else if (element_name == el_terrainambientcolour)
		{
			m_MapReader.m_LightEnv.m_TerrainAmbientColor = RGBColor(
				attrs.GetNamedItem(at_r).ToFloat(),
				attrs.GetNamedItem(at_g).ToFloat(),
				attrs.GetNamedItem(at_b).ToFloat());
		}
		else if (element_name == el_unitsambientcolour)
		{
			m_MapReader.m_LightEnv.m_UnitsAmbientColor = RGBColor(
				attrs.GetNamedItem(at_r).ToFloat(),
				attrs.GetNamedItem(at_g).ToFloat(),
				attrs.GetNamedItem(at_b).ToFloat());
		}
		else if (element_name == el_fog)
		{
			XERO_ITER_EL(element, fog)
			{
				int element_name = fog.GetNodeName();
				if (element_name == el_fogcolour)
				{
					XMBAttributeList attrs = fog.GetAttributes();
					m_MapReader.m_LightEnv.m_FogColor = RGBColor(
						attrs.GetNamedItem(at_r).ToFloat(),
						attrs.GetNamedItem(at_g).ToFloat(),
						attrs.GetNamedItem(at_b).ToFloat());
				}
				else if (element_name == el_fogfactor)
				{
					m_MapReader.m_LightEnv.m_FogFactor = fog.GetText().ToFloat();
				}
				else if (element_name == el_fogthickness)
				{
					m_MapReader.m_LightEnv.m_FogMax = fog.GetText().ToFloat();
				}
			}
		}
		else if (element_name == el_postproc)
		{
			XERO_ITER_EL(element, postproc)
			{
				int element_name = postproc.GetNodeName();
				if (element_name == el_brightness)
				{
					m_MapReader.m_LightEnv.m_Brightness = postproc.GetText().ToFloat();
				}
				else if (element_name == el_contrast)
				{
					m_MapReader.m_LightEnv.m_Contrast = postproc.GetText().ToFloat();
				}
				else if (element_name == el_saturation)
				{
					m_MapReader.m_LightEnv.m_Saturation = postproc.GetText().ToFloat();
				}
				else if (element_name == el_bloom)
				{
					m_MapReader.m_LightEnv.m_Bloom = postproc.GetText().ToFloat();
				}
				else if (element_name == el_posteffect)
				{
					if (m_MapReader.pPostproc)
						m_MapReader.pPostproc->SetPostEffect(postproc.GetText().FromUTF8());
				}
			}
		}
		else if (element_name == el_water)
		{
			XERO_ITER_EL(element, waterbody)
			{
				ENSURE(waterbody.GetNodeName() == el_waterbody);
				XERO_ITER_EL(waterbody, waterelement)
				{
					int element_name = waterelement.GetNodeName();
					if (element_name == el_height)
					{
						CmpPtr<ICmpWaterManager> cmpWaterManager(*m_MapReader.pSimContext, SYSTEM_ENTITY);
						ENSURE(cmpWaterManager);
						cmpWaterManager->SetWaterLevel(entity_pos_t::FromString(waterelement.GetText()));
						continue;
					}

					// The rest are purely graphical effects, and should be ignored if
					// graphics are disabled
					if (!m_MapReader.pWaterMan)
						continue;
					
					if (element_name == el_type)
					{
						if (waterelement.GetText() == "default")
							m_MapReader.pWaterMan->m_WaterType = L"ocean";
						else
							m_MapReader.pWaterMan->m_WaterType =  waterelement.GetText().FromUTF8();
					}
					else if (element_name == el_shininess || element_name == el_reflectiontint || element_name == el_reflectiontintstrength)
					{
						// deprecated.
					}
#define READ_COLOUR(el, out) \
					else if (element_name == el) \
					{ \
						XMBAttributeList attrs = waterelement.GetAttributes(); \
						out = CColor( \
							attrs.GetNamedItem(at_r).ToFloat(), \
							attrs.GetNamedItem(at_g).ToFloat(), \
							attrs.GetNamedItem(at_b).ToFloat(), \
							1.f); \
					}

#define READ_FLOAT(el, out) \
					else if (element_name == el) \
					{ \
						out = waterelement.GetText().ToFloat(); \
					} \

					READ_COLOUR(el_colour, m_MapReader.pWaterMan->m_WaterColor)
					READ_COLOUR(el_tint, m_MapReader.pWaterMan->m_WaterTint)
					READ_FLOAT(el_waviness, m_MapReader.pWaterMan->m_Waviness)
					READ_FLOAT(el_murkiness, m_MapReader.pWaterMan->m_Murkiness)
					READ_FLOAT(el_windangle, m_MapReader.pWaterMan->m_WindAngle)

#undef READ_FLOAT
#undef READ_COLOUR

					else
						debug_warn(L"Invalid map XML data");
				}

			}
		}
		else
			debug_warn(L"Invalid map XML data");
	}

	m_MapReader.m_LightEnv.CalculateSunDirection();
}

void CXMLReader::ReadCamera(XMBElement parent)
{
	// defaults if we don't find player starting camera
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
			declination = attrs.GetNamedItem(at_angle).ToFloat();
		}
		else if (element_name == el_rotation)
		{
			rotation = attrs.GetNamedItem(at_angle).ToFloat();
		}
		else if (element_name == el_position)
		{
			translation = CVector3D(
				attrs.GetNamedItem(at_x).ToFloat(),
				attrs.GetNamedItem(at_y).ToFloat(),
				attrs.GetNamedItem(at_z).ToFloat());
		}
		else
			debug_warn(L"Invalid map XML data");
	}

	if (m_MapReader.pGameView)
	{
		m_MapReader.pGameView->GetCamera()->m_Orientation.SetXRotation(declination);
		m_MapReader.pGameView->GetCamera()->m_Orientation.RotateY(rotation);
		m_MapReader.pGameView->GetCamera()->m_Orientation.Translate(translation);
		m_MapReader.pGameView->GetCamera()->UpdateFrustum();
	}
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
			CStrW name(attrs.GetNamedItem(at_name).FromUTF8());
			float timescale = attrs.GetNamedItem(at_timescale).ToFloat();
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
						pathData.m_Mode = attrs.GetNamedItem(at_mode).ToInt();
						pathData.m_Style = attrs.GetNamedItem(at_style).ToInt();
						pathData.m_Growth = attrs.GetNamedItem(at_growth).ToInt();
						pathData.m_Switch = attrs.GetNamedItem(at_switch).ToInt();
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
							data.Position.X = attrs.GetNamedItem(at_x).ToFloat();
							data.Position.Y = attrs.GetNamedItem(at_y).ToFloat();
							data.Position.Z = attrs.GetNamedItem(at_z).ToFloat();
							continue;
						}
						else if ( elementName == el_rotation )
						{
							data.Rotation.X = attrs.GetNamedItem(at_x).ToFloat();
							data.Rotation.Y = attrs.GetNamedItem(at_y).ToFloat();
							data.Rotation.Z = attrs.GetNamedItem(at_z).ToFloat();
							continue;
						}
						else if ( elementName == el_time )
							data.Distance = nodeChild.GetText().ToFloat();
						else 
							debug_warn(L"Invalid cinematic element for node child");
					
						backwardSpline.AddNode(data.Position, data.Rotation, data.Distance);
					}
				}
				else
					debug_warn(L"Invalid cinematic element for path child");
				
				
			}

			//Construct cinema path with data gathered
			CCinemaPath temp(pathData, backwardSpline);
			const std::vector<SplineData>& nodes = temp.GetAllNodes();
			if ( nodes.empty() )
			{
				debug_warn(L"Failure loading cinematics");
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
			ENSURE("Invalid cinema child");
	}

	if (m_MapReader.pCinema)
		m_MapReader.pCinema->SetAllPaths(pathList);
}

void CXMLReader::ReadTriggers(XMBElement UNUSED(parent))
{
}

int CXMLReader::ReadEntities(XMBElement parent, double end_time)
{
	XMBElementList entities = parent.GetChildNodes();

	ENSURE(m_MapReader.pSimulation2);
	CSimulation2& sim = *m_MapReader.pSimulation2;
	CmpPtr<ICmpPlayerManager> cmpPlayerManager(sim, SYSTEM_ENTITY);

	while (entity_idx < entities.Count)
	{
		// all new state at this scope and below doesn't need to be
		// wrapped, since we only yield after a complete iteration.

		XMBElement entity = entities.Item(entity_idx++);
		ENSURE(entity.GetNodeName() == el_entity);

		XMBAttributeList attrs = entity.GetAttributes();
		CStr uid = attrs.GetNamedItem(at_uid);
		ENSURE(!uid.empty());
		int EntityUid = uid.ToInt();

		CStrW TemplateName;
		int PlayerID = 0;
		CFixedVector3D Position;
		CFixedVector3D Orientation;
		long Seed = -1;

		// Obstruction control groups.
		entity_id_t ControlGroup = INVALID_ENTITY;
		entity_id_t ControlGroup2 = INVALID_ENTITY;

		XERO_ITER_EL(entity, setting)
		{
			int element_name = setting.GetNodeName();

			// <template>
			if (element_name == el_template)
			{
				TemplateName = setting.GetText().FromUTF8();
			}
			// <player>
			else if (element_name == el_player)
			{
				PlayerID = setting.GetText().ToInt();
			}
			// <position>
			else if (element_name == el_position)
			{
				XMBAttributeList attrs = setting.GetAttributes();
				Position = CFixedVector3D(
					fixed::FromString(attrs.GetNamedItem(at_x)),
					fixed::FromString(attrs.GetNamedItem(at_y)),
					fixed::FromString(attrs.GetNamedItem(at_z)));
			}
			// <orientation>
			else if (element_name == el_orientation)
			{
				XMBAttributeList attrs = setting.GetAttributes();
				Orientation = CFixedVector3D(
					fixed::FromString(attrs.GetNamedItem(at_x)),
					fixed::FromString(attrs.GetNamedItem(at_y)),
					fixed::FromString(attrs.GetNamedItem(at_z)));
				// TODO: what happens if some attributes are missing?
			}
			// <obstruction>
			else if (element_name == el_obstruction)
			{
				XMBAttributeList attrs = setting.GetAttributes();
				ControlGroup = attrs.GetNamedItem(at_group).ToInt();
				ControlGroup2 = attrs.GetNamedItem(at_group2).ToInt();
			}
			// <actor>
			else if (element_name == el_actor)
			{
				XMBAttributeList attrs = setting.GetAttributes();
				CStr seedStr = attrs.GetNamedItem(at_seed);
				if (!seedStr.empty())
				{
					Seed = seedStr.ToLong();
					ENSURE(Seed >= 0);
				}
			}
			else
				debug_warn(L"Invalid map XML data");
		}

		entity_id_t ent = sim.AddEntity(TemplateName, EntityUid);
		entity_id_t player = cmpPlayerManager->GetPlayerByID(PlayerID);
		if (ent == INVALID_ENTITY || player == INVALID_ENTITY)
		{	// Don't add entities with invalid player IDs
			LOGERROR(L"Failed to load entity template '%ls'", TemplateName.c_str());
		}
		else
		{
			CmpPtr<ICmpPosition> cmpPosition(sim, ent);
			if (cmpPosition)
			{
				cmpPosition->JumpTo(Position.X, Position.Z);
				cmpPosition->SetYRotation(Orientation.Y);
				// TODO: other parts of the position
			}

			CmpPtr<ICmpOwnership> cmpOwnership(sim, ent);
			if (cmpOwnership)
				cmpOwnership->SetOwner(PlayerID);

			CmpPtr<ICmpObstruction> cmpObstruction(sim, ent);
			if (cmpObstruction)
			{
				if (ControlGroup != INVALID_ENTITY)
					cmpObstruction->SetControlGroup(ControlGroup);
				if (ControlGroup2 != INVALID_ENTITY)
					cmpObstruction->SetControlGroup2(ControlGroup2);

				cmpObstruction->ResolveFoundationCollisions();
			}

			CmpPtr<ICmpVisual> cmpVisual(sim, ent);
			if (cmpVisual)
			{
				if (Seed != -1)
					cmpVisual->SetActorSeed((u32)Seed);
				// TODO: variation/selection strings
			}

			if (PlayerID == m_MapReader.m_PlayerID && (boost::algorithm::ends_with(TemplateName, L"civil_centre") || m_MapReader.m_StartingCameraTarget == INVALID_ENTITY))
			{
				// Focus on civil centre or first entity owned by player
				m_MapReader.m_StartingCameraTarget = ent;
			}
		}

		completed_jobs++;
		LDR_CHECK_TIMEOUT(completed_jobs, total_jobs);
	}

	return 0;
}

void CXMLReader::ReadXML()
{
	for (int i = 0; i < nodes.Count; ++i)
	{
		XMBElement node = nodes.Item(i);
		CStr name = xmb_file.GetElementString(node.GetNodeName());
		if (name == "Terrain")
		{
			ReadTerrain(node);
		}
		else if (name == "Environment")
		{
			ReadEnvironment(node);
		}
		else if (name == "Camera")
		{
			ReadCamera(node);
		}
		else if (name == "ScriptSettings")
		{
			// Already loaded - this is to prevent an assertion
		}
		else if (name == "Entities")
		{
			// Handled by ProgressiveReadEntities instead
		}
		else if (name == "Paths")
		{
			ReadCinema(node);
		}
		else if (name == "Triggers")
		{
			ReadTriggers(node);
		}
		else if (name == "Script")
		{
			if (m_MapReader.pSimulation2)
				m_MapReader.pSimulation2->SetStartupScript(node.GetText());
		}
		else
		{
			debug_printf(L"Invalid XML element in map file: %hs\n", name.c_str());
			debug_warn(L"Invalid map XML data");
		}
	}
}

int CXMLReader::ProgressiveReadEntities()
{
	// yield after this time is reached. balances increased progress bar
	// smoothness vs. slowing down loading.
	const double end_time = timer_Time() + 200e-3;

	int ret;

	while (node_idx < nodes.Count)
	{
		XMBElement node = nodes.Item(node_idx);
		CStr name = xmb_file.GetElementString(node.GetNodeName());
		if (name == "Entities")
		{
			if (!m_MapReader.m_SkipEntities)
			{
				ret = ReadEntities(node, end_time);
				if (ret != 0)	// error or timed out
					return ret;
			}
		}

		node_idx++;
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// load script settings from map
int CMapReader::LoadScriptSettings()
{
	if (!xml_reader)
		xml_reader = new CXMLReader(filename_xml, *this);

	// parse the script settings
	if (pSimulation2)
		pSimulation2->SetMapSettings(xml_reader->ReadScriptSettings());

	return 0;
}

// load player settings script
int CMapReader::LoadPlayerSettings()
{
	if (pSimulation2)
		pSimulation2->LoadPlayerSettings(true);
	return 0;
}

// load map settings script
int CMapReader::LoadMapSettings()
{
	if (pSimulation2)
		pSimulation2->LoadMapSettings();
	return 0;
}

int CMapReader::ReadXML()
{
	if (!xml_reader)
		xml_reader = new CXMLReader(filename_xml, *this);

	xml_reader->ReadXML();

	return 0;
}

// progressive
int CMapReader::ReadXMLEntities()
{
	if (!xml_reader)
		xml_reader = new CXMLReader(filename_xml, *this);

	int ret = xml_reader->ProgressiveReadEntities();
	// finished or failed
	if (ret <= 0)
	{
		SAFE_DELETE(xml_reader);
	}

	return ret;
}

int CMapReader::DelayLoadFinished()
{
	// we were dynamically allocated by CWorld::Initialize
	delete this;

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////


int CMapReader::LoadRMSettings()
{
	// copy random map settings over to sim
	ENSURE(pSimulation2);
	pSimulation2->SetMapSettings(m_ScriptSettings);

	return 0;
}

int CMapReader::GenerateMap()
{
	if (!m_MapGen)
	{
		// Initialize map generator
		m_MapGen = new CMapGenerator();

		VfsPath scriptPath;
		
		if (m_ScriptFile.length())
			scriptPath = L"maps/random/"+m_ScriptFile;

		// Stringify settings to pass across threads
		std::string scriptSettings = pSimulation2->GetScriptInterface().StringifyJSON(m_ScriptSettings.get());
		
		// Try to generate map
		m_MapGen->GenerateMap(scriptPath, scriptSettings);
	}

	// Check status
	int progress = m_MapGen->GetProgress();
	if (progress < 0)
	{
		// RMS failed - return to main menu
		throw PSERROR_Game_World_MapLoadFailed("Error generating random map.\nCheck application log for details.");
	}
	else if (progress == 0)
	{
		// Finished, get results as StructuredClone object, which must be read to obtain the JS val
		shared_ptr<ScriptInterface::StructuredClone> results = m_MapGen->GetResults();

		// Parse data into simulation context
		CScriptValRooted data(pSimulation2->GetScriptInterface().GetContext(), pSimulation2->GetScriptInterface().ReadStructuredClone(results));
		if (data.undefined())
		{
			// RMS failed - return to main menu
			throw PSERROR_Game_World_MapLoadFailed("Error generating random map.\nCheck application log for details.");
		}
		else
		{
			m_MapData = data;
		}
	}
	else
	{
		// Still working

		// Sleep for a while, slowing down the rendering thread
		// to allow more CPU for the map generator thread
		SDL_Delay(100);
	}
	
	// return progress
	return progress;
};


int CMapReader::ParseTerrain()
{
	TIMER(L"ParseTerrain");
	JSContext* cx = pSimulation2->GetScriptInterface().GetContext();
	JSAutoRequest rq(cx);
	
	// parse terrain from map data
	//	an error here should stop the loading process
#define GET_TERRAIN_PROPERTY(val, prop, out)\
	if (!pSimulation2->GetScriptInterface().GetProperty(val, #prop, out))\
		{	LOGERROR(L"CMapReader::ParseTerrain() failed to get '%hs' property", #prop);\
			throw PSERROR_Game_World_MapLoadFailed("Error parsing terrain data.\nCheck application log for details"); }

	JS::RootedValue tmpMapData(cx, m_MapData.get()); // TODO: Check if this temporary root can be removed after SpiderMonkey 31 upgrade 
	u32 size;
	GET_TERRAIN_PROPERTY(tmpMapData, size, size)

	m_PatchesPerSide = size / PATCH_SIZE;

	// flat heightmap of u16 data
	GET_TERRAIN_PROPERTY(tmpMapData, height, m_Heightmap)

	// load textures
	std::vector<std::string> textureNames;
	GET_TERRAIN_PROPERTY(tmpMapData, textureNames, textureNames)
	num_terrain_tex = textureNames.size();

	while (cur_terrain_tex < num_terrain_tex)
	{
		ENSURE(CTerrainTextureManager::IsInitialised()); // we need this for the terrain properties (even when graphics are disabled)
		CTerrainTextureEntry* texentry = g_TexMan.FindTexture(textureNames[cur_terrain_tex]);
		m_TerrainTextures.push_back(texentry);

		cur_terrain_tex++;
	}

	// build tile data
	m_Tiles.resize(SQR(size));

	JS::RootedValue tileData(cx);
	GET_TERRAIN_PROPERTY(tmpMapData, tileData, &tileData)

	// parse tile data object into flat arrays
	std::vector<u16> tileIndex;
	std::vector<u16> tilePriority;
	GET_TERRAIN_PROPERTY(tileData, index, tileIndex);
	GET_TERRAIN_PROPERTY(tileData, priority, tilePriority);

	ENSURE(SQR(size) == tileIndex.size() && SQR(size) == tilePriority.size());

	// reorder by patches and store
	for (size_t x = 0; x < size; ++x)
	{
		size_t patchX = x / PATCH_SIZE;
		size_t offX = x % PATCH_SIZE;
		for (size_t y = 0; y < size; ++y)
		{
			size_t patchY = y / PATCH_SIZE;
			size_t offY = y % PATCH_SIZE;
			
			STileDesc tile;
			tile.m_Tex1Index = tileIndex[y*size + x];
			tile.m_Tex2Index = 0xFFFF;
			tile.m_Priority = tilePriority[y*size + x];

			m_Tiles[(patchY * m_PatchesPerSide + patchX) * SQR(PATCH_SIZE) + (offY * PATCH_SIZE + offX)] = tile;
		}
	}

	// reset generator state
	cur_terrain_tex = 0;

#undef GET_TERRAIN_PROPERTY

	return 0;
}

int CMapReader::ParseEntities()
{
	TIMER(L"ParseEntities");
	JSContext* cx = pSimulation2->GetScriptInterface().GetContext();
	JSAutoRequest rq(cx);
	
	JS::RootedValue tmpMapData(cx, m_MapData.get()); // TODO: Check if this temporary root can be removed after SpiderMonkey 31 upgrade 

	// parse entities from map data
	std::vector<Entity> entities;

	if (!pSimulation2->GetScriptInterface().GetProperty(tmpMapData, "entities", entities))
		LOGWARNING(L"CMapReader::ParseEntities() failed to get 'entities' property");

	CSimulation2& sim = *pSimulation2;
	CmpPtr<ICmpPlayerManager> cmpPlayerManager(sim, SYSTEM_ENTITY);

	size_t entity_idx = 0;
	size_t num_entities = entities.size();
	
	Entity currEnt;

	while (entity_idx < num_entities)
	{
		// Get current entity struct
		currEnt = entities[entity_idx];

		entity_id_t ent = pSimulation2->AddEntity(currEnt.templateName, currEnt.entityID);
		entity_id_t player = cmpPlayerManager->GetPlayerByID(currEnt.playerID);
		if (ent == INVALID_ENTITY || player == INVALID_ENTITY)
		{	// Don't add entities with invalid player IDs
			LOGERROR(L"Failed to load entity template '%ls'", currEnt.templateName.c_str());
		}
		else
		{
			CmpPtr<ICmpPosition> cmpPosition(sim, ent);
			if (cmpPosition)
			{
				cmpPosition->JumpTo(currEnt.position.X, currEnt.position.Z);
				cmpPosition->SetYRotation(currEnt.rotation.Y);
				// TODO: other parts of the position
			}

			CmpPtr<ICmpOwnership> cmpOwnership(sim, ent);
			if (cmpOwnership)
				cmpOwnership->SetOwner(currEnt.playerID);

			// Detect and fix collisions between foundation-blocking entities.
			// This presently serves to copy wall tower control groups to wall
			// segments, allowing players to expand RMS-generated walls.
			CmpPtr<ICmpObstruction> cmpObstruction(sim, ent);
			if (cmpObstruction)
				cmpObstruction->ResolveFoundationCollisions();

			if (currEnt.playerID == m_PlayerID && (boost::algorithm::ends_with(currEnt.templateName, L"civil_centre") || m_StartingCameraTarget == INVALID_ENTITY))
			{
				// Focus on civil centre or first entity owned by player
				m_StartingCameraTarget = currEnt.entityID;
			}
		}

		entity_idx++;
	}

	return 0;
}

int CMapReader::ParseEnvironment()
{
	// parse environment settings from map data
	JSContext* cx = pSimulation2->GetScriptInterface().GetContext();
	JSAutoRequest rq(cx);
	
	JS::RootedValue tmpMapData(cx, m_MapData.get()); // TODO: Check if this temporary root can be removed after SpiderMonkey 31 upgrade 

#define GET_ENVIRONMENT_PROPERTY(val, prop, out)\
	if (!pSimulation2->GetScriptInterface().GetProperty(val, #prop, out))\
		LOGWARNING(L"CMapReader::ParseEnvironment() failed to get '%hs' property", #prop);

	JS::RootedValue envObj(cx);
	GET_ENVIRONMENT_PROPERTY(tmpMapData, Environment, &envObj)

	if (envObj.isUndefined())
	{
		LOGWARNING(L"CMapReader::ParseEnvironment(): Environment settings not found");
		return 0;
	}

	//m_LightEnv.SetLightingModel("standard");
	if (pPostproc)
		pPostproc->SetPostEffect(L"default");

	std::wstring skySet;
	GET_ENVIRONMENT_PROPERTY(envObj, SkySet, skySet)
	if (pSkyMan)
		pSkyMan->SetSkySet(skySet);

	CColor sunColor;
	GET_ENVIRONMENT_PROPERTY(envObj, SunColour, sunColor)
	m_LightEnv.m_SunColor = RGBColor(sunColor.r, sunColor.g, sunColor.b);

	GET_ENVIRONMENT_PROPERTY(envObj, SunElevation, m_LightEnv.m_Elevation)
	GET_ENVIRONMENT_PROPERTY(envObj, SunRotation, m_LightEnv.m_Rotation)
	
	CColor terrainAmbientColor;
	GET_ENVIRONMENT_PROPERTY(envObj, TerrainAmbientColour, terrainAmbientColor)
	m_LightEnv.m_TerrainAmbientColor = RGBColor(terrainAmbientColor.r, terrainAmbientColor.g, terrainAmbientColor.b);

	CColor unitsAmbientColor;
	GET_ENVIRONMENT_PROPERTY(envObj, UnitsAmbientColour, unitsAmbientColor)
	m_LightEnv.m_UnitsAmbientColor = RGBColor(unitsAmbientColor.r, unitsAmbientColor.g, unitsAmbientColor.b);

	// Water properties
	JS::RootedValue waterObj(cx);
	GET_ENVIRONMENT_PROPERTY(envObj, Water, &waterObj)

	JS::RootedValue waterBodyObj(cx);
	GET_ENVIRONMENT_PROPERTY(waterObj, WaterBody, &waterBodyObj)

	// Water level - necessary
	float waterHeight;
	GET_ENVIRONMENT_PROPERTY(waterBodyObj, Height, waterHeight)

	CmpPtr<ICmpWaterManager> cmpWaterManager(*pSimulation2, SYSTEM_ENTITY);
	ENSURE(cmpWaterManager);
	cmpWaterManager->SetWaterLevel(entity_pos_t::FromFloat(waterHeight));

	// If we have graphics, get rest of settings
	if (pWaterMan)
	{
		GET_ENVIRONMENT_PROPERTY(waterBodyObj, Type, pWaterMan->m_WaterType)
		if (pWaterMan->m_WaterType == L"default")
			pWaterMan->m_WaterType = L"ocean";
		GET_ENVIRONMENT_PROPERTY(waterBodyObj, Colour, pWaterMan->m_WaterColor)
		GET_ENVIRONMENT_PROPERTY(waterBodyObj, Tint, pWaterMan->m_WaterTint)
		GET_ENVIRONMENT_PROPERTY(waterBodyObj, Waviness, pWaterMan->m_Waviness)
		GET_ENVIRONMENT_PROPERTY(waterBodyObj, Murkiness, pWaterMan->m_Murkiness)
		GET_ENVIRONMENT_PROPERTY(waterBodyObj, WindAngle, pWaterMan->m_WindAngle)
	}

	JS::RootedValue fogObject(cx);
	GET_ENVIRONMENT_PROPERTY(envObj, Fog, &fogObject);

	GET_ENVIRONMENT_PROPERTY(fogObject, FogFactor, m_LightEnv.m_FogFactor);
	GET_ENVIRONMENT_PROPERTY(fogObject, FogThickness, m_LightEnv.m_FogMax);

	CColor fogColor;
	GET_ENVIRONMENT_PROPERTY(fogObject, FogColor, fogColor);
	m_LightEnv.m_FogColor = RGBColor(fogColor.r, fogColor.g, fogColor.b);

	JS::RootedValue postprocObject(cx);
	GET_ENVIRONMENT_PROPERTY(envObj, Postproc, &postprocObject);

	std::wstring postProcEffect;
	GET_ENVIRONMENT_PROPERTY(postprocObject, PostprocEffect, postProcEffect);

	if (pPostproc)
		pPostproc->SetPostEffect(postProcEffect);

	GET_ENVIRONMENT_PROPERTY(postprocObject, Brightness, m_LightEnv.m_Brightness);
	GET_ENVIRONMENT_PROPERTY(postprocObject, Contrast, m_LightEnv.m_Contrast);
	GET_ENVIRONMENT_PROPERTY(postprocObject, Saturation, m_LightEnv.m_Saturation);
	GET_ENVIRONMENT_PROPERTY(postprocObject, Bloom, m_LightEnv.m_Bloom);

	m_LightEnv.CalculateSunDirection();

#undef GET_ENVIRONMENT_PROPERTY

	return 0;
}

int CMapReader::ParseCamera()
{
	JSContext* cx = pSimulation2->GetScriptInterface().GetContext();
	JSAutoRequest rq(cx);
	// parse camera settings from map data
	// defaults if we don't find player starting camera
	float declination = DEGTORAD(30.f), rotation = DEGTORAD(-45.f);
	CVector3D translation = CVector3D(100, 150, -100);

#define GET_CAMERA_PROPERTY(val, prop, out)\
	if (!pSimulation2->GetScriptInterface().GetProperty(val, #prop, out))\
		LOGWARNING(L"CMapReader::ParseCamera() failed to get '%hs' property", #prop);

	JS::RootedValue tmpMapData(cx, m_MapData.get()); // TODO: Check if this temporary root can be removed after SpiderMonkey 31 upgrade 
	JS::RootedValue cameraObj(cx);
	GET_CAMERA_PROPERTY(tmpMapData, Camera, &cameraObj)

	if (!cameraObj.isUndefined())
	{	// If camera property exists, read values
		CFixedVector3D pos;
		GET_CAMERA_PROPERTY(cameraObj, Position, pos)
		translation = pos;

		GET_CAMERA_PROPERTY(cameraObj, Rotation, rotation)
		GET_CAMERA_PROPERTY(cameraObj, Declination, declination)
	}
#undef GET_CAMERA_PROPERTY

	if (pGameView)
	{
		pGameView->GetCamera()->m_Orientation.SetXRotation(declination);
		pGameView->GetCamera()->m_Orientation.RotateY(rotation);
		pGameView->GetCamera()->m_Orientation.Translate(translation);
		pGameView->GetCamera()->UpdateFrustum();
	}

	return 0;
}

CMapReader::~CMapReader()
{
	// Cleaup objects
	delete xml_reader;
	delete m_MapGen;
}
