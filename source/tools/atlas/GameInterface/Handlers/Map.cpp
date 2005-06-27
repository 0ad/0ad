#include "precompiled.h"

#include "MessageHandler.h"

#include "graphics/Patch.h"
#include "ps/Game.h"

namespace AtlasMessage {


void fGenerateMap(IMessage* msg)
{
	mGenerateMap* cmd = static_cast<mGenerateMap*>(msg);

	int tiles = cmd->size * PATCH_SIZE + 1;

	u16* heightmap = new u16[tiles*tiles];
	for (int y = 0; y < tiles; ++y)
		for (int x = 0; x < tiles; ++x)
			heightmap[x + y*tiles] = 32768;

	g_Game->GetWorld()->GetTerrain()->Resize(cmd->size);
	g_Game->GetWorld()->GetTerrain()->SetHeightMap(heightmap);

	delete[] heightmap;
}
REGISTER(GenerateMap);

}
