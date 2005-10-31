#include "precompiled.h"

#include "MessageHandler.h"

#include "../CommandProc.h"

#include "graphics/TextureManager.h"

#include "../Brushes.h"

namespace AtlasMessage {

QUERYHANDLER(GetTerrainGroups)
{
	const CTextureManager::TerrainGroupMap &groups = g_TexMan.GetGroups();
	for (CTextureManager ::TerrainGroupMap::const_iterator it = groups.begin(); it != groups.end(); ++it)
		msg->groupnames.push_back(CStrW(it->first));
}

}
