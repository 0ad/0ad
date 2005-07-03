#include "precompiled.h"

#include "MessageHandler.h"

#include "graphics/Patch.h"
#include "graphics/TextureManager.h"
#include "graphics/TextureEntry.h"
#include "ps/Game.h"

namespace AtlasMessage {


void fGenerateMap(IMessage* msg)
{
	mGenerateMap* cmd = static_cast<mGenerateMap*>(msg);

	// Convert size in patches to number of vertices
	int vertices = cmd->size * PATCH_SIZE + 1;

	// Generate flat heightmap
	u16* heightmap = new u16[vertices*vertices];
	for (int z = 0; z < vertices; ++z)
		for (int x = 0; x < vertices; ++x)
			heightmap[x + z*vertices] = 32768 +(int)(2048.f*(rand()/(float)RAND_MAX-0.5f));

	// Initialise terrain using the heightmap
	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
	terrain->Initialize(cmd->size, heightmap);

	delete[] heightmap;

	// Cover terrain with default texture
	// TODO: split into fCoverWithTexture
	CTextureEntry* texentry = g_TexMan.FindTexture("grass1_spring.dds"); // TODO: make default customisable
	Handle tex = texentry ? texentry->GetHandle() : 0;

	int patches = terrain->GetPatchesPerSide();
	for (int pz = 0; pz < patches; ++pz) {
		for (int px = 0; px < patches; ++px) {

			CPatch* patch = terrain->GetPatch(px, pz);

			for (int z = 0; z < PATCH_SIZE; ++z)
				for (int x = 0; x < PATCH_SIZE; ++x)
					patch->m_MiniPatches[z][x].Tex1 = tex;
		}
	}

}
REGISTER(GenerateMap);

}
