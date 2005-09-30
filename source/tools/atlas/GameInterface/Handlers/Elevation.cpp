#include "precompiled.h"

#include "MessageHandler.h"

#include "../CommandProc.h"

#include "graphics/Terrain.h"
#include "ps/Game.h"

namespace AtlasMessage {


BEGIN_COMMAND(AlterElevation)

	// TODO: much more efficient version of this, and without the memory leaks
	u16* OldTerrain;
	u16* NewTerrain;

	void Construct()
	{
		OldTerrain = NewTerrain = NULL;
	}
	void Destruct()
	{
		delete OldTerrain;
		delete NewTerrain;
	}

	void Do()
	{

		CTerrain* terrain = g_Game->GetWorld()->GetTerrain();

		int verts = terrain->GetVerticesPerSide()*terrain->GetVerticesPerSide();
		OldTerrain = new u16[verts];
		memcpy(OldTerrain, terrain->GetHeightMap(), verts*sizeof(u16));

		int amount = (int)d->amount;

		// If the framerate is very high, 'amount' is often very
		// small (even zero) so the integer truncation is significant
		static float roundingError = 0.0;
		roundingError += d->amount - (float)amount;
		if (roundingError >= 1.f)
		{
			amount += (int)roundingError;
			roundingError -= (float)(int)roundingError;
		}

		CVector3D vec;
		d->pos.GetWorldSpace(vec);
		uint32_t x, z;
		terrain->CalcFromPosition(vec, x, z);
		terrain->RaiseVertex(x, z, amount);
		terrain->MakeDirty(x, z, x, z);
	}

	void Undo()
	{
		CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
		if (! NewTerrain)
		{
			int verts = terrain->GetVerticesPerSide()*terrain->GetVerticesPerSide();
			NewTerrain = new u16[verts];
			memcpy(NewTerrain, terrain->GetHeightMap(), verts*sizeof(u16));
		}
		terrain->SetHeightMap(OldTerrain); // CTerrain duplicates the data
	}

	void Redo()
	{
		CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
		terrain->SetHeightMap(NewTerrain); // CTerrain duplicates the data
	}

	void MergeWithSelf(cAlterElevation* prev)
	{
		std::swap(prev->NewTerrain, NewTerrain);
	}

END_COMMAND(AlterElevation);

}
