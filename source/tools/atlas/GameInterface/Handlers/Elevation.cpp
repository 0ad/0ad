#include "precompiled.h"

#include "MessageHandler.h"

#include "graphics/Terrain.h"
#include "ps/Game.h"

namespace AtlasMessage {


void fAlterElevation(IMessage* msg)
{
	mAlterElevation* cmd = static_cast<mAlterElevation*>(msg);

	// TODO:
	// Create something like the AtlasCommandProcessor, so that undo is
	// possible: Push an AlterElevationCommand onto the command stack,
	// which has Do and Undo methods, and also has Merge (so multiple
	// consecutive elevation-altering moments can be combined into a single
	// brush stroke, to conserve memory and also so 'undo' undoes the entire
	// stroke at once).

	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();

	u16* heightmap = terrain->GetHeightMap();
	int size = terrain->GetVerticesPerSide();

	int x = (int)cmd->pos.x;
	int z = (int)cmd->pos.z;
	terrain->RaiseVertex(x, z, (int)cmd->amount);
	terrain->MakeDirty(x, z, x, z);
}
REGISTER(AlterElevation);

}
