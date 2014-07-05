/* Copyright (C) 2014 Wildfire Games.
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
#include "CinemaTrack.h"
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
						 CLightEnv* pLightEnv, CCamera* pCamera, CCinemaManager* pCinema,
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
		LOGERROR(L"Failed to write map '%ls'", pathname.string().c_str());
		return;
	}

	VfsPath pathnameXML = pathname.ChangeExtension(L".xml");
	WriteXML(pathnameXML, pWaterMan, pSkyMan, pLightEnv, pCamera, pCinema, pPostproc, pSimulation2);
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
						  CLightEnv* pLightEnv, CCamera* pCamera, CCinemaManager* pCinema,
						  CPostprocManager* pPostproc,
						  CSimulation2* pSimulation2)
{
	XML_Start();

	{
		XML_Element("Scenario");
		XML_Attribute("version", (int)FILE_VERSION);

		ENSURE(pSimulation2);
		CSimulation2& sim = *pSimulation2;

		if (!sim.GetStartupScript().empty())
		{
			XML_Element("Script");
			XML_CDATA(sim.GetStartupScript().c_str());
		}

		{
			XML_Element("Environment");

			XML_Setting("SkySet", pSkyMan->GetSkySet());
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
			{
				XML_Element("Fog");
				XML_Setting("FogFactor", pLightEnv->m_FogFactor);
				XML_Setting("FogThickness", pLightEnv->m_FogMax);
				{
					XML_Element("FogColour");
					XML_Attribute("r", pLightEnv->m_FogColor.X);
					XML_Attribute("g", pLightEnv->m_FogColor.Y);
					XML_Attribute("b", pLightEnv->m_FogColor.Z);
				}
			}

			{
				XML_Element("Water");
				{
					XML_Element("WaterBody");
					CmpPtr<ICmpWaterManager> cmpWaterManager(sim, SYSTEM_ENTITY);
					ENSURE(cmpWaterManager);
					XML_Setting("Type", pWaterMan->m_WaterType);
					{
						XML_Element("Colour");
						XML_Attribute("r", pWaterMan->m_WaterColor.r);
						XML_Attribute("g", pWaterMan->m_WaterColor.g);
						XML_Attribute("b", pWaterMan->m_WaterColor.b);
					}
					{
						XML_Element("Tint");
						XML_Attribute("r", pWaterMan->m_WaterTint.r);
						XML_Attribute("g", pWaterMan->m_WaterTint.g);
						XML_Attribute("b", pWaterMan->m_WaterTint.b);
					}
					XML_Setting("Height", cmpWaterManager->GetExactWaterLevel(0, 0));
					XML_Setting("Waviness", pWaterMan->m_Waviness);
					XML_Setting("Murkiness", pWaterMan->m_Murkiness);
					XML_Setting("WindAngle", pWaterMan->m_WindAngle);
				}
			}
			
			{
				XML_Element("Postproc");
				{
					XML_Setting("Brightness", pLightEnv->m_Brightness);
					XML_Setting("Contrast", pLightEnv->m_Contrast);
					XML_Setting("Saturation", pLightEnv->m_Saturation);
					XML_Setting("Bloom", pLightEnv->m_Bloom);
					XML_Setting("PostEffect", pPostproc->GetPostEffect());
				}
			}
		}

		{
			XML_Element("Camera");

			{
				XML_Element("Position");
				CVector3D pos = pCamera->m_Orientation.GetTranslation();
				XML_Attribute("x", pos.X);
				XML_Attribute("y", pos.Y);
				XML_Attribute("z", pos.Z);
			}

			CVector3D in = pCamera->m_Orientation.GetIn();
			// Convert to spherical coordinates
			float rotation = atan2(in.X, in.Z);
			float declination = atan2(sqrt(in.X*in.X + in.Z*in.Z), in.Y) - (float)M_PI/2;

			{
				XML_Element("Rotation");
				XML_Attribute("angle", rotation);
			}
			{
				XML_Element("Declination");
				XML_Attribute("angle", declination);
			}
		}

		{
			std::string settings = sim.GetMapSettingsString();
			if (!settings.empty())
			{
				XML_Element("ScriptSettings");
				XML_CDATA(("\n" + settings + "\n").c_str());
			}
		}

		{
			XML_Element("Entities");

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

				XML_Element("Entity");
				XML_Attribute("uid", ent);

				XML_Setting("Template", cmpTemplateManager->GetCurrentTemplateName(ent));

				CmpPtr<ICmpOwnership> cmpOwnership(sim, ent);
				if (cmpOwnership)
					XML_Setting("Player", (int)cmpOwnership->GetOwner());

				CmpPtr<ICmpPosition> cmpPosition(sim, ent);
				if (cmpPosition)
				{
					CFixedVector3D pos = cmpPosition->GetPosition();
					CFixedVector3D rot = cmpPosition->GetRotation();
					{
						XML_Element("Position");
						XML_Attribute("x", pos.X);
						XML_Attribute("z", pos.Z);
						// TODO: height offset etc
					}
					{
						XML_Element("Orientation");
						XML_Attribute("y", rot.Y);
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
						XML_Element("Obstruction");
						if (group != ent)
							XML_Attribute("group", group);
						if (group2 != INVALID_ENTITY)
							XML_Attribute("group2", group2);
					}
				}

				CmpPtr<ICmpVisual> cmpVisual(sim, ent);
				if (cmpVisual)
				{
					u32 seed = cmpVisual->GetActorSeed();
					if (seed != (u32)ent)
					{
						XML_Element("Actor");
						XML_Attribute("seed", seed);
					}
					// TODO: variation/selection strings
				}
			}
		}

		const std::map<CStrW, CCinemaPath>& paths = pCinema->GetAllPaths();
		std::map<CStrW, CCinemaPath>::const_iterator it = paths.begin();
		
		{
			XML_Element("Paths");

			for ( ; it != paths.end(); ++it )
			{
				CStrW name = it->first;
				float timescale = it->second.GetTimescale();
				
				XML_Element("Path");
				XML_Attribute("name", name);
				XML_Attribute("timescale", timescale);

				const std::vector<SplineData>& nodes = it->second.GetAllNodes();
				const CCinemaData* data = it->second.GetData();
				
				{
					XML_Element("Distortion");
					XML_Attribute("mode", data->m_Mode);
					XML_Attribute("style", data->m_Style);
					XML_Attribute("growth", data->m_Growth);
					XML_Attribute("switch", data->m_Switch);
				}

				for ( ssize_t j=nodes.size()-1; j >= 0; --j )
				{
					XML_Element("Node");
					
					{
						XML_Element("Position");
						XML_Attribute("x", nodes[j].Position.X);
						XML_Attribute("y", nodes[j].Position.Y);
						XML_Attribute("z", nodes[j].Position.Z);
					}

					{
						XML_Element("Rotation");
						XML_Attribute("x", nodes[j].Rotation.X);
						XML_Attribute("y", nodes[j].Rotation.Y);
						XML_Attribute("z", nodes[j].Rotation.Z);
					}

					XML_Setting("Time", nodes[j].Distance);
				}
			}
		}
	}
	if (!XML_StoreVFS(g_VFS, filename))
		LOGERROR(L"Failed to write map '%ls'", filename.string().c_str());
}
