#include "UserConfig.h"
#include <assert.h>

CUserConfig g_UserCfg;

CUserConfig::CUserConfig()
{
	m_ScrollSpeed=5;
	m_MapLoadDir="mods\\official\\maps\\scenarios";
	m_MapSaveDir="mods\\official\\maps\\scenarios";
	m_TerrainLoadDir="mods\\official\\art\\textures\\terrain";
	m_TerrainSaveDir="mods\\official\\art\\textures\\terrain";
	m_PMDSaveDir=".";
	m_ModelLoadDir="mods\\official\\art\\meshes";
	m_ModelTexLoadDir="mods\\official\\art\\textures\\skins";
	m_ModelAnimationDir="mods\\official\\art\\animation";
	m_TextureExt="dds";
}

void CUserConfig::SetOptionString(ECfgOption opt,const char* str)
{
	switch (opt) {
		case CFG_MAPLOADDIR:
			m_MapLoadDir=str;
			break;

		case CFG_MAPSAVEDIR:
			m_MapSaveDir=str;
			break;

		case CFG_TERRAINLOADDIR:
			m_TerrainLoadDir=str;
			break;

		case CFG_TERRAINSAVEDIR:
			m_TerrainSaveDir=str;
			break;

		case CFG_PMDSAVEDIR:
			m_PMDSaveDir=str;
			break;
	
		case CFG_MODELLOADDIR:
			m_ModelLoadDir=str;
			break;
	
		case CFG_MODELTEXLOADDIR:
			m_ModelTexLoadDir=str;
			break;

		case CFG_MODELANIMATIONDIR:
			m_ModelAnimationDir=str;
			break;

		case CFG_TEXTUREEXT:
			m_TextureExt=str;
			break;

		default:
			assert(0 && "unhandled case statement");
	}
}

const char* CUserConfig::GetOptionString(ECfgOption opt)
{
	switch (opt) {
		case CFG_MAPLOADDIR:
			return (const char*) m_MapLoadDir;

		case CFG_MAPSAVEDIR:
			return (const char*) m_MapSaveDir;

		case CFG_TERRAINLOADDIR:
			return (const char*) m_TerrainLoadDir;

		case CFG_TERRAINSAVEDIR:
			return (const char*) m_TerrainSaveDir;
	
		case CFG_PMDSAVEDIR:
			return (const char*) m_PMDSaveDir;
	
		case CFG_MODELLOADDIR:
			return (const char*) m_ModelLoadDir;
	
		case CFG_MODELTEXLOADDIR:
			return (const char*) m_ModelTexLoadDir;

		case CFG_MODELANIMATIONDIR:
			return (const char*) m_ModelAnimationDir;

		case CFG_TEXTUREEXT:
			return (const char*) m_TextureExt;

		default:
			assert(0 && "unhandled case statement");
	}

	return 0;
}


void CUserConfig::SetOptionInt(ECfgOption opt,int value)
{
	switch (opt) {
		case CFG_SCROLLSPEED:
			m_ScrollSpeed=value;
			break;

		default:
			assert(0 && "unhandled case statement");
	}
}

int CUserConfig::GetOptionInt(ECfgOption opt)
{
	switch (opt) {
		case CFG_SCROLLSPEED:
			return m_ScrollSpeed;

		default:
			assert(0 && "unhandled case statement");
	}

	return 0;
}

