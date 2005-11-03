#include "precompiled.h"

#include "MessageHandler.h"

#include "../CommandProc.h"

#include "graphics/TextureManager.h"
#include "graphics/TextureEntry.h"

#include "../Brushes.h"

namespace AtlasMessage {

QUERYHANDLER(GetTerrainGroups)
{
	const CTextureManager::TerrainGroupMap &groups = g_TexMan.GetGroups();
	for (CTextureManager ::TerrainGroupMap::const_iterator it = groups.begin(); it != groups.end(); ++it)
		msg->groupnames.push_back(CStrW(it->first));
}

QUERYHANDLER(GetTerrainGroupPreviews)
{
	CTerrainGroup* group = g_TexMan.FindGroup(msg->groupname);
	for (std::vector<CTextureEntry*>::const_iterator it = group->GetTerrains().begin(); it != group->GetTerrains().end(); ++it)
	{
		msg->previews.push_back(AtlasMessage::sTerrainGroupPreview());
		msg->previews.back().name = CStrW((*it)->GetTag());

		u32 c = (*it)->GetBaseColor();
		unsigned char* buf = (unsigned char*)malloc(msg->imagewidth*msg->imageheight*3);

		// TODO: An actual preview of the texture. (There's no need to shrink
		// the entire texture to fit, since it's the small details in the
		// texture that are interesting, so we could just crop a chunk out of
		// the middle.)
		for (int i = 0; i < msg->imagewidth*msg->imageheight; ++i)
		{
			buf[i*3+0] = (c>>16) & 0xff;
			buf[i*3+1] = (c>>8) & 0xff;
			buf[i*3+2] = (c>>0) & 0xff;
		}

		msg->previews.back().imagedata = buf;
	}

}

}
