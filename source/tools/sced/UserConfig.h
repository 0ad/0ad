#ifndef _USERCONFIG_H
#define _USERCONFIG_H

#include "ps\CStr.h"

enum ECfgOption {
	CFG_MAPLOADDIR,
	CFG_MAPSAVEDIR,
	CFG_TERRAINLOADDIR,
	CFG_TERRAINSAVEDIR,
	CFG_PMDSAVEDIR,
	CFG_MODELLOADDIR,
	CFG_MODELTEXLOADDIR,
	CFG_MODELANIMATIONDIR,
	CFG_TEXTUREEXT,
	CFG_SCROLLSPEED
};

class CUserConfig 
{
public:
	CUserConfig();

	void SetOptionString(ECfgOption opt,const char* str);
	const char* GetOptionString(ECfgOption opt);

	void SetOptionInt(ECfgOption opt,int value);
	int GetOptionInt(ECfgOption opt);

private:
	// map load directory
	CStr m_MapLoadDir;
	// map save directory
	CStr m_MapSaveDir;
	// terrain load directory
	CStr m_TerrainLoadDir;
	// terrain save directory
	CStr m_TerrainSaveDir;
	// PMD save directory
	CStr m_PMDSaveDir;
	// model load directory
	CStr m_ModelLoadDir;
	// model texture load directory
	CStr m_ModelTexLoadDir;
	// model animation load directory
	CStr m_ModelAnimationDir;
	// texture file extension
	CStr m_TextureExt;
	// map scroll speed
	int m_ScrollSpeed;
};

extern CUserConfig g_UserCfg;

#endif