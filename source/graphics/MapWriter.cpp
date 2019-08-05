/* Copyright (C) 2019 Wildfire Games.
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

#include "Camera.h"
#include "CinemaManager.h"
#include "GameView.h"
#include "LightEnv.h"
#include "MapReader.h"
#include "MapWriter.h"
#include "Patch.h"
#include "Terrain.h"
#include "TerrainTextureEntry.h"
#include "TerrainTextureManager.h"

#include "maths/MathUtil.h"
#include "maths/NUSpline.h"
#include "ps/CLogger.h"
#include "ps/Loader.h"
#include "ps/Filesystem.h"
#include "ps/XML/XMLWriter.h"
#include "renderer/PostprocManager.h"
#include "renderer/SkyManager.h"
#include "renderer/WaterManager.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpCinemaManager.h"
#include "simulation2/components/ICmpObstruction.h"
#include "simulation2/components/ICmpOwnership.h"
#include "simulation2/components/ICmpPosition.h"
#include "simulation2/components/ICmpTemplateManager.h"
#include "simulation2/components/ICmpVisual.h"
#include "simulation2/components/ICmpWaterManager.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// CMapWriter constructor: nothing to do at the minute
CMapWriter::CMapWriter()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// SaveMap: try to save the current map to the given file
void CMapWriter::SaveMap(const VfsPath& pathname, CTerrain* pTerrain,
						 WaterManager* pWaterMan, SkyManager* pSkyMan,
						 CLightEnv* pLightEnv, CCamera* pCamera, CCinemaManager* UNUSED(pCinema),
						 CPostprocManager* pPostproc,
						 CSimulation2* pSimulation2)
{
	CFilePacker packer(FILE_VERSION, "PSMP");

	// build necessary data
	PackMap(packer, pTerrain);

	try
	{
		// write it out
		packer.Write(pathname);
	}
	catch (PSERROR_File_WriteFailed&)
	{
		LOGERROR("Failed to write map '%s'", pathname.string8());
		return;
	}

	VfsPath pathnameXML = pathname.ChangeExtension(L".xml");
	WriteXML(pathnameXML, pWaterMan, pSkyMan, pLightEnv, pCamera, pPostproc, pSimulation2);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// GetHandleIndex: return the index of the given handle in the given list; or 0xFFFF if
// handle isn't in list
static u16 GetEntryIndex(const CTerrainTextureEntry* entry, const std::vector<CTerrainTextureEntry*>& entries)
{
	const size_t limit = std::min(entries.size(), size_t(0xFFFEu));	// paranoia
	for (size_t i=0;i<limit;i++) {
		if (entries[i]==entry) {
			return (u16)i;
		}
	}

	return 0xFFFF;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// EnumTerrainTextures: build lists of textures used by map, and tile descriptions for
// each tile on the terrain
void CMapWriter::EnumTerrainTextures(CTerrain *pTerrain,
									 std::vector<CStr>& textures,
									 std::vector<STileDesc>& tiles)
{
	// the list of all handles in use
	std::vector<CTerrainTextureEntry*> entries;

	// resize tile array to required size
	tiles.resize(SQR(pTerrain->GetVerticesPerSide()-1));
	STileDesc* tileptr=&tiles[0];

	// now iterate through all the tiles
	const ssize_t patchesPerSide=pTerrain->GetPatchesPerSide();
	for (ssize_t j=0;j<patchesPerSide;j++) {
		for (ssize_t i=0;i<patchesPerSide;i++) {
			for (ssize_t m=0;m<PATCH_SIZE;m++) {
				for (ssize_t k=0;k<PATCH_SIZE;k++) {
					CMiniPatch& mp=pTerrain->GetPatch(i,j)->m_MiniPatches[m][k];	// can't fail
					u16 index=u16(GetEntryIndex(mp.GetTextureEntry(),entries));
					if (index==0xFFFF) {
						index=(u16)entries.size();
						entries.push_back(mp.GetTextureEntry());
					}

					tileptr->m_Tex1Index=index;
					tileptr->m_Tex2Index=0xFFFF;
					tileptr->m_Priority=mp.GetPriority();
					tileptr++;
				}
			}
		}
	}

	// now find the texture names for each handle
	for (size_t i=0;i<entries.size();i++) {
		CStr texturename;
		CTerrainTextureEntry* texentry=entries[i];
		if (!texentry) {
			// uh-oh, this shouldn't happen; set texturename to empty string
			texturename="";
		} else {
			texturename=texentry->GetTag();
		}
		textures.push_back(texturename);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// PackMap: pack the current world into a raw data stream
void CMapWriter::PackMap(CFilePacker& packer, CTerrain* pTerrain)
{
	// now pack everything up
	PackTerrain(packer, pTerrain);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// PackTerrain: pack the terrain onto the end of the output data stream
//		- data: map size, heightmap, list of textures used by map, texture tile assignments
void CMapWriter::PackTerrain(CFilePacker& packer, CTerrain* pTerrain)
{
	// pack map size
	const ssize_t mapsize = pTerrain->GetPatchesPerSide();
	packer.PackSize(mapsize);

	// pack heightmap
	packer.PackRaw(pTerrain->GetHeightMap(),sizeof(u16)*SQR(pTerrain->GetVerticesPerSide()));

	// the list of textures used by map
	std::vector<CStr> terrainTextures;
	// descriptions of each tile
	std::vector<STileDesc> tiles;

	// build lists by scanning through the terrain
	EnumTerrainTextures(pTerrain, terrainTextures, tiles);

	// pack texture names
	const size_t numTextures = terrainTextures.size();
	packer.PackSize(numTextures);
	for (size_t i=0;i<numTextures;i++)
		packer.PackString(terrainTextures[i]);

	// pack tile data
	packer.PackRaw(&tiles[0],sizeof(STileDesc)*tiles.size());
}

void CMapWriter::WriteXML(const VfsPath& filename,
						  WaterManager* pWaterMan, SkyManager* pSkyMan,
						  CLightEnv* pLightEnv, CCamera* pCamera,
						  CPostprocManager* pPostproc,
						  CSimulation2* pSimulation2)
{
	XMLWriter_File xmlMapFile;
	{
		XMLWriter_Element scenarioTag(xmlMapFile, "Scenario");
		scenarioTag.Attribute("version", static_cast<int>(FILE_VERSION));

		ENSURE(pSimulation2);
		CSimulation2& sim = *pSimulation2;

		if (!sim.GetStartupScript().empty())
		{
			XMLWriter_Element scriptTag(xmlMapFile, "Script");
			scriptTag.Text(sim.GetStartupScript().c_str(), true);
		}

		{
			XMLWriter_Element environmentTag(xmlMapFile, "Environment");
			environmentTag.Setting("SkySet", pSkyMan->GetSkySet());
			{
				XMLWriter_Element sunColorTag(xmlMapFile, "SunColor");
				sunColorTag.Attribute("r", pLightEnv->m_SunColor.X); // yes, it's X/Y/Z...
				sunColorTag.Attribute("g", pLightEnv->m_SunColor.Y);
				sunColorTag.Attribute("b", pLightEnv->m_SunColor.Z);
			}
			{
				XMLWriter_Element SunElevationTag(xmlMapFile, "SunElevation");
				SunElevationTag.Attribute("angle", pLightEnv->m_Elevation);
			}
			{
				XMLWriter_Element sunRotationTag(xmlMapFile, "SunRotation");
				sunRotationTag.Attribute("angle", pLightEnv->m_Rotation);
			}
			{
				XMLWriter_Element terrainAmbientColorTag(xmlMapFile, "TerrainAmbientColor");
				terrainAmbientColorTag.Attribute("r", pLightEnv->m_TerrainAmbientColor.X);
				terrainAmbientColorTag.Attribute("g", pLightEnv->m_TerrainAmbientColor.Y);
				terrainAmbientColorTag.Attribute("b", pLightEnv->m_TerrainAmbientColor.Z);
			}
			{
				XMLWriter_Element unitsAmbientColorTag(xmlMapFile, "UnitsAmbientColor");
				unitsAmbientColorTag.Attribute("r", pLightEnv->m_UnitsAmbientColor.X);
				unitsAmbientColorTag.Attribute("g", pLightEnv->m_UnitsAmbientColor.Y);
				unitsAmbientColorTag.Attribute("b", pLightEnv->m_UnitsAmbientColor.Z);
			}
			{
				XMLWriter_Element fogTag(xmlMapFile, "Fog");
				fogTag.Setting("FogFactor", pLightEnv->m_FogFactor);
				fogTag.Setting("FogThickness", pLightEnv->m_FogMax);
				{
					XMLWriter_Element fogColorTag(xmlMapFile, "FogColor");
					fogColorTag.Attribute("r", pLightEnv->m_FogColor.X);
					fogColorTag.Attribute("g", pLightEnv->m_FogColor.Y);
					fogColorTag.Attribute("b", pLightEnv->m_FogColor.Z);
				}
			}

			{
				XMLWriter_Element waterTag(xmlMapFile, "Water");
				{
					XMLWriter_Element waterBodyTag(xmlMapFile, "WaterBody");
					CmpPtr<ICmpWaterManager> cmpWaterManager(sim, SYSTEM_ENTITY);
					ENSURE(cmpWaterManager);
					waterBodyTag.Setting("Type", pWaterMan->m_WaterType);
					{
						XMLWriter_Element colorTag(xmlMapFile, "Color");
						colorTag.Attribute("r", pWaterMan->m_WaterColor.r);
						colorTag.Attribute("g", pWaterMan->m_WaterColor.g);
						colorTag.Attribute("b", pWaterMan->m_WaterColor.b);
					}
					{
						XMLWriter_Element tintTag(xmlMapFile, "Tint");
						tintTag.Attribute("r", pWaterMan->m_WaterTint.r);
						tintTag.Attribute("g", pWaterMan->m_WaterTint.g);
						tintTag.Attribute("b", pWaterMan->m_WaterTint.b);
					}
					waterBodyTag.Setting("Height", cmpWaterManager->GetExactWaterLevel(0, 0));
					waterBodyTag.Setting("Waviness", pWaterMan->m_Waviness);
					waterBodyTag.Setting("Murkiness", pWaterMan->m_Murkiness);
					waterBodyTag.Setting("WindAngle", pWaterMan->m_WindAngle);
				}
			}

			{
				XMLWriter_Element postProcTag(xmlMapFile, "Postproc");
				{
					postProcTag.Setting("Brightness", pLightEnv->m_Brightness);
					postProcTag.Setting("Contrast", pLightEnv->m_Contrast);
					postProcTag.Setting("Saturation", pLightEnv->m_Saturation);
					postProcTag.Setting("Bloom", pLightEnv->m_Bloom);
					postProcTag.Setting("PostEffect", pPostproc->GetPostEffect());
				}
			}
		}

		{
			XMLWriter_Element cameraTag(xmlMapFile, "Camera");
			{
				XMLWriter_Element positionTag(xmlMapFile, "Position");
				CVector3D pos = pCamera->m_Orientation.GetTranslation();
				positionTag.Attribute("x", pos.X);
				positionTag.Attribute("y", pos.Y);
				positionTag.Attribute("z", pos.Z);
			}

			CVector3D in = pCamera->m_Orientation.GetIn();
			// Convert to spherical coordinates
			float rotation = atan2(in.X, in.Z);
			float declination = atan2(sqrt(in.X*in.X + in.Z*in.Z), in.Y) - static_cast<float>(M_PI / 2);

			{
				XMLWriter_Element rotationTag(xmlMapFile, "Rotation");
				rotationTag.Attribute("angle", rotation);
			}
			{
				XMLWriter_Element declinationTag(xmlMapFile, "Declination");
				declinationTag.Attribute("angle", declination);
			}
		}

		{
			std::string settings = sim.GetMapSettingsString();
			if (!settings.empty())
			{
				XMLWriter_Element scriptSettingsTag(xmlMapFile, "ScriptSettings");
				scriptSettingsTag.Text(("\n" + settings + "\n").c_str(), true);
			}
		}

		{
			XMLWriter_Element entitiesTag(xmlMapFile, "Entities");
			CmpPtr<ICmpTemplateManager> cmpTemplateManager(sim, SYSTEM_ENTITY);
			ENSURE(cmpTemplateManager);

			// This will probably need to be changed in the future, but for now we'll
			// just save all entities that have a position
			CSimulation2::InterfaceList ents = sim.GetEntitiesWithInterface(IID_Position);
			for (CSimulation2::InterfaceList::const_iterator it = ents.begin(); it != ents.end(); ++it)
			{
				entity_id_t ent = it->first;

				// Don't save local entities (placement previews etc)
				if (ENTITY_IS_LOCAL(ent))
					continue;

				XMLWriter_Element entityTag(xmlMapFile, "Entity");
				entityTag.Attribute("uid", ent);

				entityTag.Setting("Template", cmpTemplateManager->GetCurrentTemplateName(ent));

				CmpPtr<ICmpOwnership> cmpOwnership(sim, ent);
				if (cmpOwnership)
					entityTag.Setting("Player", static_cast<int>(cmpOwnership->GetOwner()));

				CmpPtr<ICmpPosition> cmpPosition(sim, ent);
				if (cmpPosition)
				{
					CFixedVector3D pos;
					if (cmpPosition->IsInWorld())
						pos = cmpPosition->GetPosition();

					CFixedVector3D rot = cmpPosition->GetRotation();
					{
						XMLWriter_Element positionTag(xmlMapFile, "Position");
						positionTag.Attribute("x", pos.X);
						positionTag.Attribute("z", pos.Z);
						// TODO: height offset etc
					}
					{
						XMLWriter_Element orientationTag(xmlMapFile, "Orientation");
						orientationTag.Attribute("y", rot.Y);
						// TODO: X, Z maybe
					}
				}

				CmpPtr<ICmpObstruction> cmpObstruction(sim, ent);
				if (cmpObstruction)
				{
					// TODO: Currently only necessary because Atlas
					// does not set up control groups for its walls.
					cmpObstruction->ResolveFoundationCollisions();

					entity_id_t group = cmpObstruction->GetControlGroup();
					entity_id_t group2 = cmpObstruction->GetControlGroup2();

					// Don't waste space writing the default control groups.
					if (group != ent || group2 != INVALID_ENTITY)
					{
						XMLWriter_Element obstructionTag(xmlMapFile, "Obstruction");
						if (group != ent)
							obstructionTag.Attribute("group", group);
						if (group2 != INVALID_ENTITY)
							obstructionTag.Attribute("group2", group2);
					}
				}

				CmpPtr<ICmpVisual> cmpVisual(sim, ent);
				if (cmpVisual)
				{
					entity_id_t seed = static_cast<entity_id_t>(cmpVisual->GetActorSeed());
					if (seed != ent)
					{
						XMLWriter_Element actorTag(xmlMapFile, "Actor");
						actorTag.Attribute("seed",seed);
					}
					// TODO: variation/selection strings
				}
			}
		}


		CmpPtr<ICmpCinemaManager> cmpCinemaManager(sim, SYSTEM_ENTITY);
		if (cmpCinemaManager)
		{
			const std::map<CStrW, CCinemaPath>& paths = cmpCinemaManager->GetPaths();
			std::map<CStrW, CCinemaPath>::const_iterator it = paths.begin();
			XMLWriter_Element pathsTag(xmlMapFile, "Paths");

			for ( ; it != paths.end(); ++it )
			{
				fixed timescale = it->second.GetTimescale();
				const std::vector<SplineData>& position_nodes = it->second.GetAllNodes();
				const std::vector<SplineData>& target_nodes = it->second.GetTargetSpline().GetAllNodes();
				const CCinemaData* data = it->second.GetData();

				XMLWriter_Element pathTag(xmlMapFile, "Path");
				pathTag.Attribute("name", data->m_Name);
				pathTag.Attribute("timescale", timescale);
				pathTag.Attribute("orientation", data->m_Orientation);
				pathTag.Attribute("mode", data->m_Mode);
				pathTag.Attribute("style", data->m_Style);

				struct SEvent
				{
					fixed time;
					const char* type;
					CFixedVector3D value;
					SEvent(fixed time, const char* type, CFixedVector3D value)
						: time(time), type(type), value(value)
					{}
					bool operator<(const SEvent& another) const
					{
						return time < another.time;
					}
				};

				// All events of a manipulating of camera (position/rotation/target)
				std::vector<SEvent> events;
				events.reserve(position_nodes.size() + target_nodes.size());

				fixed last_position = fixed::Zero();
				for (size_t i = 0; i < position_nodes.size(); ++i)
				{
					fixed distance = i > 0 ? position_nodes[i - 1].Distance : fixed::Zero();
					last_position += distance;
					events.emplace_back(last_position, "Position", position_nodes[i].Position);
				}

				fixed last_target = fixed::Zero();
				for (size_t i = 0; i < target_nodes.size(); ++i)
				{
					fixed distance = i > 0 ? target_nodes[i - 1].Distance : fixed::Zero();
					last_target += distance;
					events.emplace_back(last_target, "Target", target_nodes[i].Position);
				}

				std::sort(events.begin(), events.end());
				for (size_t i = 0; i < events.size();)
				{
					XMLWriter_Element nodeTag(xmlMapFile, "Node");
					fixed deltatime = i > 0 ? (events[i].time - events[i - 1].time) : fixed::Zero();
					nodeTag.Attribute("deltatime", deltatime);
					size_t j = i;
					for (; j < events.size() && events[j].time == events[i].time; ++j)
					{
						// Types: Position/Rotation/Target
						XMLWriter_Element eventTypeTag(xmlMapFile, events[j].type);
						eventTypeTag.Attribute("x", events[j].value.X);
						eventTypeTag.Attribute("y", events[j].value.Y);
						eventTypeTag.Attribute("z", events[j].value.Z);
					}
					i = j;
				}
			}
		}
	}
	if (!xmlMapFile.StoreVFS(g_VFS, filename))
		LOGERROR("Failed to write map '%s'", filename.string8());
}
